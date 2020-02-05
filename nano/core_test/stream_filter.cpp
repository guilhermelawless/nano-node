#include <nano/core_test/testutil.hpp>
#include <nano/node/testing.hpp>

#include <gtest/gtest.h>

TEST (stream_filter, unit)
{
	nano::genesis genesis;
	nano::stream_filter filter (1);
	auto one_block = [&genesis, &filter](std::shared_ptr<nano::block> const & block_a, bool expect_duplicate_a) {
		nano::publish message (block_a);
		auto bytes (message.to_bytes ());
		nano::bufferstream stream (bytes->data (), bytes->size ());

		// First read the header
		bool error{ false };
		nano::message_header header (error, stream);
		ASSERT_FALSE (error);

		// Now filter the rest of the stream
		bool duplicate (filter (error, stream));
		ASSERT_EQ (expect_duplicate_a, duplicate);
		ASSERT_FALSE (error);

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

TEST (stream_filter, many)
{
	nano::system system;
	nano::genesis genesis;
	nano::stream_filter filter (4);
	nano::keypair key1;
	for (int i = 0; i < 100; ++i)
	{
		auto block (std::make_shared<nano::state_block> (nano::test_genesis_key.pub, genesis.open->hash (), nano::test_genesis_key.pub, nano::genesis_amount - i * 10 * nano::xrb_ratio, key1.pub, nano::test_genesis_key.prv, nano::test_genesis_key.pub, 0));

		nano::publish message (block);
		auto bytes (message.to_bytes ());
		nano::bufferstream stream (bytes->data (), bytes->size ());

		// First read the header
		bool error{ false };
		nano::message_header header (error, stream);
		ASSERT_FALSE (error);

		// Now filter the rest of the stream
		// All blocks should pass through
		ASSERT_FALSE (filter (error, stream));
		ASSERT_FALSE (error);

		// Make sure the stream was rewinded correctly
		auto deserialized_block (nano::deserialize_block (stream, header.block_type ()));
		ASSERT_NE (nullptr, deserialized_block);
		ASSERT_EQ (*block, *deserialized_block);
	}
}
