#include <nano/crypto_lib/random_pool.hpp>
#include <nano/lib/locks.hpp>
#include <nano/secure/buffer.hpp>
#include <nano/secure/common.hpp>
#include <nano/secure/network_filter.hpp>

nano::network_filter::network_filter (size_t size_a) :
items (size_a, nano::uint128_t{ 0 })
{
	nano::random_pool::generate_block (key, key.size ());
}

bool nano::network_filter::apply (uint8_t const * bytes_a, size_t count_a, nano::uint128_t * digest_a)
{
	// Get hash before locking
	auto digest (hash (bytes_a, count_a));

	nano::lock_guard<std::mutex> lock (mutex);
	auto & element (get_element (digest));
	bool existed (element == digest);
	if (!existed)
	{
		// Replace likely old element with a new one
		element = digest;
	}
	if (digest_a)
	{
		*digest_a = digest;
	}
	return existed;
}

void nano::network_filter::clear (nano::uint128_t const & digest_a)
{
	nano::lock_guard<std::mutex> lock (mutex);
	auto & element (get_element (digest_a));
	if (element == digest_a)
	{
		element = nano::uint128_t{ 0 };
	}
}

void nano::network_filter::clear (uint8_t const * bytes_a, size_t count_a)
{
	clear (hash (bytes_a, count_a));
}

template <typename OBJECT>
void nano::network_filter::clear (OBJECT const & object_a)
{
	std::vector<uint8_t> bytes;
	{
		nano::vectorstream stream (bytes);
		object_a->serialize (stream);
	}
	clear (bytes.data (), bytes.size ());
}

void nano::network_filter::clear ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	items.assign (items.size (), nano::uint128_t{ 0 });
}

nano::uint128_t & nano::network_filter::get_element (nano::uint128_t const & hash_a)
{
	assert (!mutex.try_lock ());
	assert (items.size () > 0);
	size_t index (hash_a % items.size ());
	return items[index];
}

nano::uint128_t nano::network_filter::hash (uint8_t const * bytes_a, size_t count_a) const
{
	nano::uint128_union digest{ 0 };
	CryptoPP::SipHash<2, 4, true> siphash (key, static_cast<unsigned int> (key.size ()));
	siphash.CalculateDigest (digest.bytes.data (), bytes_a, count_a);
	return digest.number ();
}

// Explicitly instantiate
template void nano::network_filter::clear (std::shared_ptr<nano::vote> const &);
template void nano::network_filter::clear (std::shared_ptr<nano::block> const &);
