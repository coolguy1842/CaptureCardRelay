#include <algorithm>
#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <settings.hpp>

const Clay_String toClayString(const char* str) {
    if(str == NULL) {
        return { .isStaticallyAllocated = true, .length = 7, .chars = "(null)" };
    }

    return { .isStaticallyAllocated = true, .length = static_cast<int32_t>(strlen(str)), .chars = str };
}

Clay_String displayModeStr(CameraDisplayMode mode) {
    switch(mode) {
    case CameraDisplayMode::CONTAIN: return toClayString("Contain");
    case CameraDisplayMode::COVER:   return toClayString("Cover");
    case CameraDisplayMode::FILL:    return toClayString("Fill");
    case CameraDisplayMode::NONE:    return toClayString("None");
    default:                         return toClayString("Unknown");
    }
}

std::optional<CameraDisplayMode> displayModeFromStr(const char* str) {
    if(strcmp(str, "Contain") == 0) return CameraDisplayMode::CONTAIN;
    if(strcmp(str, "Cover") == 0) return CameraDisplayMode::COVER;
    if(strcmp(str, "Fill") == 0) return CameraDisplayMode::FILL;
    if(strcmp(str, "None") == 0) return CameraDisplayMode::NONE;

    return std::nullopt;
}

const Clay_ElementId camerasContainerID          = CLAY_ID("CamerasContainer");
const Clay_ElementId recordingDevicesContainerID = CLAY_ID("RecordingDevicesContainer");

const Clay_ElementId fullscreenContainerID = CLAY_ID("FullscreenContainer");
const Clay_ElementId fullscreenToggleID    = CLAY_ID("FullscreenToggle");

const Clay_ElementId volumeContainerID   = CLAY_ID("VolumeContainer");
const Clay_ElementId volumeSliderTrackID = CLAY_ID("VolumeSliderTrack");

const Clay_ElementId displayModeContainerID = CLAY_ID("DisplayModeContainer");

Clay_TransitionData EnterExitSlide(Clay_TransitionData initialState, Clay_TransitionProperty properties);
void Application::render() {
    SDL_SetRenderDrawColor(m_renderData.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderData.renderer);

    Clay_BeginLayout();
    Clay_TextElementConfig defaultTextConfig = CLAY_TEXT_CONFIG({
        .textColor = { 0xFF, 0xFF, 0xFF, 0xFF },
        .fontSize  = 24,
    });

    SDL_Cursor* nextCursor = m_defaultCursor;

    CLAY(
        CLAY_ID("Body"),
        {
            .layout = { .sizing = { CLAY_SIZING_PERCENT(1.0), CLAY_SIZING_PERCENT(1.0) } },
            .custom = { .customData = m_cameraData.get() },
        }
    ) {
        if(m_settingsActive) {
            static const Clay_ElementId settingsBackgroundID = CLAY_ID("SettingsBackground");
            CLAY(
                settingsBackgroundID,
                {
                    .layout = {
                        .sizing         = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    },
                    .backgroundColor = { 0x00, 0x00, 0x00, 0xAF },
                }
            ) {
                // only if background is clicked, not children
                Clay_ElementIdArray ids = Clay_GetPointerOverIds();
                if(ids.length != 0 && ids.internalArray[ids.length - 1].id == settingsBackgroundID.id && m_mouseClicked) {
                    m_settingsActive = false;
                }

                CLAY(
                    CLAY_ID("Settings"),
                    {
                        .layout = {
                            .sizing   = { CLAY_SIZING_PERCENT(0.66), CLAY_SIZING_PERCENT(0.66) },
                            .padding  = CLAY_PADDING_ALL(16),
                            .childGap = 16,
                        },
                        .backgroundColor = { 0x24, 0x24, 0x24, 0xEF },
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .clip            = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
                        .border          = { .color = { 0x9F, 0x9F, 0x9F, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                    }
                ) {
                    Clay_TextElementConfig selectedTextConfig = CLAY_TEXT_CONFIG({
                        .textColor = { 0x9F, 0x9F, 0x9F, 0xFF },
                        .fontSize  = static_cast<uint16_t>(defaultTextConfig.fontSize + 2),
                    });

                    Clay_TextElementConfig hoveredTextConfig = CLAY_TEXT_CONFIG({
                        .textColor = { 0xBF, 0xBF, 0xBF, 0xFF },
                        .fontSize  = static_cast<uint16_t>(defaultTextConfig.fontSize + 1),
                    });

#define SEPARATOR CLAY_AUTO_ID({                                                                      \
    .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(2) } }, \
    .backgroundColor = { 0xAF, 0xAF, 0xAF, 0xFF },                                                    \
})

                    CLAY(
                        camerasContainerID,
                        {
                            .layout = {
                                .sizing          = { .width = CLAY_SIZING_FIT(0, 200) },
                                .padding         = CLAY_PADDING_ALL(12),
                                .childGap        = 6,
                                .childAlignment  = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                            .cornerRadius    = CLAY_CORNER_RADIUS(6),
                            // set vertical to true only if active menu, then make inactive if offscreen to avoid scissor fails
                            .clip   = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                            .border = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("Camera"), defaultTextConfig);
                        SEPARATOR;

                        auto CameraButton = [&](const CameraInfo& info) {
                            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() } } }) {
                                Clay_TextElementConfig textConfig = defaultTextConfig;
                                if(m_activeDropdown.id == camerasContainerID.id) {
                                    if(Clay_Hovered()) {
                                        nextCursor = m_pointerCursor;
                                        textConfig = hoveredTextConfig;

                                        if(m_mouseClicked) {
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
                        };

                        if(m_activeDropdown.id == camerasContainerID.id) {
                            for(auto& info : m_cameras) {
                                CameraButton(info);
                            }
                        }
                        else {
                            CameraButton(m_currentCamera);

                            if(Clay_Hovered()) {
                                nextCursor = m_pointerCursor;

                                if(m_mouseClicked) {
                                    m_activeDropdown = camerasContainerID;
                                }
                            }
                        }
                    }

                    CLAY(
                        recordingDevicesContainerID,
                        {
                            .layout = {
                                .sizing          = { .width = CLAY_SIZING_FIT(0, 250) },
                                .padding         = CLAY_PADDING_ALL(12),
                                .childGap        = 6,
                                .childAlignment  = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                            .cornerRadius    = CLAY_CORNER_RADIUS(6),
                            .clip            = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                            .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("Recording Device"), defaultTextConfig);
                        SEPARATOR;

                        auto RecordingDevice = [&](const RecordingDeviceInfo& info) {
                            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() } } }) {
                                Clay_TextElementConfig textConfig = defaultTextConfig;
                                if(m_activeDropdown.id == recordingDevicesContainerID.id) {
                                    if(Clay_Hovered()) {
                                        nextCursor = m_pointerCursor;
                                        textConfig = hoveredTextConfig;

                                        if(m_mouseClicked) {
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
                        };

                        if(m_activeDropdown.id == recordingDevicesContainerID.id) {
                            for(const RecordingDeviceInfo& info : m_recordingDevices) {
                                RecordingDevice(info);
                            }
                        }
                        else {
                            RecordingDevice(m_currentRecordingDevice);

                            if(Clay_Hovered()) {
                                nextCursor = m_pointerCursor;

                                if(m_mouseClicked) {
                                    m_activeDropdown = recordingDevicesContainerID;
                                }
                            }
                        }
                    }

                    CLAY(
                        displayModeContainerID,
                        {
                            .layout = {
                                .padding         = CLAY_PADDING_ALL(12),
                                .childGap        = 6,
                                .childAlignment  = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                            .cornerRadius    = CLAY_CORNER_RADIUS(6),
                            .clip            = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                            .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("Display Mode"), defaultTextConfig);
                        SEPARATOR;

                        CameraDisplayMode currentMode = m_cameraData->camera.displayMode;
                        auto DisplayMode              = [&](const CameraDisplayMode& mode) {
                            CLAY_AUTO_ID({ .layout = { .sizing = { .width = CLAY_SIZING_GROW() } } }) {
                                Clay_TextElementConfig textConfig = defaultTextConfig;
                                if(m_activeDropdown.id == displayModeContainerID.id) {
                                    if(Clay_Hovered()) {
                                        nextCursor = m_pointerCursor;
                                        textConfig = hoveredTextConfig;

                                        if(m_mouseClicked) {
                                            Settings::get()->setDisplayMode(mode);
                                            m_activeDropdown = m_invalidDropdown;
                                        }
                                    }

                                    if(currentMode == mode) {
                                        textConfig = selectedTextConfig;
                                    }
                                }

                                CLAY_TEXT(displayModeStr(mode), textConfig);
                            }
                        };

                        if(m_activeDropdown.id == displayModeContainerID.id) {
                            DisplayMode(CONTAIN);
                            DisplayMode(COVER);
                            DisplayMode(FILL);
                            DisplayMode(NONE);
                        }
                        else {
                            DisplayMode(currentMode);

                            if(Clay_Hovered()) {
                                nextCursor = m_pointerCursor;

                                if(m_mouseClicked) {
                                    m_activeDropdown = displayModeContainerID;
                                }
                            }
                        }
                    }

                    CLAY(
                        fullscreenContainerID,
                        {
                            .layout = {
                                .padding         = CLAY_PADDING_ALL(12),
                                .childGap        = 6,
                                .childAlignment  = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                            .cornerRadius    = CLAY_CORNER_RADIUS(6),
                            .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("Fullscreen"), defaultTextConfig);
                        SEPARATOR;

                        Clay_Sizing regularSizing = { .width = CLAY_SIZING_FIXED(18), .height = CLAY_SIZING_FIXED(18) };
                        Clay_Sizing hoveredSizing = { .width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20) };

                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing  = Clay_Hovered() ? hoveredSizing : regularSizing,
                                .padding = CLAY_PADDING_ALL(2),
                            },
                            .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                            .cornerRadius    = CLAY_CORNER_RADIUS(2),
                        }) {
                            if(Clay_Hovered()) {
                                nextCursor = m_pointerCursor;

                                if(m_mouseClicked) {
                                    setFullscreen(!Settings::get()->isFullscreen());
                                }
                            }

                            Clay_Color fullscreenColor    = { 0x36, 0x7E, 0xFF, 0xFF };
                            Clay_Color nonFullscreenColor = { 0xAF, 0xAF, 0xAF, 0xFF };

                            CLAY_AUTO_ID({
                                .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() } },
                                .backgroundColor = m_isFullscreen ? fullscreenColor : nonFullscreenColor,
                            });
                        }
                    }

                    CLAY_AUTO_ID({
                        .layout = {
                            .padding         = CLAY_PADDING_ALL(12),
                            .childGap        = 6,
                            .childAlignment  = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
                        .cornerRadius    = CLAY_CORNER_RADIUS(6),
                        .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
                    }) {
                        CLAY_TEXT(CLAY_STRING("Volume"), defaultTextConfig);
                        SEPARATOR;

                        CLAY(
                            volumeSliderTrackID,
                            {
                                .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(6) } },
                                .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                                .cornerRadius    = CLAY_CORNER_RADIUS(6),
                            }
                        ) {
                            float regularRange = 100.0f / MAX_VOLUME;
                            // range past 100%
                            float extraRange = 1.0f - regularRange;

                            auto checkSliderClicked = [&]() {
                                if(Clay_Hovered()) {
                                    if(!m_mouseHeld) {
                                        nextCursor = m_pointerCursor;
                                    }

                                    if(m_mouseClicked) {
                                        m_slidingVolume = true;
                                    }
                                }
                            };

                            checkSliderClicked();

                            CLAY_AUTO_ID({
                                .layout          = { .sizing = { .width = CLAY_SIZING_PERCENT(regularRange), .height = CLAY_SIZING_GROW() } },
                                .backgroundColor = { 0x00, 0xFF, 0x00, 0xFF },
                                .cornerRadius    = { 6, 0, 6, 0 },
                            });

                            CLAY_AUTO_ID({
                                .layout          = { .sizing = { .width = CLAY_SIZING_PERCENT(extraRange + 0.01f), .height = CLAY_SIZING_GROW() } },
                                .backgroundColor = { 0xFF, 0x00, 0x00, 0xFF },
                                .cornerRadius    = { 0, 6, 0, 6 },
                            });

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
                                        .offset = {
                                            .x = ((float)m_volume / MAX_VOLUME) * data.boundingBox.width,
                                        },
                                        .attachPoints = {
                                            .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                                            .parent  = CLAY_ATTACH_POINT_LEFT_CENTER,
                                        },
                                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                                        .attachTo           = CLAY_ATTACH_TO_PARENT,
                                        .clipTo             = CLAY_CLIP_TO_ATTACHED_PARENT,
                                    },
                                }) {
                                    checkSliderClicked();
                                }
                            }
                        }

                        CLAY_AUTO_ID({ .layout = { .padding = { 0, 0, 6, 0 } } }) {
                            CLAY_TEXT(
                                toClayString(m_volumeText.c_str()),
                                CLAY_TEXT_CONFIG({
                                    .textColor = { 0xBF, 0xBF, 0xBF, 0xFF },
                                    .fontSize  = static_cast<uint16_t>(defaultTextConfig.fontSize - 2),
                                })
                            );
                        }
                    }
                }
            }
        }

        if(m_status.expire.has_value()) {
            // minimize system clock checks
            if(m_status.expire > std::chrono::system_clock::now()) {
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
            else {
                m_status.expire = std::nullopt;
            }
        }
    }

    static uint64_t prev = SDL_GetPerformanceCounter();
    uint64_t now         = SDL_GetPerformanceCounter();

    float deltaTime = (now - prev) / static_cast<float>(SDL_GetPerformanceFrequency());
    prev            = now;

    Clay_RenderCommandArray renderCommands = Clay_EndLayout(deltaTime);
    SDL_Clay_RenderClayCommands(&m_renderData, &renderCommands);

    if(m_currentCursor != nextCursor) {
        m_currentCursor = nextCursor;
        SDL_SetCursor(m_currentCursor);
    }

    const static int VOLUME_SNAP_RANGE_MIN = 95;
    const static int VOLUME_SNAP_RANGE_MAX = 105;

    // offset from snap range to keep snapped variable
    const static int VOLUME_SNAP_RANGE_KEEP = 12;

    static float prevCursorX = 0.0f;
    if(m_slidingVolume) {

        // snap volume to 100, if it goes within a range outside of the snap range then dont snap to 100
        Clay_ElementData data = Clay_GetElementData(volumeSliderTrackID);
        if(data.found && prevCursorX != m_cursorX) {
            float percent = std::clamp((m_cursorX - data.boundingBox.x) / data.boundingBox.width, 0.0f, 1.0f);
            int volume    = percent * MAX_VOLUME;

            m_volumeInSnapRange = volume >= VOLUME_SNAP_RANGE_MIN && volume <= VOLUME_SNAP_RANGE_MAX;
            bool keepSnapRange  = m_volumeInSnapRange || (volume >= VOLUME_SNAP_RANGE_MIN - VOLUME_SNAP_RANGE_KEEP && volume < VOLUME_SNAP_RANGE_MAX + VOLUME_SNAP_RANGE_KEEP);

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

            setVolume(volume, false);
        }
    }

    prevCursorX = m_cursorX;
    SDL_RenderPresent(m_renderData.renderer);
}

Clay_TransitionData EnterExitSlide(Clay_TransitionData initialState, Clay_TransitionProperty properties) {
    Clay_TransitionData targetState = initialState;
    if(properties & CLAY_TRANSITION_PROPERTY_Y) {
        targetState.boundingBox.y += targetState.boundingBox.height;
    }

    return targetState;
}
