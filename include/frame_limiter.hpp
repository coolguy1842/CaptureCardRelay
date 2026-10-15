#pragma once
#include <SDL3/SDL.h>

// original: https://github.com/flightlessmango/MangoHud/blob/master/mangohud-next/client/fps_limiter.h
class FrameLimiter {
public:
    explicit FrameLimiter(bool useEarly);

    [[nodiscard]] bool active() const;
    [[nodiscard]] bool useEarly() const;

#ifdef DEBUG
    // returns ms as float
    float frameTime();

    // rolling max, min over past 200 frame times
    [[nodiscard]] float stableFrameTimeAverage() const;
    [[nodiscard]] float stableFrameTimeMin() const;
    [[nodiscard]] float stableFrameTimeMax() const;

    [[nodiscard]] float frameTimeAverage() const;
    [[nodiscard]] float frameTimeMin() const;
    [[nodiscard]] float frameTimeMax() const;

    // updates the stable variables e.g min, max & frame time
    void updateStable();
    bool hasStableChanged();
#endif

    void setFPSLimit(float fps);
    void setUseEarly(bool useEarly);

    void limit(bool isEarly);

private:
    bool m_useEarly;
    bool m_active = false;

    SDL_Time m_target     = 0;
    SDL_Time m_overhead   = 0;
    SDL_Time m_frameStart = 0;
    SDL_Time m_frameEnd   = 0;

    SDL_Time m_frameTimeStart = 0;

#ifdef DEBUG
    const static size_t maxFrameTimes = 100;

    struct Timings {
        float average = -1.0f;
        float min     = -1;
        float max     = -1;
    };

    // prevent div by 0
    Timings m_stableTimings;
    Timings m_timings;

    float m_secondClock  = 0;
    bool m_stableChanged = false;
#endif

    [[nodiscard]] SDL_Time calculateSleepTime(SDL_Time start, SDL_Time end) const;
    void doSleep(SDL_Time sleepTime);
};
