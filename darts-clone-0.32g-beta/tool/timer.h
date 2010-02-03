#ifndef DARTS_CLONE_TIMER_H
#define DARTS_CLONE_TIMER_H

#include <ctime>

namespace Darts {

class Timer
{
public:
	Timer() : cl_(std::clock()) {}

	double elapsed() const
	{ return 1.0 * (std::clock() - cl_) / CLOCKS_PER_SEC; }

	void reset() { cl_ = std::clock(); }

private:
	std::clock_t cl_;

	// Disallows copies.
	Timer(const Timer &);
	Timer &operator=(const Timer &);
};

}  // namespace Darts

#endif  // DARTS_CLONE_TIMER_H
