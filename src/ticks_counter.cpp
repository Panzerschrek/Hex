#include "ticks_counter.hpp"

h_TicksCounter::h_TicksCounter(unsigned int frequency_calc_interval)
	: frequency_calc_interval_(frequency_calc_interval * CLOCKS_PER_SEC / 1000)
	, total_ticks_(0)
	, output_ticks_frequency_(0)
	, current_sample_ticks_(0)
	, last_update_time_(std::clock())
{
}

h_TicksCounter::~h_TicksCounter()
{
}

void h_TicksCounter::Tick()
{
	total_ticks_++;
	current_sample_ticks_++;

	time_t current_time= std::clock();
	time_t dt= current_time - last_update_time_;

	if( dt >= frequency_calc_interval_ )
	{
		output_ticks_frequency_= current_sample_ticks_ * CLOCKS_PER_SEC / frequency_calc_interval_;
		current_sample_ticks_= 0;
		last_update_time_+= dt;
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
