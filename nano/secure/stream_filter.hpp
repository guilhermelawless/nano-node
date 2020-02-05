#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/lib/stream.hpp>
#include <nano/secure/buffer.hpp>

#include <crypto/cryptopp/seckey.h>
#include <crypto/cryptopp/siphash.h>

#include <mutex>
#include <unordered_set>

namespace nano
{
/**
 * A probabilistic duplicate filter using SipHash 2/4/128
 * The probability of false negatives (unique packet marked as duplicate) is zero.
 * The probability of false positives (duplicate packet marked as unique) is marginal but not zero.
 */
class stream_filter final
{
public:
	stream_filter () = delete;
	stream_filter (size_t capacity_a);
	/**
	 * Reads \p stream_a and inserts its siphash digest in the filter.
	 * @note the stream is rewinded back to its position before the function was called
	 * @remark error_a is set to true if an exception occurs when reading the stream, and rewinding is not performed
	 * @return true if the contents were already in the filter. This means it is *most likely* a duplicate
	 **/
	bool operator() (bool & error_a, nano::stream & stream_a);

	/** Sets every element to zero */
	void clear ();

private:
	using siphash_t = CryptoPP::SipHash<2, 4, true>;
	using item_key_t = nano::uint128_t;

	/**
	 * Reads \p stream_a until end of stream and hashes the resulting contents.
	 * Then, \p stream_a is rewinded back to its position before the function was called.
	 * @remark error_a is set to true if an exception occurs when reading the stream, and rewinding is not performed
	 * @return the siphash digest of the contents in \p stream_a
	 **/
	item_key_t hash (bool & error_a, nano::stream & stream_a) const;

	size_t capacity;
	std::vector<item_key_t> items;
	CryptoPP::SecByteBlock key{ siphash_t::KEYLENGTH };
	std::mutex mutex;
};
}
