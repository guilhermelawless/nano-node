#pragma once

#include <nano/lib/numbers.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

namespace mi = boost::multi_index;

namespace nano
{
/**
 * A deterministic duplicate filter for nano::block_hash elements.
 * Elements are stored in a sequenced and uniquely hashed container.
 * Elements can be inserted and removed.
 * Once full capacity is reached, the oldest element is removed.
 */
class duplicate_filter final
{
	// clang-format off
	class tag_sequence {};
	class tag_hash {};
	// clang-format on

public:
	duplicate_filter () = delete;
	duplicate_filter (size_t);
	/** Returns true if \p hash_a was already present */
	bool insert (nano::block_hash const & hash_a);
	/** Returns true if \p hash_a was present */
	bool erase (nano::block_hash const & hash_a);

private:
	size_t capacity;

	// clang-format off
	boost::multi_index_container<nano::block_hash,
	mi::indexed_by<
		mi::sequenced<mi::tag<tag_sequence>>,
		mi::hashed_unique<mi::tag<tag_hash>,
			mi::identity<nano::block_hash>>>>
	items;
	// clang-format on
};
}
