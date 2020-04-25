#include <nano/node/distributed_work_manager.hpp>
#include <nano/node/node.hpp>

nano::distributed_work_manager::distributed_work_manager (nano::node & node_a) :
node (node_a)
{
}

nano::distributed_work_manager::~distributed_work_manager ()
{
	stop ();
}

bool nano::distributed_work_manager::make (nano::work_version const version_a, nano::root const & root_a, std::vector<std::pair<std::string, uint16_t>> const & peers_a, uint64_t difficulty_a, std::function<void(boost::optional<uint64_t>)> const & callback_a, boost::optional<nano::account> const & account_a)
{
	return make (std::chrono::seconds (1), nano::work_request{ version_a, root_a, difficulty_a, account_a, callback_a, peers_a });
}

bool nano::distributed_work_manager::make (std::chrono::seconds const & backoff_a, nano::work_request const & request_a)
{
	bool error_l{ true };
	if (!stopped)
	{
		if (node.work_generation_enabled (request_a.peers))
		{
			std::weak_ptr<nano::distributed_work_manager> this_w (shared_from_this ());
			auto identifier_l = ++counter;
			auto distributed (std::make_shared<nano::distributed_work> (node, request_a, backoff_a, [this_w, identifier_l] {
				if (auto this_l = this_w.lock ())
				{
					nano::unique_lock<std::mutex> lock (this_l->mutex, std::defer_lock);
					if (lock.try_lock ())
					{
						this_l->items.get<tag_id> ().erase (identifier_l);
					}
				}
			}));
			{
				nano::lock_guard<std::mutex> guard (mutex);
				error_l = !items.emplace (item_t{ identifier_l, request_a.root, distributed }).second;
			}
			if (!error_l)
			{
				distributed->start ();
			}
		}
	}
	return error_l;
}

void nano::distributed_work_manager::cancel (nano::root const & root_a, bool const local_stop)
{
	nano::lock_guard<std::mutex> guard_l (mutex);
	auto & items_by_root (items.get<tag_root> ());
	auto existing (items_by_root.equal_range (root_a));
	for (auto work : boost::make_iterator_range (existing))
	{
		if (auto distributed = work.item.lock ())
		{
			// Send work_cancel to work peers and stop local work generation
			distributed->cancel ();
		}
	}
	items_by_root.erase (existing.first, existing.second);
}

void nano::distributed_work_manager::stop ()
{
	if (!stopped.exchange (true))
	{
		// Cancel any ongoing work
		std::unordered_set<nano::root> roots_l;
		nano::unique_lock<std::mutex> lock_l (mutex);
		for (auto const & item_l : items.get<tag_root> ())
		{
			roots_l.insert (item_l.root);
		}
		lock_l.unlock ();
		for (auto const & root_l : roots_l)
		{
			cancel (root_l, true);
		}
		lock_l.lock ();
		items.clear ();
	}
}

size_t nano::distributed_work_manager::size () const
{
	nano::lock_guard<std::mutex> guard_l (mutex);
	return items.get<tag_id> ().size ();
}

std::unique_ptr<nano::container_info_component> nano::collect_container_info (distributed_work_manager & distributed_work, const std::string & name)
{
	auto item_count = distributed_work.size ();
	auto sizeof_item_element = sizeof (decltype (nano::distributed_work_manager::items)::value_type);
	auto composite = std::make_unique<container_info_composite> (name);
	composite->add_component (std::make_unique<container_info_leaf> (container_info{ "items", item_count, sizeof_item_element }));
	return composite;
}
