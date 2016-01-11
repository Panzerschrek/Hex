#include <chrono>
#include <thread>

#include "time.hpp"

uint64_t hGetTimeMS()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void hSleep( unsigned int ms )
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
