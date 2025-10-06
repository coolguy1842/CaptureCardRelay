#include <application.hpp>

void Application::render() {
    SDL_SetRenderDrawColor(m_renderData.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderData.renderer);

    Clay_BeginLayout();

    // clang-format off
    CLAY(
        CLAY_ID("Body"),
        {
            .layout = {
                .sizing = {
                    CLAY_SIZING_PERCENT(1.0),
                    CLAY_SIZING_PERCENT(1.0)
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16
            },
            .backgroundColor = { 0, 0, 0,255 },
            .custom = { .customData = m_cameraData.get() }
        }
    ) {
        if(!m_status.text.empty()) {
            // 0 on start
            const float height = Clay_GetElementData(CLAY_ID("Status")).boundingBox.height;
            const float offset = -8.0f;

            CLAY(
                CLAY_ID("Status"),
                {
                    .layout = {
                        .padding = CLAY_PADDING_ALL(8)
                    },
                    .backgroundColor = { 0, 0, 0, 0xAF },
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                    .floating = {
                        .offset = {
                            offset,
                            height + (m_status.animationProgress * (-height + offset))
                        },
                        .parentId = CLAY_ID("Body").id,
                        // if height isnt set yet then hide
                        .zIndex = static_cast<int16_t>(height <= 0.0f ? -1 : 1),
                        .attachPoints = {
                            .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                            .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM
                        },
                        .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID
                    }
                }
            ) {
                Clay__OpenTextElement(
                    {
                        .isStaticallyAllocated = false,
                        .length = static_cast<int32_t>(m_status.text.size()),
                        .chars = m_status.text.c_str()
                    },
                    CLAY_TEXT_CONFIG({
                        .textColor = { 255, 255, 255, 255 },
                        .fontSize = 24
                    })
                );
            }
        }
    }
    // clang-format on

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();
    SDL_Clay_RenderClayCommands(&m_renderData, &renderCommands);

    SDL_RenderPresent(m_renderData.renderer);
}