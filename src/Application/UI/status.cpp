#include <application.hpp>

// pretty hacky, but allows knowing when the animation ends to stop rendering clay when not needed
static bool s_animationActive = false;

#define LERP(from, to, mix) (from + (to - from) * mix)
bool EaseOut(Clay_TransitionCallbackArguments arguments) {
    float ratio = 1;
    if(arguments.duration > 0) {
        ratio = CLAY__MIN(arguments.elapsedTime / arguments.duration, 1);
    }

    float inverse    = 1.0f - ratio;
    float lerpAmount = 1.0f - (inverse * inverse * inverse);
    if(arguments.properties & CLAY_TRANSITION_PROPERTY_Y) {
        arguments.current->boundingBox.y = LERP(arguments.initial.boundingBox.y, arguments.target.boundingBox.y, lerpAmount);
    }

    if(arguments.transitionState == CLAY_TRANSITION_STATE_EXITING && ratio >= 1) {
        s_animationActive = false;
    }

    return ratio >= 1;
}

Clay_TransitionData EnterExitSlide(Clay_TransitionData initialState, Clay_TransitionProperty properties) {
    Clay_TransitionData targetState = initialState;
    if(properties & CLAY_TRANSITION_PROPERTY_Y) {
        targetState.boundingBox.y += targetState.boundingBox.height;
    }

    return targetState;
}

void Application::BuildStatus() {
    if(!m_status.expire.has_value()) {
        m_statusActive = s_animationActive;

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
                .handler    = EaseOut,
                .duration   = 0.5f,
                .properties = CLAY_TRANSITION_PROPERTY_Y,
                .enter      = { .setInitialState = EnterExitSlide },
                .exit       = { .setFinalState = EnterExitSlide },
            },
        }
    ) {
        s_animationActive = true;

        Clay_String statusStr = { .isStaticallyAllocated = false, .length = static_cast<int32_t>(m_status.text.size()), .chars = m_status.text.c_str() };
        CLAY_TEXT(statusStr, defaultTextConfig);
    }
}