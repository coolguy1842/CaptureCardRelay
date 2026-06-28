#pragma once
#include <SDL3/SDL_time.h>

// original: https://github.com/flightlessmango/MangoHud/blob/master/mangohud-next/client/fps_limiter.h
class FrameLimiter {
public:
    FrameLimiter(bool useEarly);

    bool active() const;
    bool useEarly() const;

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

    SDL_Time calculateSleepTime(SDL_Time start, SDL_Time end);
    void doSleep(SDL_Time sleepTime);
};
