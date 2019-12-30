#pragma once

#include <nano/node/common.hpp>

#include <boost/circular_buffer.hpp>

namespace nano
{
class bandwidth_limiter final
{
public:
	// initialize with rate 0 = unbounded
	bandwidth_limiter (const size_t);
	bool should_drop (const size_t &);
	size_t get_rate ();

private:
	//last time rate was adjusted
	std::chrono::steady_clock::time_point next_trend;
	//trend rate over 20 poll periods
	boost::circular_buffer<size_t> rate_buffer{ 20, 0 };
	//limit bandwidth to
	const size_t limit;
	//rate, increment if message_size + rate < rate
	size_t rate;
	//trended rate to even out spikes in traffic
	size_t trended_rate;
	std::mutex mutex;
};
}
