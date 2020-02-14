#include <nano/core_test/testutil.hpp>
#include <nano/lib/jsonconfig.hpp>
#include <nano/node/election.hpp>
#include <nano/node/testing.hpp>
#include <nano/node/transport/udp.hpp>

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include <numeric>

using namespace std::chrono_literals;

namespace
{
void add_required_children_node_config_tree (nano::jsonconfig & tree);
}

TEST (node, fork_open_flip)
{
	for (int i = 0; i < 1000; ++i)
	{
		nano::system system (2);
		auto & node1 (*system.nodes[0]);
		auto & node2 (*system.nodes[1]);
		ASSERT_EQ (1, node1.network.size ());
		nano::keypair key1;
		nano::genesis genesis;
		nano::keypair rep1;
		nano::keypair rep2;
		auto send1 (std::make_shared<nano::send_block> (genesis.hash (), key1.pub, nano::genesis_amount - 1, nano::test_genesis_key.prv, nano::test_genesis_key.pub, *system.work.generate (genesis.hash ())));
		node1.process_active (send1);
		node2.process_active (send1);
		// We should be keeping this block
		auto open1 (std::make_shared<nano::open_block> (send1->hash (), rep1.pub, key1.pub, key1.prv, key1.pub, *system.work.generate (key1.pub)));
		// This block should be evicted
		auto open2 (std::make_shared<nano::open_block> (send1->hash (), rep2.pub, key1.pub, key1.prv, key1.pub, *system.work.generate (key1.pub)));
		ASSERT_FALSE (*open1 == *open2);
		// node1 gets copy that will remain
		node1.process_active (open1);
		node1.block_processor.flush ();
		// node2 gets copy that will be evicted
		node2.process_active (open2);
		node2.block_processor.flush ();
		ASSERT_EQ (2, node1.active.size ());
		ASSERT_EQ (2, node2.active.size ());
		system.wallet (0)->insert_adhoc (nano::test_genesis_key.prv);
		// Notify both nodes that a fork exists
		node1.process_active (open2);
		node1.block_processor.flush ();
		node2.process_active (open1);
		node2.block_processor.flush ();
		nano::unique_lock<std::mutex> lock (node2.active.mutex);
		auto conflict (node2.active.roots.find (open1->qualified_root ()));
		ASSERT_NE (node2.active.roots.end (), conflict);
		auto votes1 (conflict->election);
		ASSERT_NE (nullptr, votes1);
		ASSERT_EQ (1, votes1->last_votes.size ());
		lock.unlock ();
		ASSERT_TRUE (node1.block (open1->hash ()) != nullptr);
		ASSERT_TRUE (node2.block (open2->hash ()) != nullptr);
		system.deadline_set (10s);
		// Node2 should eventually settle on open1
		while (node2.block (open1->hash ()) == nullptr)
		{
			ASSERT_NO_ERROR (system.poll ());
		}
		node2.block_processor.flush ();
		auto transaction1 (node1.store.tx_begin_read ());
		auto transaction2 (node2.store.tx_begin_read ());
		lock.lock ();
		auto winner (*votes1->tally ().begin ());
		ASSERT_EQ (*open1, *winner.second);
		ASSERT_EQ (nano::genesis_amount - 1, winner.first);
		ASSERT_TRUE (node1.store.block_exists (transaction1, open1->hash ()));
		ASSERT_TRUE (node2.store.block_exists (transaction2, open1->hash ()));
		ASSERT_FALSE (node2.store.block_exists (transaction2, open2->hash ()));
	}
}

namespace
{
void add_required_children_node_config_tree (nano::jsonconfig & tree)
{
	nano::logging logging1;
	nano::jsonconfig logging_l;
	logging1.serialize_json (logging_l);
	tree.put_child ("logging", logging_l);
	nano::jsonconfig preconfigured_peers_l;
	tree.put_child ("preconfigured_peers", preconfigured_peers_l);
	nano::jsonconfig preconfigured_representatives_l;
	tree.put_child ("preconfigured_representatives", preconfigured_representatives_l);
	nano::jsonconfig work_peers_l;
	tree.put_child ("work_peers", work_peers_l);
	tree.put ("version", std::to_string (nano::node_config::json_version ()));
}
}
