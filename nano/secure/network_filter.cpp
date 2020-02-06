#include <nano/crypto_lib/random_pool.hpp>
#include <nano/lib/locks.hpp>
#include <nano/secure/network_filter.hpp>

nano::network_filter::network_filter (size_t size_a) :
items (size_a, item_key_t{ 0 })
{
	nano::random_pool::generate_block (key, key.size ());
}

bool nano::network_filter::apply (uint8_t const * bytes_a, size_t count_a)
{
	// Get hash before locking
	auto digest (hash (bytes_a, count_a));

	nano::lock_guard<std::mutex> lock (mutex);
	assert (items.size () > 0);
	size_t index (digest % items.size ());
	auto & element (items[index]);
	bool existed (element == digest);
	if (!existed)
	{
		// Replace likely old element with a new one
		element = digest;
	}
	return existed;
}

nano::network_filter::item_key_t nano::network_filter::hash (uint8_t const * bytes_a, size_t count_a) const
{
	nano::uint128_union digest{ 0 };
	CryptoPP::SipHash<2, 4, true> siphash (key, static_cast<unsigned int> (key.size ()));
	siphash.CalculateDigest (digest.bytes.data (), bytes_a, count_a);
	return digest.number ();
}

void nano::network_filter::clear ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	items.assign (items.size (), item_key_t{ 0 });
}
