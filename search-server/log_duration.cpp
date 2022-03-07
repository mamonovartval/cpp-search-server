#include "log_duration.h"

LogDuration::LogDuration(const std::string& s, std::ostream& os)
	: name_operation_(s), out_(os) {}

LogDuration::~LogDuration() {
	using namespace std::chrono;
	using namespace std::literals;

	const auto end_time = Clock::now();
	const auto dur = end_time - start_time_;
	out_ << name_operation_ << ": "s <<
		duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
}
