#pragma once

#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION_STREAM(...) LogDuration int(__VA_ARGS__)

class LogDuration {
public:
	using Clock = std::chrono::steady_clock;

	LogDuration(const std::string& s, std::ostream& os);

	~LogDuration();

private:
	const Clock::time_point start_time_ = Clock::now();
	std::string name_operation_;
	std::ostream& out_;
};
