#pragma once

#include <chrono>

namespace frame {
	class Timer {
	public:
		using Seconds = std::ratio<1>;
		using Milliseconds = std::ratio<1, 1000>;
		using Microseconds = std::ratio<1, 1000000>;
		using Nanoseconds = std::ratio<1, 1000000000>;

		using Clock = std::chrono::steady_clock;
		using DefaultResolution = Seconds;

		Timer() :
			start_time{ Clock::now() },
			previous_tick{ Clock::now() }
		{
		}

		virtual ~Timer() = default;

		void start() {
			if (!running)
			{
				running = true;
				start_time = Clock::now();
			}
		}

		void lap() {
			lapping = true;
			lap_time = Clock::now();
		}

		template <typename T = DefaultResolution>
		double stop() {
			if (!running) {
				return 0;
			}

			running = false;
			lapping = false;
			auto duration = std::chrono::duration<double, T>(Clock::now() - start_time);
			start_time = Clock::now();
			lap_time = Clock::now();

			return duration.count();
		}

		template <typename T = DefaultResolution>
		double elapsed() {
			if (!running) {
				return 0;
			}

			Clock::time_point start = start_time;

			if (lapping) {
				start = lap_time;
			}

			return std::chrono::duration<double, T>(Clock::now() - start).count();
		}

		template <typename T = DefaultResolution>
		double tick() {
			auto now = Clock::now();
			auto duration = std::chrono::duration<double, T>(now - previous_tick);
			previous_tick = now;
			return duration.count();
		}

		bool is_running() const { return running; }

	private:
		bool running{ false };

		bool lapping{ false };

		Clock::time_point start_time;

		Clock::time_point lap_time;

		Clock::time_point previous_tick;
	};
}
