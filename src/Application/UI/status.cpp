#include <application.hpp>

Clay_TransitionData EnterExitSlide(Clay_TransitionData initialState, Clay_TransitionProperty properties) {
    Clay_TransitionData targetState = initialState;
    if(properties & CLAY_TRANSITION_PROPERTY_Y) {
        targetState.boundingBox.y += targetState.boundingBox.height;
    }

    return targetState;
}

void Application::BuildStatus() {
    if(!m_status.expire.has_value()) {
        return;
    }
    // minimize system clock checks
    else if(m_status.expire <= std::chrono::system_clock::now()) {
        m_status.expire = std::nullopt;
        return;
    }

    CLAY(
        CLAY_ID("Status"),
        {
            .layout          = { .padding = CLAY_PADDING_ALL(8) },
            .backgroundColor = { 0x00, 0x00, 0x00, 0xAF },
            .cornerRadius    = CLAY_CORNER_RADIUS(6),

            .floating = {
                .attachPoints = { .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM },
                .attachTo     = CLAY_ATTACH_TO_PARENT,
            },
            // animate show and hide
            .transition = {
                .handler    = Clay_EaseOut,
                .duration   = 0.5f,
                .properties = static_cast<Clay_TransitionProperty>(CLAY_TRANSITION_PROPERTY_Y),
                .enter      = { .setInitialState = EnterExitSlide },
                .exit       = { .setFinalState = EnterExitSlide },
            },
        }
    ) {
        Clay_String statusStr = { .isStaticallyAllocated = false, .length = static_cast<int32_t>(m_status.text.size()), .chars = m_status.text.c_str() };
        CLAY_TEXT(statusStr, defaultTextConfig);
    }
}