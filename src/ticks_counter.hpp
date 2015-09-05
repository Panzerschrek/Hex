#pragma once
#include <ctime>

class h_TicksCounter
{
public:
	h_TicksCounter(unsigned int frequency_calc_interval= 1000);
	~h_TicksCounter();

	void Tick();
	unsigned int GetTicksFrequency() const;
	unsigned int GetTotalTicks() const;

private:
	h_TicksCounter& operator=(const h_TicksCounter&)= delete;

private:
	const time_t frequency_calc_interval_;

	unsigned int total_ticks_;
	unsigned int output_ticks_frequency_;
	unsigned int current_sample_ticks_;

	time_t last_update_time_;
};
