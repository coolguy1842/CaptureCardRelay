#include <application.hpp>
#include <format>

bool Application::Clay_MouseClicked() { return Clay_GetPointerState().state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME; }
bool Application::Clay_MouseHeld() {
    switch(Clay_GetPointerState().state) {
    case CLAY_POINTER_DATA_PRESSED_THIS_FRAME:
    case CLAY_POINTER_DATA_PRESSED:            return true;
    default:                                   return false;
    }
}

bool Application::Clay_MouseReleasedNow() { return Clay_GetPointerState().state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME; }
bool Application::Clay_MouseReleased() { return !Clay_MouseHeld(); }

bool Application::Clay_DirectlyClicked() {
    if(!Clay_MouseClicked()) {
        return false;
    }

    Clay_ElementIdArray ids = Clay_GetPointerOverIds();
    return ids.length != 0 && ids.internalArray[ids.length - 1].id == Clay_GetOpenElementId();
}

Clay_RenderCommandArray Application::buildUI() {
    Clay_BeginLayout();
    m_nextCursor = m_defaultCursor;

    CLAY(
        CLAY_ID("Body"),
        {
            .layout = { .sizing = { CLAY_SIZING_PERCENT(1.0), CLAY_SIZING_PERCENT(1.0) } },
            .custom = { .customData = m_cameraData.get() },
        }
    ) {
#ifdef DEBUG
        if(m_showFrametime) {
            CLAY_AUTO_ID({
                .layout          = { .padding = CLAY_PADDING_ALL(8) },
                .backgroundColor = { 0x00, 0x00, 0x00, 0xAF },
                .cornerRadius    = CLAY_CORNER_RADIUS(6),

                .floating = {
                    .offset       = { .x = 16, .y = 16 },
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                        .parent  = CLAY_ATTACH_POINT_LEFT_TOP,
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }) {
                if(m_frameTimeText.empty() || m_frameLimiter.willFrameTimeRollover()) {
                    float stableTime = m_frameLimiter.stableFrameTime();

                    m_frameTimeText = std::format(
                        "FPS: {}  |  {:0.2}ms\nmin: {:0.2}ms\nmax: {:0.2}ms",
                        static_cast<int64_t>(1000 / stableTime), stableTime,
                        m_frameLimiter.frameTimeMin(),
                        m_frameLimiter.frameTimeMax()
                    );
                }

                Clay_String str = { .isStaticallyAllocated = false, .length = static_cast<int32_t>(m_frameTimeText.size()), .chars = m_frameTimeText.c_str() };
                CLAY_TEXT(str, defaultTextConfig);
            }
        }

        m_frameLimiter.frameTime();
#endif

        BuildSettingsMenu();
        BuildStatus();
    }

    static uint64_t prev = SDL_GetPerformanceCounter();
    uint64_t now         = SDL_GetPerformanceCounter();

    float deltaTime = (now - prev) / static_cast<float>(SDL_GetPerformanceFrequency());
    prev            = now;

    if(m_currentCursor != m_nextCursor) {
        m_currentCursor = m_nextCursor;
        SDL_SetCursor(m_currentCursor);
    }

    return Clay_EndLayout(deltaTime);
}

void Application::updateUI() {
    Clay_SetPointerState(Clay_Vector2{ .x = m_cursorX, .y = m_cursorY }, m_mouseHeld);

    static uint64_t prev = SDL_GetPerformanceCounter();
    uint64_t now         = SDL_GetPerformanceCounter();

    float deltaTime = static_cast<double>(((now - prev) * 1000 / static_cast<float>(SDL_GetPerformanceFrequency())));
    prev            = now;

    Clay_UpdateScrollContainers(!(m_slidingVolume || m_slidingFPS || m_currentScrollBar != 0), Clay_Vector2{ m_mouseWheelX, m_mouseWheelY }, deltaTime);
    m_mouseWheelX = 0.0f;
    m_mouseWheelY = 0.0f;

    updateSettingsUI();
}