#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/lib/stream.hpp>
#include <nano/secure/buffer.hpp>

#include <crypto/cryptopp/seckey.h>
#include <crypto/cryptopp/siphash.h>

#include <mutex>

namespace nano
{
/**
 * A probabilistic duplicate filter based on directed map caches, using SipHash 2/4/128
 * The probability of false negatives (unique packet marked as duplicate) is the probability of a 128-bit SipHash collision.
 * The probability of false positives (duplicate packet marked as unique) shrinks with a larger filter.
 */
class stream_filter final
{
public:
	stream_filter () = delete;
	stream_filter (size_t capacity_a);
	/**
	 * Reads \p stream_a and inserts its siphash digest in the filter.
	 * @note \p stream_a is rewinded back to its position before the function was called.
	 * @remark \p error_a is set to true if an exception occurs when reading \p stream_a, and rewinding is not performed.
	 * @return a boolean representing the previous existance of \p stream_a contents in the filter, or false if \p error_a was set.
	 **/
	bool operator() (bool & error_a, nano::stream & stream_a);

	/** Sets every element of the filter to zero, keeping its size and capacity. */
	void clear ();

private:
	using siphash_t = CryptoPP::SipHash<2, 4, true>;
	using item_key_t = nano::uint128_t;

	/**
	 * Reads \p stream_a until end of stream and hashes the resulting contents.
	 * Then, \p stream_a is rewinded back to its position before the function was called.
	 * @remark \p error_a is set to true if an exception occurs when reading \p stream_a, and rewinding is not performed.
	 * @return the siphash digest of the contents in \p stream_a .
	 **/
	item_key_t hash (bool & error_a, nano::stream & stream_a) const;

	size_t capacity;
	std::vector<item_key_t> items;
	CryptoPP::SecByteBlock key{ siphash_t::KEYLENGTH };
	std::mutex mutex;
};
}
