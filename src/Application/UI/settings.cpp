#include "settings.hpp"
#include <algorithm>
#include <application.hpp>
#include <clay_renderer_SDL3.hpp>

#define SEPARATOR CLAY_AUTO_ID({                                                                      \
    .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(2) } }, \
    .backgroundColor = { 0xAF, 0xAF, 0xAF, 0xFF },                                                    \
})

#define HSPACER(pixels) CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_FIXED(pixels) } } })
#define VSPACER(pixels) CLAY_AUTO_ID({ .layout = { .sizing = { .height = CLAY_SIZING_FIXED(pixels) } } })

#define CONTAINER_TITLE(title)                                                                                                                            \
    CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() }, .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER } } }) { \
        CLAY_TEXT(CLAY_STRING(title), defaultTextConfig);                                                                                                 \
    }

#define GROWGROW { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() }

static const Clay_ElementId camerasContainerID          = CLAY_ID("CamerasContainer");
static const Clay_ElementId pixelFormatContainerID      = CLAY_ID("PixelFormatContainer");
static const Clay_ElementId recordingDevicesContainerID = CLAY_ID("RecordingDevicesContainer");
static const Clay_ElementId displayModeContainerID      = CLAY_ID("DisplayModeContainer");
static const Clay_ElementId frameLimitTypeContainerID   = CLAY_ID("FrameLimitTypeContainer");

const Clay_String toClayString(const char* str) {
    if(str == NULL) {
        return { .isStaticallyAllocated = true, .length = 7, .chars = "(null)" };
    }

    return { .isStaticallyAllocated = true, .length = static_cast<int32_t>(strlen(str)), .chars = str };
}

Clay_String cameraPixelFormatStr(PixelFormat mode) {
    switch(mode) {
    case PIXEL_FORMAT_CAMERA: return toClayString("Camera");
    case PIXEL_FORMAT_RGB24:  return toClayString("RGB24");
    default:                  return toClayString("Unknown");
    }
}

Clay_String displayModeStr(CameraDisplayMode mode) {
    switch(mode) {
    case DISPLAY_MODE_CONTAIN: return toClayString("Contain");
    case DISPLAY_MODE_COVER:   return toClayString("Cover");
    case DISPLAY_MODE_FILL:    return toClayString("Fill");
    case DISPLAY_MODE_NONE:    return toClayString("None");
    default:                   return toClayString("Unknown");
    }
}

Clay_String frameLimitTypeStr(FrameLimitType type) {
    switch(type) {
    case FrameLimitType::FRAME_LIMIT_CAMERA:         return toClayString("Camera");
    case FrameLimitType::FRAME_LIMIT_VSYNC:          return toClayString("VSync");
    case FrameLimitType::FRAME_LIMIT_VSYNC_ADAPTIVE: return toClayString("VSync Adaptive");
    case FrameLimitType::FRAME_LIMIT_FPS:            return toClayString("FPS");
    case FrameLimitType::FRAME_LIMIT_NONE:           return toClayString("None");
    default:                                         return toClayString("Unknown");
    }
}

#define SettingContainer(id, widthSizing, shouldClip)                                                \
    CLAY_AUTO_ID({                                                                                   \
        .layout = {                                                                                  \
            .padding = CLAY_PADDING_ALL(12),                                                         \
        },                                                                                           \
        .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },                                               \
        .cornerRadius    = CLAY_CORNER_RADIUS(6),                                                    \
        .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) }, \
    })                                                                                               \
    CLAY(                                                                                            \
        id,                                                                                          \
        {                                                                                            \
            .layout = {                                                                              \
                .sizing          = { .width = widthSizing },                                         \
                .childGap        = 6,                                                                \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                               \
            },                                                                                       \
            .clip = { .vertical = shouldClip, .childOffset = Clay_GetScrollOffset() },               \
        }                                                                                            \
    )

void Application::Build_ScrollBar(Clay_ElementId id, bool vertical) {
    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(id);
    Clay_ElementData elementData        = Clay_GetElementData(id);
    if(
        !scrollData.found || !elementData.found ||
        (vertical ? scrollData.scrollContainerDimensions.height >= scrollData.contentDimensions.height : scrollData.scrollContainerDimensions.width >= scrollData.contentDimensions.width)
    ) {
        return;
    }

    Clay_FloatingAttachPoints verticalAttach   = { .element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_TOP };
    Clay_FloatingAttachPoints horizontalAttach = { .element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM };

    CLAY_AUTO_ID(
        {
            .layout = {
                .sizing = {
                    .width  = vertical ? CLAY_SIZING_FIXED(6) : CLAY_SIZING_FIXED(scrollData.scrollContainerDimensions.width),
                    .height = vertical ? CLAY_SIZING_FIXED(scrollData.scrollContainerDimensions.height) : CLAY_SIZING_FIXED(6),
                },
            },
            .floating = {
                .offset = {
                    .x = vertical ? 0 : -(scrollData.scrollPosition->x / scrollData.contentDimensions.width) * scrollData.scrollContainerDimensions.width,
                    .y = vertical ? -(scrollData.scrollPosition->y / scrollData.contentDimensions.height) * scrollData.scrollContainerDimensions.height : 0,
                },
                .parentId     = id.id,
                .zIndex       = 1,
                .attachPoints = vertical ? verticalAttach : horizontalAttach,
                .attachTo     = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
            },
        }
    ) {
        bool parentHovered = Clay_Hovered();

        float handleWidth  = (scrollData.scrollContainerDimensions.width / scrollData.contentDimensions.width) * scrollData.scrollContainerDimensions.width;
        float handleHeight = (scrollData.scrollContainerDimensions.height / scrollData.contentDimensions.height) * scrollData.scrollContainerDimensions.height;

        Clay_Color regularColor = { 0x54, 0x54, 0x54, 0xFF };
        Clay_Color hoveredColor = { 0x44, 0x44, 0x44, 0xFF };

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {
                    vertical ? CLAY_SIZING_FIXED(6) : CLAY_SIZING_FIXED(handleWidth),
                    vertical ? CLAY_SIZING_FIXED(handleHeight) : CLAY_SIZING_FIXED(6),
                },
            },
            .backgroundColor = parentHovered || Clay_Hovered() ? hoveredColor : regularColor,
            .cornerRadius    = CLAY_CORNER_RADIUS(6),
        });

        uint32_t currentID = Clay_GetOpenElementId();
        if(m_currentScrollBar != currentID && parentHovered && Clay_MouseClicked()) {
            m_currentScrollBar         = currentID;
            m_currentScrollClickOrigin = { m_cursorX, m_cursorY };
        }

        if(m_currentScrollBar == currentID) {
            if(vertical) {
                float centerOffset  = ((elementData.boundingBox.y + (handleHeight / 2)) - m_cursorY);
                float scrollPercent = centerOffset / (scrollData.scrollContainerDimensions.height - handleHeight);

                scrollData.scrollPosition->y = scrollPercent * (scrollData.contentDimensions.height - scrollData.scrollContainerDimensions.height);
            }
            else {
                float centerOffset  = ((elementData.boundingBox.x + (handleWidth / 2)) - m_cursorX);
                float scrollPercent = centerOffset / (scrollData.scrollContainerDimensions.width - handleWidth);

                scrollData.scrollPosition->x = scrollPercent * (scrollData.contentDimensions.width - scrollData.scrollContainerDimensions.width);
            }
        }
    }
}

void Application::BuildCameraLabel(const Application::CameraInfo& info) {
    CLAY_AUTO_ID() {
        Clay_TextElementConfig textConfig = defaultTextConfig;
        if(m_activeDropdown.id == camerasContainerID.id) {
            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;
                textConfig   = hoveredTextConfig;

                if(Clay_MouseClicked()) {
                    setCamera(info);
                    m_activeDropdown = m_invalidDropdown;
                }
            }

            if(m_currentCamera.id == info.id) {
                textConfig = selectedTextConfig;
            }
        }

        CLAY_TEXT(toClayString(info.name), textConfig);
    }
}

void Application::BuildCameraSettings() {
    SettingContainer(camerasContainerID, CLAY_SIZING_FIXED(200), true) {
        CONTAINER_TITLE("Camera");
        SEPARATOR;

        if(m_activeDropdown.id == camerasContainerID.id) {
            for(auto& info : m_cameras) {
                BuildCameraLabel(info);
            }

            continue;
        }

        BuildCameraLabel(m_currentCamera);
        if(Clay_Hovered()) {
            m_nextCursor = m_pointerCursor;

            if(Clay_MouseClicked()) {
                m_activeDropdown = camerasContainerID;
            }
        }
    }

    Build_ScrollBar(camerasContainerID);
}

void Application::BuildPixelFormatLabel(const PixelFormat& format) {
    CLAY_AUTO_ID() {
        Clay_TextElementConfig textConfig = defaultTextConfig;
        if(m_activeDropdown.id == pixelFormatContainerID.id) {
            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;
                textConfig   = hoveredTextConfig;

                if(Clay_MouseClicked()) {
                    Settings::get()->setPixelFormat(format);
                    m_activeDropdown = m_invalidDropdown;
                }
            }

            if(m_cameraData->camera.textureFormat == static_cast<SDL_PixelFormat>(format)) {
                textConfig = selectedTextConfig;
            }
        }

        CLAY_TEXT(cameraPixelFormatStr(format), textConfig);
    }
}

void Application::BuildPixelFormatSettings() {
    SettingContainer(pixelFormatContainerID, CLAY_SIZING_FIXED(200), true) {
        CONTAINER_TITLE("Pixel Format");
        SEPARATOR;

        if(m_activeDropdown.id == pixelFormatContainerID.id) {
            BuildPixelFormatLabel(PIXEL_FORMAT_RGB24);
            BuildPixelFormatLabel(PIXEL_FORMAT_CAMERA);

            continue;
        }

        BuildPixelFormatLabel(m_pixelFormat);
        if(Clay_Hovered()) {
            m_nextCursor = m_pointerCursor;

            if(Clay_MouseClicked()) {
                m_activeDropdown = pixelFormatContainerID;
            }
        }
    }

    Build_ScrollBar(pixelFormatContainerID);
}

void Application::BuildRecordingDeviceLabel(const RecordingDeviceInfo& info) {
    CLAY_AUTO_ID() {
        Clay_TextElementConfig textConfig = defaultTextConfig;
        if(m_activeDropdown.id == recordingDevicesContainerID.id) {
            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;
                textConfig   = hoveredTextConfig;

                if(Clay_MouseClicked()) {
                    setRecordingDevice(info);
                    m_activeDropdown = m_invalidDropdown;
                }
            }

            if(m_currentRecordingDevice.id == info.id) {
                textConfig = selectedTextConfig;
            }
        }

        CLAY_TEXT(toClayString(info.name), textConfig);
    }
}

void Application::BuildRecordingDeviceSettings() {
    SettingContainer(recordingDevicesContainerID, CLAY_SIZING_FIXED(250), true) {
        CONTAINER_TITLE("Recording Device");
        SEPARATOR;

        if(m_activeDropdown.id == recordingDevicesContainerID.id) {
            for(const RecordingDeviceInfo& info : m_recordingDevices) {
                BuildRecordingDeviceLabel(info);
            }

            continue;
        }

        BuildRecordingDeviceLabel(m_currentRecordingDevice);
        if(Clay_Hovered()) {
            m_nextCursor = m_pointerCursor;

            if(Clay_MouseClicked()) {
                m_activeDropdown = recordingDevicesContainerID;
            }
        }
    }

    Build_ScrollBar(recordingDevicesContainerID);
}

void Application::BuildDisplayModeLabel(const CameraDisplayMode& displayMode) {
    CLAY_AUTO_ID() {
        Clay_TextElementConfig textConfig = defaultTextConfig;
        if(m_activeDropdown.id == displayModeContainerID.id) {
            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;
                textConfig   = hoveredTextConfig;

                if(Clay_MouseClicked()) {
                    Settings::get()->setDisplayMode(displayMode);
                    m_activeDropdown = m_invalidDropdown;
                }
            }

            if(m_cameraData->camera.displayMode == displayMode) {
                textConfig = selectedTextConfig;
            }
        }

        CLAY_TEXT(displayModeStr(displayMode), textConfig);
    }
}

void Application::BuildDisplayModeSettings() {
    SettingContainer(displayModeContainerID, CLAY_SIZING_FIT(), true) {
        CONTAINER_TITLE("Display Mode");
        SEPARATOR;

        if(m_activeDropdown.id == displayModeContainerID.id) {
            BuildDisplayModeLabel(DISPLAY_MODE_CONTAIN);
            BuildDisplayModeLabel(DISPLAY_MODE_COVER);
            BuildDisplayModeLabel(DISPLAY_MODE_FILL);
            BuildDisplayModeLabel(DISPLAY_MODE_NONE);

            continue;
        }

        BuildDisplayModeLabel(m_cameraData->camera.displayMode);
        if(Clay_Hovered()) {
            m_nextCursor = m_pointerCursor;

            if(Clay_MouseClicked()) {
                m_activeDropdown = displayModeContainerID;
            }
        }
    }

    Build_ScrollBar(displayModeContainerID);
}

void Application::BuildFrameLimitTypeLabel(const FrameLimitType& type) {
    CLAY_AUTO_ID() {
        Clay_TextElementConfig textConfig = defaultTextConfig;
        if(m_activeDropdown.id == frameLimitTypeContainerID.id) {
            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;
                textConfig   = hoveredTextConfig;

                if(Clay_MouseClicked()) {
                    m_frameLimitInfo.type = type;
                    Settings::get()->setFrameLimitInfo(m_frameLimitInfo);

                    m_activeDropdown = m_invalidDropdown;
                }
            }

            if(m_frameLimitInfo.type == type) {
                textConfig = selectedTextConfig;
            }
        }

        CLAY_TEXT(frameLimitTypeStr(type), textConfig);
    }
}

void Application::BuildFrameLimiterSettings() {
    const auto checkSliderClicked = [this]() {
        if(Clay_Hovered()) {
            if(!Clay_MouseHeld()) {
                m_nextCursor = m_pointerCursor;
            }

            if(Clay_MouseClicked()) {
                m_slidingFPS = true;
            }
        }
    };

    SettingContainer(CLAY_ID("FrameLimitContainer"), CLAY_SIZING_FIT(), false) {
        CONTAINER_TITLE("Frame Limiting");
        SEPARATOR;

        CLAY(
            frameLimitTypeContainerID,
            {
                .layout = {
                    .childGap        = 6,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                .clip            = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
            }
        ) {
            if(m_activeDropdown.id == frameLimitTypeContainerID.id) {
                BuildFrameLimitTypeLabel(FRAME_LIMIT_CAMERA);
                BuildFrameLimitTypeLabel(FRAME_LIMIT_VSYNC);
                BuildFrameLimitTypeLabel(FRAME_LIMIT_VSYNC_ADAPTIVE);
                BuildFrameLimitTypeLabel(FRAME_LIMIT_FPS);
                BuildFrameLimitTypeLabel(FRAME_LIMIT_NONE);

                continue;
            }

            CLAY_AUTO_ID() {
                CLAY_TEXT(CLAY_STRING("Type: "), defaultTextConfig);
                BuildFrameLimitTypeLabel(m_frameLimitInfo.type);
            }

            if(Clay_Hovered()) {
                m_nextCursor = m_pointerCursor;

                if(Clay_MouseClicked()) {
                    m_activeDropdown = frameLimitTypeContainerID;
                }
            }
        }

        Build_ScrollBar(frameLimitTypeContainerID);

        if(m_frameLimitInfo.type == FRAME_LIMIT_FPS) {
            SEPARATOR;

            CLAY(
                fpsSliderTrackID,
                {
                    .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(6) } },
                    .backgroundColor = { 0x94, 0x94, 0x94, 0xFF },
                    .cornerRadius    = CLAY_CORNER_RADIUS(6),
                }
            ) {
                checkSliderClicked();

                Clay_ElementData data = Clay_GetElementData(fpsSliderTrackID);
                if(data.found && data.boundingBox.width != 0.0f) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing  = { .width = CLAY_SIZING_FIXED(6), .height = CLAY_SIZING_FIXED(12) },
                            .padding = CLAY_PADDING_ALL(1),
                        },
                        .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                        .cornerRadius    = CLAY_CORNER_RADIUS(4),

                        .floating = {
                            .offset       = { .x = (m_slidingFPS ? m_fpsSliderPosition : (m_frameLimitInfo.fps / MAX_FPS)) * data.boundingBox.width },
                            .attachPoints = {
                                .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                                .parent  = CLAY_ATTACH_POINT_LEFT_CENTER,
                            },
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .clipTo   = CLAY_CLIP_TO_ATTACHED_PARENT,
                        },
                    }) {
                        checkSliderClicked();
                    }
                }
            }

            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() }, .padding = { 0, 0, 6, 0 }, .childAlignment = { .x = CLAY_ALIGN_X_CENTER } } }) {
                CLAY_TEXT(
                    toClayString(m_fpsText.c_str()),
                    CLAY_TEXT_CONFIG({ .textColor = defaultTextConfig.textColor, .fontSize = static_cast<uint16_t>(defaultTextConfig.fontSize - 4) })
                );
            }
        }
    }
}

void Application::BuildFullscreenSettings() {
    SettingContainer(CLAY_ID("FullscreenContainer"), CLAY_SIZING_FIT(), false) {
        CONTAINER_TITLE("Fullscreen");
        SEPARATOR;

        const int baseSize          = 18;
        const int hoveredSizeOffset = 2;

        const Clay_Sizing regularSizing = { .width = CLAY_SIZING_FIXED(baseSize), .height = CLAY_SIZING_FIXED(baseSize) };
        const Clay_Sizing hoveredSizing = { .width = CLAY_SIZING_FIXED(baseSize + hoveredSizeOffset), .height = CLAY_SIZING_FIXED(baseSize + hoveredSizeOffset) };

        CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() }, .childAlignment = { .x = CLAY_ALIGN_X_CENTER } } }) {
            CLAY(
                CLAY_ID("FullscreenToggleButton"),
                {
                    .layout = {
                        .sizing  = Clay_Hovered() ? hoveredSizing : regularSizing,
                        .padding = CLAY_PADDING_ALL(2),
                    },
                    .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                    .cornerRadius    = CLAY_CORNER_RADIUS(2),
                }
            ) {
                if(Clay_Hovered()) {
                    if(Clay_MouseClicked()) {
                        setFullscreen(!m_isFullscreen);
                    }

                    m_nextCursor = m_pointerCursor;
                }

                const Clay_Color fullscreenColor    = { 0x36, 0x7E, 0xFF, 0xFF };
                const Clay_Color nonFullscreenColor = { 0xAF, 0xAF, 0xAF, 0xFF };

                CLAY(CLAY_ID("FullscreenToggleButtonInner"), { .layout = { .sizing = GROWGROW }, .backgroundColor = m_isFullscreen ? fullscreenColor : nonFullscreenColor });
            }
        }
    }
}

void Application::BuildVolumeSettings() {
    const auto checkSliderClicked = [this]() {
        if(Clay_Hovered()) {
            if(!Clay_MouseHeld()) {
                m_nextCursor = m_pointerCursor;
            }

            if(Clay_MouseClicked()) {
                m_slidingVolume = true;
            }
        }
    };

    const Clay_Color regularColor = { 0x52, 0xFA, 0x4D, 0xFF };
    const Clay_Color extraColor   = { 0xFA, 0x4D, 0x4D, 0xFF };

    SettingContainer(CLAY_ID("VolumeContainer"), CLAY_SIZING_FIT(), false) {
        CONTAINER_TITLE("Volume");
        SEPARATOR;

        CLAY(
            volumeSliderTrackID,
            {
                .layout       = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(6) } },
                .cornerRadius = CLAY_CORNER_RADIUS(6),
            }
        ) {
            checkSliderClicked();

            CLAY(
                CLAY_ID("VolumeTrackRegularRange"),
                {
                    // 0 to 100% volume
                    .layout          = { .sizing = { .width = CLAY_SIZING_PERCENT(100.0f / MAX_VOLUME), .height = CLAY_SIZING_GROW() } },
                    .backgroundColor = regularColor,
                    .cornerRadius    = { 6, 0, 6, 0 },
                }
            );

            CLAY(
                CLAY_ID("VolumeTrackExtraRange"),
                {
                    .layout          = { .sizing = GROWGROW },
                    .backgroundColor = extraColor,
                    .cornerRadius    = { 0, 6, 0, 6 },
                }
            );

            Clay_ElementData data = Clay_GetElementData(volumeSliderTrackID);
            if(data.found && data.boundingBox.width != 0.0f) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing  = { .width = CLAY_SIZING_FIXED(6), .height = CLAY_SIZING_FIXED(12) },
                        .padding = CLAY_PADDING_ALL(1),
                    },
                    .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                    .cornerRadius    = CLAY_CORNER_RADIUS(4),

                    .floating = {
                        .offset       = { .x = ((float)m_volume / MAX_VOLUME) * data.boundingBox.width },
                        .attachPoints = {
                            .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                            .parent  = CLAY_ATTACH_POINT_LEFT_CENTER,
                        },
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .clipTo   = CLAY_CLIP_TO_ATTACHED_PARENT,
                    },
                }) {
                    checkSliderClicked();
                }
            }
        }

        CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() }, .padding = { 0, 0, 6, 0 }, .childAlignment = { .x = CLAY_ALIGN_X_CENTER } } }) {
            CLAY_TEXT(
                toClayString(m_volumeText.c_str()),
                CLAY_TEXT_CONFIG({ .textColor = defaultTextConfig.textColor, .fontSize = static_cast<uint16_t>(defaultTextConfig.fontSize - 4) })
            );
        }
    }
}

const Clay_ElementId settingsContainerID = CLAY_ID("SettingsContainer");
void Application::BuildSettingsMenu() {
    if(!m_settingsActive) {
        return;
    }

    CLAY(
        CLAY_ID("SettingsBackground"),
        {
            .layout = {
                .sizing         = GROWGROW,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = { 0x00, 0x00, 0x00, 0xAF },
        }
    ) {
        if(Clay_DirectlyClicked()) {
            m_settingsActive = false;
        }

        CLAY(
            CLAY_ID("Settings"),
            {
                .layout = {
                    .sizing  = { CLAY_SIZING_PERCENT(0.66), CLAY_SIZING_PERCENT(0.66) },
                    .padding = CLAY_PADDING_ALL(16) },
                .backgroundColor = { 0x24, 0x24, 0x24, 0xEF },
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                .border          = { .color = { 0x9F, 0x9F, 0x9F, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
            }
        ) {
            if(Clay_DirectlyClicked()) {
                m_activeDropdown = m_invalidDropdown;
            }

            CLAY_AUTO_ID({ .layout = { .sizing = GROWGROW, .layoutDirection = CLAY_TOP_TO_BOTTOM } }) {
                CLAY(
                    settingsContainerID,
                    {
                        .layout = { .sizing = { .height = CLAY_SIZING_GROW() }, .childGap = 16 },
                        .clip   = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
                    }
                ) {
                    if(Clay_DirectlyClicked()) {
                        m_activeDropdown = m_invalidDropdown;
                    }

                    BuildCameraSettings();
                    BuildRecordingDeviceSettings();
                    BuildDisplayModeSettings();
                    BuildPixelFormatSettings();

                    BuildFrameLimiterSettings();

                    BuildFullscreenSettings();
                    BuildVolumeSettings();
                }

                // Build_ScrollBar(settingsContainerID, true);
                Build_ScrollBar(settingsContainerID, false);
            }
        }
    }
}

void Application::updateSettingsUI() {
    const static int VOLUME_SNAP_RANGE_MIN = 95;
    const static int VOLUME_SNAP_RANGE_MAX = 105;

    // offset from snap range to keep snapped variable
    const static int VOLUME_SNAP_RANGE_KEEP = 12;

    if(Clay_MouseReleasedNow()) {
        if(m_slidingFPS) {
            m_slidingFPS = false;
            Settings::get()->setFrameLimitInfo(m_frameLimitInfo);
        }

        if(m_slidingVolume) {
            m_slidingVolume        = false;
            m_volumeSnapped        = false;
            m_volumeInSnapRange    = false;
            m_volumeFreeDuringSnap = false;

            Settings::get()->setVolume(m_volume);
        }

        m_currentScrollBar = 0;
    }

    static float prevCursorX = 0.0f;
    if(m_slidingFPS) {
        Clay_ElementData data = Clay_GetElementData(fpsSliderTrackID);
        if(data.found && prevCursorX != m_cursorX) {
            const float percent = std::clamp((m_cursorX - data.boundingBox.x) / data.boundingBox.width, 0.0f, 1.0f);
            m_fpsSliderPosition = percent;

            int fps = percent * MAX_FPS;

            // round to nearest 5
            fps = ((fps + 5 - 1) / 5) * 5;
            if(fps < 20.0f) {
                fps = 0.0f;
            }

            updateFrameLimiter({ .type = m_frameLimitInfo.type, .fps = static_cast<float>(fps) });
        }
    }

    if(m_slidingVolume) {
        // snap volume to 100, if it goes within a range outside of the snap range then dont snap to 100
        Clay_ElementData data = Clay_GetElementData(volumeSliderTrackID);
        if(data.found && prevCursorX != m_cursorX) {
            const float percent = std::clamp((m_cursorX - data.boundingBox.x) / data.boundingBox.width, 0.0f, 1.0f);
            int volume          = percent * MAX_VOLUME;

            m_volumeInSnapRange      = volume >= VOLUME_SNAP_RANGE_MIN && volume <= VOLUME_SNAP_RANGE_MAX;
            const bool keepSnapRange = m_volumeInSnapRange || (volume >= VOLUME_SNAP_RANGE_MIN - VOLUME_SNAP_RANGE_KEEP && volume < VOLUME_SNAP_RANGE_MAX + VOLUME_SNAP_RANGE_KEEP);

            if(m_volumeInSnapRange && !m_volumeSnapped) {
                m_volumeSnapped        = true;
                m_volumeFreeDuringSnap = false;
            }

            if(m_volumeSnapped && !m_volumeFreeDuringSnap) {
                volume = 100;
            }

            if(!m_volumeInSnapRange && keepSnapRange) {
                m_volumeFreeDuringSnap = true;
            }

            m_volumeSnapped = m_volumeSnapped && keepSnapRange;
            setVolume(volume, false, false);
        }

        prevCursorX = m_cursorX;
    }
}