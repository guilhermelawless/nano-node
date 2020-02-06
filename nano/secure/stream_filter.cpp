#include <nano/crypto_lib/random_pool.hpp>
#include <nano/lib/locks.hpp>
#include <nano/secure/stream_filter.hpp>

nano::stream_filter::stream_filter (size_t capacity_a) :
capacity (capacity_a),
items (capacity_a, item_key_t{ 0 })
{
	nano::random_pool::generate_block (key, key.size ());
}

bool nano::stream_filter::operator() (bool & error_a, nano::stream & stream_a, size_t count_a)
{
	bool existed{ false };
	auto digest (hash (error_a, stream_a, count_a));
	if (!error_a)
	{
		nano::lock_guard<std::mutex> lock (mutex);
		assert (capacity > 0 && items.size () == capacity);
		size_t index (digest % items.size ());
		auto & element (items[index]);
		existed = element == digest;
		if (!existed)
		{
			// Replace likely old element with a new one
			element = digest;
		}
	}
	return existed;
}

nano::stream_filter::item_key_t nano::stream_filter::hash (bool & error_a, nano::stream & stream_a, size_t count_a) const
{
	nano::uint128_union digest{ 0 };
	uint8_t buffer[count_a];
	try
	{
		auto amount_read (stream_a.sgetn (buffer, count_a));
		error_a = amount_read != count_a;
		// Rewind stream to previous state
		stream_a.pubseekoff (-amount_read, std::ios_base::cur);
	}
	catch (std::runtime_error const &)
	{
		error_a = true;
	}
	if (!error_a)
	{
		CryptoPP::SipHash<2, 4, true> siphash (key, key.size ());
		siphash.CalculateDigest (digest.bytes.data (), buffer, count_a);
	}
	return digest.number ();
}

void nano::stream_filter::clear ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	items.assign (capacity, item_key_t{ 0 });
}
