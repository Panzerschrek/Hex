#pragma once
#include <chrono>

class h_TicksCounter
{
public:
	h_TicksCounter(unsigned int frequency_calc_interval_ms= 1000);
	~h_TicksCounter();

	void Tick( unsigned int count = 1);
	unsigned int GetTicksFrequency() const;
	unsigned int GetTotalTicks() const;

private:
	h_TicksCounter& operator=(const h_TicksCounter&)= delete;

private:
	const uint64_t frequency_calc_interval_ms_;

	unsigned int total_ticks_;
	unsigned int output_ticks_frequency_;
	unsigned int current_sample_ticks_;

	uint64_t last_update_time_ms_;
};
