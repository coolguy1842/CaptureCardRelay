#include <chrono>
#include <frame_limiter.hpp>
#include <thread>

SDL_Time getNanoseconds() {
    SDL_Time nanos;
    if(!SDL_GetCurrentTime(&nanos)) {
        // fallback nanos function
        nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    return nanos;
}

FrameLimiter::FrameLimiter(bool useEarly)
    : m_useEarly(useEarly)
    , m_frameTimeStart(getNanoseconds()) {
    setFPSLimit(0);

#ifdef DEBUG
    frameTime();
#endif
}

bool FrameLimiter::active() const { return m_active; }
bool FrameLimiter::useEarly() const { return m_useEarly; }

#ifdef DEBUG
float FrameLimiter::frameTime() {
    SDL_Time frameTimeEnd = getNanoseconds();
    SDL_Time frameTime    = frameTimeEnd - m_frameTimeStart;

    float ms = frameTime / 1e+6;

    if(m_timings.average == -1) {
        m_timings = {
            .average = ms,
            .min     = ms,
            .max     = ms,
        };
    }
    else {
        // use rolling average: https://stackoverflow.com/questions/12636613/how-to-calculate-moving-average-without-keeping-the-count-and-data-total#comment79377875_23493727
        m_timings = {
            .average = (m_timings.average * (maxFrameTimes - 1) + ms) / maxFrameTimes,
            .min     = SDL_min(m_timings.min, ms),
            .max     = SDL_max(m_timings.max, ms),
        };
    }

    m_secondClock += ms;

    // update frametimes every second
    if(m_secondClock >= 1000.0f) {
        updateStable();
        m_timings.average = -1;
    }

    m_frameTimeStart = getNanoseconds();

    return ms;
}

float FrameLimiter::frameTimeAverage() const { return m_timings.average; }
float FrameLimiter::frameTimeMin() const { return m_timings.min; }
float FrameLimiter::frameTimeMax() const { return m_timings.max; }

float FrameLimiter::stableFrameTimeAverage() const { return m_stableTimings.average; }
float FrameLimiter::stableFrameTimeMin() const { return m_stableTimings.min; }
float FrameLimiter::stableFrameTimeMax() const { return m_stableTimings.max; }

bool FrameLimiter::hasStableChanged() {
    if(m_stableChanged) {
        m_stableChanged = false;
        return true;
    }

    return false;
}

void FrameLimiter::updateStable() {
    m_stableTimings   = m_timings;
    m_timings.average = -1;

    m_secondClock   = 0.0f;
    m_stableChanged = true;
}

#endif

void FrameLimiter::setFPSLimit(float fps) {
    const int64_t new_target = (fps <= 0.0f) ? 0 : static_cast<int64_t>(1'000'000'000.0f / fps);
    if(m_target == new_target) {
        return;
    }

    m_target = new_target;
    m_active = (new_target > 0);
}

void FrameLimiter::setUseEarly(bool useEarly) {
    m_useEarly = useEarly;
}

void FrameLimiter::limit(bool isEarly) {
    if(!m_active || m_target <= 0) {
        return;
    }
    else if(isEarly != m_useEarly) {
        return;
    }

    m_frameStart       = getNanoseconds();
    int64_t sleep_time = calculateSleepTime(m_frameStart, m_frameEnd);
    if(sleep_time > 0) {
        doSleep(sleep_time);
    }

    m_frameEnd = getNanoseconds();
}

SDL_Time FrameLimiter::calculateSleepTime(SDL_Time start, SDL_Time end) {
    if(m_target <= 0 || start <= 0) {
        return 0;
    }

    SDL_Time work  = SDL_max(start - end, 0);
    SDL_Time sleep = (m_target - work) - m_overhead;

    return sleep > 0 ? sleep : 0;
}

void FrameLimiter::doSleep(SDL_Time sleepTime) {
    if(sleepTime <= 0) {
        return;
    }

    int64_t before = getNanoseconds();
    std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));

    int64_t over = (getNanoseconds() - before) - sleepTime;
    if(over < 0 || over > (m_target / 2)) {
        over = 0;
    }

    m_overhead = over;
}