#include "time.hpp"
#include "ticks_counter.hpp"

h_TicksCounter::h_TicksCounter(unsigned int frequency_calc_interval_ms)
	: frequency_calc_interval_ms_(frequency_calc_interval_ms)
	, total_ticks_(0)
	, output_ticks_frequency_(0)
	, current_sample_ticks_(0)
	, last_update_time_ms_( hGetTimeMS() )
{}

h_TicksCounter::~h_TicksCounter()
{}

void h_TicksCounter::Tick( unsigned int count )
{
	total_ticks_+= count;
	current_sample_ticks_+= count;

	uint64_t current_time_ms= hGetTimeMS();
	uint64_t dt_ms= current_time_ms - last_update_time_ms_;

	if( dt_ms >= frequency_calc_interval_ms_ )
	{
		output_ticks_frequency_= current_sample_ticks_ * 1000 / frequency_calc_interval_ms_;
		current_sample_ticks_= 0;
		last_update_time_ms_+= dt_ms;
	}
}

unsigned int h_TicksCounter::GetTicksFrequency() const
{
	return output_ticks_frequency_;
}

unsigned int h_TicksCounter::GetTotalTicks() const
{
	return total_ticks_;
}
