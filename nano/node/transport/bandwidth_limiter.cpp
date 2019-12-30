#include <nano/node/transport/bandwidth_limiter.hpp>

#include <numeric>

using namespace std::chrono_literals;

nano::bandwidth_limiter::bandwidth_limiter (const size_t limit_a) :
next_trend (std::chrono::steady_clock::now () + 50ms),
limit (limit_a),
rate (0),
trended_rate (0)
{
}

bool nano::bandwidth_limiter::should_drop (const size_t & message_size)
{
	bool result (false);
	if (limit == 0) //never drop if limit is 0
	{
		return result;
	}
	nano::lock_guard<std::mutex> lock (mutex);

	if (message_size > limit / rate_buffer.size () || rate + message_size > limit)
	{
		result = true;
	}
	else
	{
		rate = rate + message_size;
	}
	if (next_trend < std::chrono::steady_clock::now ())
	{
		next_trend = std::chrono::steady_clock::now () + 50ms;
		rate_buffer.push_back (rate);
		trended_rate = std::accumulate (rate_buffer.begin (), rate_buffer.end (), size_t{ 0 }) / rate_buffer.size ();
		rate = 0;
	}
	return result;
}

size_t nano::bandwidth_limiter::get_rate ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	return trended_rate;
}
