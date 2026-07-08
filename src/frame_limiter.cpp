#include <SDL3/SDL_stdinc.h>
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

    // just fake 60hz for startup
    m_frameTimeStable = m_frameTimeMax = m_frameTimeMin = 16.6;
#endif
}

bool FrameLimiter::active() const { return m_active; }
bool FrameLimiter::useEarly() const { return m_useEarly; }

#ifdef DEBUG
float FrameLimiter::frameTime() {
    SDL_Time frameTimeEnd = getNanoseconds();
    SDL_Time frameTime    = frameTimeEnd - m_frameTimeStart;

    m_frameTimeStart = getNanoseconds();
    float ms         = frameTime / 1e+6;

    if(++m_frameTimeNum >= maxFrameTimes) {
        m_frameTimeMin = m_frameTimes[0];
        m_frameTimeMax = m_frameTimes[0];

        float totalFrameTime = 0.0f;
        for(size_t i = 0; i <= maxFrameTimes; i++) {
            const float& time = m_frameTimes[i];

            frameTime += time;
            m_frameTimeMin = SDL_min(time, m_frameTimeMin);
            m_frameTimeMax = SDL_max(time, m_frameTimeMax);
        }

        m_frameTimeStable = totalFrameTime / maxFrameTimes;
        m_frameTimeNum    = 0;
    }

    m_frameTimes[m_frameTimeNum] = ms;
    return ms;
}

float FrameLimiter::frameTimeMin() const { return m_frameTimeMin; }
float FrameLimiter::frameTimeMax() const { return m_frameTimeMax; }
float FrameLimiter::stableFrameTime() const { return m_frameTimeStable; }
bool FrameLimiter::willFrameTimeRollover() const { return m_frameTimeNum >= maxFrameTimes - 1; }
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