#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/node/common.hpp>
#include <nano/node/distributed_work.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional/optional.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mi = boost::multi_index;

namespace nano
{
class node;
class distributed_work;
class root;
class container_info_component;

class distributed_work_manager final : public std::enable_shared_from_this<distributed_work_manager>
{
public:
	distributed_work_manager (nano::node &);
	~distributed_work_manager ();
	bool make (nano::work_version const, nano::root const &, std::vector<std::pair<std::string, uint16_t>> const &, uint64_t, std::function<void(boost::optional<uint64_t>)> const &, boost::optional<nano::account> const & = boost::none);
	bool make (std::chrono::seconds const &, nano::work_request const &);
	void cancel (nano::root const &, bool const local_stop = false);
	void stop ();
	size_t size () const;

private:
	struct item_t
	{
		uint64_t identifier;
		nano::block_hash root;
		std::weak_ptr<nano::distributed_work> item;
	};

	// clang-format off
	class tag_root {};
	class tag_id {};

	boost::multi_index_container<item_t,
	mi::indexed_by<
		mi::hashed_non_unique<mi::tag<tag_root>,
			mi::member<item_t, nano::block_hash, &item_t::root>>,
		mi::hashed_unique<mi::tag<tag_id>,
			mi::member<item_t, uint64_t, &item_t::identifier>>>>
	items;
	// clang-format on

	std::atomic<uint64_t> counter{ 0 };
	nano::node & node;
	mutable std::mutex mutex;
	std::atomic<bool> stopped{ false };

	friend std::unique_ptr<container_info_component> collect_container_info (distributed_work_manager &, const std::string &);
};

std::unique_ptr<container_info_component> collect_container_info (distributed_work_manager & distributed_work, const std::string & name);
}
