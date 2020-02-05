#include <nano/crypto_lib/random_pool.hpp>
#include <nano/lib/locks.hpp>
#include <nano/secure/stream_filter.hpp>

nano::stream_filter::stream_filter (size_t capacity_a) :
capacity (capacity_a),
items (capacity_a, item_key_t{ 0 })
{
	nano::random_pool::generate_block (key, key.size ());
}

bool nano::stream_filter::operator() (bool & error_a, nano::stream & stream_a)
{
	bool existed;
	auto digest (hash (error_a, stream_a));
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

nano::stream_filter::item_key_t nano::stream_filter::hash (bool & error_a, nano::stream & stream_a) const
{
	CryptoPP::SipHash<2, 4, true> siphash (key, key.size ());
	nano::uint128_union digest;
	try
	{
		std::array<uint8_t, 8> buffer;
		size_t bytes_read;
		size_t total_read{ 0 };
		do
		{
			bytes_read = stream_a.sgetn (buffer.data (), buffer.size ());
			total_read += bytes_read;
			siphash.Update (buffer.data (), bytes_read);
		} while (bytes_read == buffer.size ());

		// Rewind stream to previous state
		stream_a.pubseekoff (-total_read, std::ios_base::cur);
		siphash.Final (digest.bytes.data ());
	}
	catch (std::runtime_error const &)
	{
		error_a = true;
	}
	return digest.number ();
}

void nano::stream_filter::clear ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	items.assign (capacity, item_key_t{ 0 });
}
