#include <nano/core_test/testutil.hpp>
#include <nano/node/testing.hpp>

#include <gtest/gtest.h>

TEST (stream_filter, one)
{
	nano::genesis genesis;
	nano::stream_filter filter (1);
	for (int i = 0; i < 10; ++i)
	{
		nano::publish message (genesis.open);
		auto bytes (message.to_bytes ());
		nano::bufferstream stream (bytes->data (), bytes->size ());

		// First read the header
		bool error{ false };
		nano::message_header header (error, stream);
		ASSERT_FALSE (error);

		// Now filter the rest of the stream
		bool existed (filter (error, stream));
		// After the first insertion, it should always be marked as existing (duplicate)
		ASSERT_EQ (i == 0 ? false : true, existed);
		ASSERT_FALSE (error);

		// Make sure the stream was rewinded correctly
		auto block (nano::deserialize_block (stream, header.block_type ()));
		ASSERT_NE (nullptr, block);
		ASSERT_EQ (*block, *genesis.open);
	}
}

TEST (stream_filter, many)
{
	nano::system system;
	nano::genesis genesis;
	nano::stream_filter filter (1024);
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
