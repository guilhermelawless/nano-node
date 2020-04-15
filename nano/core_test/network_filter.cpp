#include <nano/core_test/testutil.hpp>
#include <nano/node/common.hpp>
#include <nano/secure/buffer.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/network_filter.hpp>

#include <gtest/gtest.h>

TEST (network_filter, unit)
{
	nano::genesis genesis;
	nano::network_filter filter (1);
	auto one_block = [&filter](std::shared_ptr<nano::block> const & block_a, bool expect_duplicate_a) {
		nano::publish message (block_a);
		auto bytes (message.to_bytes (false));
		nano::bufferstream stream (bytes->data (), bytes->size ());

		// First read the header
		bool error{ false };
		nano::message_header header (error, stream);
		ASSERT_FALSE (error);

		// This validates nano::message_header::size
		ASSERT_EQ (bytes->size (), block_a->size (block_a->type ()) + header.size);

		// Now filter the rest of the stream
		bool duplicate (filter.apply (bytes->data (), bytes->size () - header.size));
		ASSERT_EQ (expect_duplicate_a, duplicate);

		// Make sure the stream was rewinded correctly
		auto block (nano::deserialize_block (stream, header.block_type ()));
		ASSERT_NE (nullptr, block);
		ASSERT_EQ (*block, *block_a);
	};
	one_block (genesis.open, false);
	for (int i = 0; i < 10; ++i)
	{
		one_block (genesis.open, true);
	}
	auto new_block (std::make_shared<nano::state_block> (nano::test_genesis_key.pub, genesis.open->hash (), nano::test_genesis_key.pub, nano::genesis_amount - 10 * nano::xrb_ratio, nano::public_key (), nano::test_genesis_key.prv, nano::test_genesis_key.pub, 0));
	one_block (new_block, false);
	for (int i = 0; i < 10; ++i)
	{
		one_block (new_block, true);
	}
	for (int i = 0; i < 100; ++i)
	{
		one_block (genesis.open, false);
		one_block (new_block, false);
	}
}

TEST (network_filter, many)
{
	nano::genesis genesis;
	nano::network_filter filter (4);
	nano::keypair key1;
	for (int i = 0; i < 100; ++i)
	{
		auto block (std::make_shared<nano::state_block> (nano::test_genesis_key.pub, genesis.open->hash (), nano::test_genesis_key.pub, nano::genesis_amount - i * 10 * nano::xrb_ratio, key1.pub, nano::test_genesis_key.prv, nano::test_genesis_key.pub, 0));

		nano::publish message (block);
		auto bytes (message.to_bytes (false));
		nano::bufferstream stream (bytes->data (), bytes->size ());

		// First read the header
		bool error{ false };
		nano::message_header header (error, stream);
		ASSERT_FALSE (error);

		// This validates nano::message_header::size
		ASSERT_EQ (bytes->size (), block->size + header.size);

		// Now filter the rest of the stream
		// All blocks should pass through
		ASSERT_FALSE (filter.apply (bytes->data (), block->size));
		ASSERT_FALSE (error);

		// Make sure the stream was rewinded correctly
		auto deserialized_block (nano::deserialize_block (stream, header.block_type ()));
		ASSERT_NE (nullptr, deserialized_block);
		ASSERT_EQ (*block, *deserialized_block);
	}
}

TEST (network_filter, clear)
{
	nano::network_filter filter (1);
	std::vector<uint8_t> bytes1{ 1, 2, 3 };
	std::vector<uint8_t> bytes2{ 1 };
	ASSERT_FALSE (filter.apply (bytes1.data (), bytes1.size ()));
	ASSERT_TRUE (filter.apply (bytes1.data (), bytes1.size ()));
	filter.clear (bytes1.data (), bytes1.size ());
	ASSERT_FALSE (filter.apply (bytes1.data (), bytes1.size ()));
	ASSERT_TRUE (filter.apply (bytes1.data (), bytes1.size ()));
	filter.clear (bytes2.data (), bytes2.size ());
	ASSERT_TRUE (filter.apply (bytes1.data (), bytes1.size ()));
	ASSERT_FALSE (filter.apply (bytes2.data (), bytes2.size ()));
}

TEST (network_filter, optional_digest)
{
	nano::network_filter filter (1);
	std::vector<uint8_t> bytes1{ 1, 2, 3 };
	nano::uint128_t digest{ 0 };
	ASSERT_FALSE (filter.apply (bytes1.data (), bytes1.size (), &digest));
	ASSERT_NE (0, digest);
	ASSERT_TRUE (filter.apply (bytes1.data (), bytes1.size ()));
	filter.clear (digest);
	ASSERT_FALSE (filter.apply (bytes1.data (), bytes1.size ()));
}

TEST (network_filter, cycle)
{
	// Simulating a new node bootstrapping a bunch of blocks and republishing those to us
	auto const messages = 100'000;
	auto const filter_size = 0.5 * messages;
	std::cout << messages / 1000 << "k messages passing through a filter of size " << filter_size / 1000 << "k" << std::endl;
	nano::network_filter filter (filter_size);
	filter.clear ();
	{
		auto copy = filter.copy ();
		ASSERT_TRUE (std::all_of (copy.begin (), copy.end (), [](auto const x) { return x == 0; }));
	}

	// First set the filter
	for (nano::uint128_t i = 0; i < messages; ++i)
	{
		nano::uint128_union x (i);
		filter.apply (x.bytes.data (), x.bytes.size ());
	}

	auto get_nonzero = [](nano::network_filter const & filter_a) {
		auto copy = filter_a.copy ();
		auto non_zero_values = std::count_if (copy.begin (), copy.end (), [](auto const x) { return x != 0; });
		return non_zero_values;
	};

	auto nonzero1 = get_nonzero (filter);
	std::cout << "First run: " << nonzero1 << " nonzero values; the filter is " << 100. * nonzero1 / filter_size << "% used " << std::endl;

	// Now record how many are filtered
	std::array<size_t, 2> counters1{ 0, 0 };
	for (nano::uint128_t i = 0; i < messages; ++i)
	{
		nano::uint128_union x (i);
		auto existed = filter.apply (x.bytes.data (), x.bytes.size ());
		++counters1[existed];
	}
	ASSERT_EQ (counters1[0] + counters1[1], messages);
	auto nonzero2 = get_nonzero (filter);
	ASSERT_EQ (nonzero2, nonzero1);

	// Efficiency is the true positive rate
	auto const efficiency = static_cast<float> (counters1[1]) / messages;

	std::cout << "Second run: " << counters1[1] << " messages were filtered (" << 100. * efficiency << "% of " << messages << ")" << std::endl;

	// Repeating the process should result in the same counters
	std::array<size_t, 2> counters2{ 0, 0 };
	for (nano::uint128_t i = 0; i < messages; ++i)
	{
		nano::uint128_union x (i);
		auto existed = filter.apply (x.bytes.data (), x.bytes.size ());
		++counters2[existed];
	}
	ASSERT_EQ (counters2[1], counters1[1]);
}
