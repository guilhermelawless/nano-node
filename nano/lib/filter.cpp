#include <nano/lib/filter.hpp>
#include <nano/lib/numbers.hpp>

nano::duplicate_filter::duplicate_filter (size_t capacity_a) :
capacity (capacity_a)
{
}

bool nano::duplicate_filter::insert (nano::block_hash const & hash_a)
{
	bool result = items.get<tag_hash> ().insert (hash_a).second;
	if (result)
	{
		while (items.size () > capacity)
		{
			items.get<tag_sequence> ().pop_front ();
		}
	}
	return result;
}

bool nano::duplicate_filter::erase (nano::block_hash const & hash_a)
{
	bool result = items.get<tag_hash> ().erase (hash_a) > 0;
	return result;
}
