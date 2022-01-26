#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION_STREAM(...) LogDuration UNIQUE_VAR_NAME_PROFILE(__VA_ARGS__)

int UNIQUE_VAR_NAME_PROFILE;

class LogDuration {
public:
	using Clock = std::chrono::steady_clock;

	LogDuration(std::string s, std::ostream& os = std::cerr)
		: name_operation_(s), out_(os) {}

	~LogDuration() {
		using namespace std::chrono;
		using namespace std::literals;

		const auto end_time = Clock::now();
		const auto dur = end_time - start_time_;
		out_ << name_operation_ << ": "s <<
			duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
	}

private:
	const Clock::time_point start_time_ = Clock::now();
	std::string name_operation_;
	std::ostream& out_;
};