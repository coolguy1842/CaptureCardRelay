#pragma once
#include <SDL3/SDL.h>

// original: https://github.com/flightlessmango/MangoHud/blob/master/mangohud-next/client/fps_limiter.h
class FrameLimiter {
public:
    FrameLimiter(bool useEarly);

    bool active() const;
    bool useEarly() const;

#ifdef DEBUG
    // returns ms as float
    float frameTime();

    // rolling max, min over past 200 frame times
    float stableFrameTime() const;

    float frameTimeMin() const;
    float frameTimeMax() const;
    bool willFrameTimeRollover() const;
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
    size_t m_frameTimeNum     = 0;

    const static size_t maxFrameTimes = 200;
    float m_frameTimes[maxFrameTimes];

    // prevent div by 0
    float m_frameTimeStable = 1.0f;
    float m_frameTimeMin    = 0.0f;
    float m_frameTimeMax    = 0.0f;

    SDL_Time calculateSleepTime(SDL_Time start, SDL_Time end);
    void doSleep(SDL_Time sleepTime);
};
