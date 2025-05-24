/* Copyright (c) 2023-2024, Thomas Atkinson
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <unordered_map>

#ifdef TRACY_ENABLE
	#include <tracy/Tracy.hpp>
#endif

#ifdef TRACY_ENABLE
	void* operator new(size_t count);
	void operator delete(void* ptr) noexcept;

	#define PROFILE_SCOPE(name) ZoneScopedN(name)

	#define PROFILE_FUNCTION() ZoneScoped
#else
	#define PROFILE_SCOPE(name)
	#define PROFILE_FUNCTION()
#endif

enum class PlotType {
    eNumber = 0,
    ePercentage = 1,
    eMemory = 2
};

#ifdef TRACY_ENABLE
namespace {
    inline tracy::PlotFormatType toTracyPlotFormat(PlotType type) {
        switch (type) {
        case PlotType::eNumber:     return tracy::PlotFormatType::Number;
        case PlotType::ePercentage: return tracy::PlotFormatType::Percentage;
        case PlotType::eMemory:     return tracy::PlotFormatType::Memory;
        default:                    return tracy::PlotFormatType::Number;
        }
    }
}

	#define TO_TRACY_PLOT_FORMAT(name) toTracyPlotFormat(name)
#else
	#define TO_TRACY_PLOT_FORMAT(name)
#endif

template <typename T, PlotType PT = PlotType::eNumber>
class Plot {
public:
    static void plot(const char* name, T value) {
        auto* instance = getInstance();
        instance->m_plots[name] = value;
        updateTracyPlot(name, value);
    }

    static void increment(const char* name, T amount) {
        auto* instance = getInstance();
        instance->m_plots[name] += amount;
        updateTracyPlot(name, instance->m_plots[name]);
    }

    static void decrement(const char* name, T amount) {
        auto* instance = getInstance();
        instance->m_plots[name] -= amount;
        updateTracyPlot(name, instance->m_plots[name]);
    }

    static void reset(const char* name) {
        auto* instance = getInstance();
        instance->m_plots[name] = T{};
        updateTracyPlot(name, instance->m_plots[name]);
    }

private:
    static void updateTracyPlot(const char* name, T value) {
#ifdef TRACY_ENABLE
        TracyPlot(name, value);
        TracyPlotConfig(name, TO_TRACY_PLOT_FORMAT(PT), true, true, 0);
#endif
    }

    static Plot* getInstance()
    {
        static_assert(
            (std::is_same<T, int64_t>::value ||
             std::is_same<T, double>::value ||
             std::is_same<T, float>::value),
            "PlotStore only supports int64_t, double and float"
            );
        static Plot instance;
        return &instance;
    }

    std::unordered_map<const char*, T> m_plots;
};