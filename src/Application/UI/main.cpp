#include <application.hpp>

bool Application::Clay_MouseClicked() { return Clay_GetPointerState().state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME; }
bool Application::Clay_MouseHeld() {
    switch(Clay_GetPointerState().state) {
    case CLAY_POINTER_DATA_PRESSED_THIS_FRAME:
    case CLAY_POINTER_DATA_PRESSED:            return true;
    default:                                   return false;
    }
}

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

    Clay_UpdateScrollContainers(!m_slidingVolume, Clay_Vector2{ m_mouseWheelX, m_mouseWheelY }, deltaTime);
    m_mouseWheelX = 0.0f;
    m_mouseWheelY = 0.0f;

    updateSettingsUI();
}