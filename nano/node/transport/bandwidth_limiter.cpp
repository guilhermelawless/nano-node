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

	if (message_size > limit / rate_buffer.size () || trended_rate + message_size > limit)
	{
		result = true;
	}
	else
	{
		rate = rate + message_size;
	}
	auto now = std::chrono::steady_clock::now ();
	if (next_trend < now)
	{
		// Normalize in case more time has passed
		rate_buffer.push_back (rate * period / (now - next_trend + period));
		trended_rate = std::accumulate (rate_buffer.begin (), rate_buffer.end (), size_t{ 0 });
		rate = 0;
		next_trend = now + period;
	}
	return result;
}

size_t nano::bandwidth_limiter::get_rate ()
{
	nano::lock_guard<std::mutex> lock (mutex);
	return trended_rate;
}
