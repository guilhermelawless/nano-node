#include <nano/node/confirmation_solicitor.hpp>
#include <nano/node/election.hpp>

using namespace std::chrono_literals;

nano::confirmation_solicitor::confirmation_solicitor (nano::network & network_a, nano::network_constants const & params_a) :
max_confirm_req_batches (params_a.is_test_network () ? 1 : 20),
max_block_broadcasts (params_a.is_test_network () ? 4 : 30),
max_representatives (30),
network (network_a)
{
}

void nano::confirmation_solicitor::prepare (std::vector<nano::representative> const & representatives_a)
{
	assert (!prepared);
	requests.clear ();
	rebroadcasted = 0;
	representatives = representatives_a;
	prepared = true;
}

bool nano::confirmation_solicitor::broadcast (nano::election const & election_a)
{
	assert (prepared);
	bool error (true);
	if (rebroadcasted++ < max_block_broadcasts)
	{
		nano::publish message (election_a.status.winner);
		unsigned count = 0;
		// Directed broadcasting to get confirmations
		for (auto i (representatives.begin ()), n (representatives.end ()); i != n && count < max_representatives; ++i)
		{
			if (election_a.last_votes.find (i->account) == election_a.last_votes.end ())
			{
				i->channel->send (message);
				++count;
			}
		}
		// Random flood for block propagation
		network.flood_message (message, true, 0.5f);
		error = false;
	}
	return error;
}

bool nano::confirmation_solicitor::add (nano::election const & election_a)
{
	assert (prepared);
	auto const max_channel_requests (max_confirm_req_batches * nano::network::confirm_req_hashes_max);
	unsigned count = 0;
	for (auto i (representatives.begin ()), n (representatives.end ()); i != n && count < max_representatives; ++i)
	{
		auto rep (*i);
		if (election_a.last_votes.find (rep.account) == election_a.last_votes.end ())
		{
			auto & request_queue (requests[rep.channel]);
			if (request_queue.size () < max_channel_requests)
			{
				request_queue.emplace_back (election_a.status.winner->hash (), election_a.status.winner->root ());
				++count;
			}
		}
	}
	bool error (count == 0);
	return error;
}

void nano::confirmation_solicitor::flush ()
{
	assert (prepared);
	for (auto const & request_queue : requests)
	{
		auto const & channel (request_queue.first);
		std::vector<std::pair<nano::block_hash, nano::root>> roots_hashes_l;
		for (auto const & root_hash : request_queue.second)
		{
			roots_hashes_l.push_back (root_hash);
			if (roots_hashes_l.size () == nano::network::confirm_req_hashes_max)
			{
				nano::confirm_req req (roots_hashes_l);
				channel->send (req);
				roots_hashes_l.clear ();
			}
		}
		if (!roots_hashes_l.empty ())
		{
			nano::confirm_req req (roots_hashes_l);
			channel->send (req);
		}
	}
	prepared = false;
}
