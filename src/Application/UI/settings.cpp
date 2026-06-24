#include <algorithm>
#include <application.hpp>
#include <clay_renderer_SDL3.hpp>
#include <settings.hpp>

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
static const Clay_ElementId recordingDevicesContainerID = CLAY_ID("RecordingDevicesContainer");
static const Clay_ElementId displayModeContainerID      = CLAY_ID("DisplayModeContainer");

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

Clay_ElementDeclaration SettingContainerConfig(Clay_SizingAxis widthSizing = CLAY_SIZING_FIT(), bool clip = false) {
    return {
        .layout = {
            .sizing          = { .width = widthSizing },
            .padding         = CLAY_PADDING_ALL(12),
            .childGap        = 6,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = { 0x34, 0x34, 0x34, 0xFF },
        .cornerRadius    = CLAY_CORNER_RADIUS(6),
        .clip            = { .vertical = clip, .childOffset = Clay_GetScrollOffset() },
        .border          = { .color = { 0xFF, 0xFF, 0xFF, 0xFF }, .width = CLAY_BORDER_OUTSIDE(1) },
    };
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
    CLAY(camerasContainerID, SettingContainerConfig(CLAY_SIZING_FIXED(200), true)) {
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
    CLAY(recordingDevicesContainerID, SettingContainerConfig(CLAY_SIZING_FIXED(250), true)) {
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
    CLAY(displayModeContainerID, SettingContainerConfig(CLAY_SIZING_FIT(), true)) {
        CONTAINER_TITLE("Display Mode");
        SEPARATOR;

        if(m_activeDropdown.id == displayModeContainerID.id) {
            BuildDisplayModeLabel(CONTAIN);
            BuildDisplayModeLabel(COVER);
            BuildDisplayModeLabel(FILL);
            BuildDisplayModeLabel(NONE);

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
}

void Application::BuildFullscreenSettings() {
    CLAY(CLAY_ID("FullscreenContainer"), SettingContainerConfig()) {
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

    CLAY(CLAY_ID("VolumeContainer"), SettingContainerConfig()) {
        CONTAINER_TITLE("Volume");
        SEPARATOR;

        CLAY(
            volumeSliderTrackID,
            {
                .layout          = { .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_FIXED(6) } },
                .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF },
                .cornerRadius    = CLAY_CORNER_RADIUS(6),
            }
        ) {
            checkSliderClicked();

            CLAY(
                CLAY_ID("VolumeTrackRegularRange"),
                {
                    // 0 to 100% volume
                    .layout          = { .sizing = { .width = CLAY_SIZING_PERCENT(100.0f / MAX_VOLUME), .height = CLAY_SIZING_GROW() } },
                    .backgroundColor = { 0x52, 0xFA, 0x4D, 0xFF },
                    .cornerRadius    = { 6, 0, 6, 0 },
                }
            );

            CLAY(
                CLAY_ID("VolumeTrackExtraRange"),
                {
                    .layout          = { .sizing = GROWGROW },
                    .backgroundColor = { 0xFA, 0x4D, 0x4D, 0xFF },
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
            BuildCameraSettings();
            BuildRecordingDeviceSettings();
            BuildDisplayModeSettings();

            BuildFullscreenSettings();
            BuildVolumeSettings();
        }
    }
}

void Application::updateSettingsUI() {
    const static int VOLUME_SNAP_RANGE_MIN = 95;
    const static int VOLUME_SNAP_RANGE_MAX = 105;

    // offset from snap range to keep snapped variable
    const static int VOLUME_SNAP_RANGE_KEEP = 12;

    if(m_slidingVolume && !Clay_MouseHeld()) {
        m_slidingVolume        = false;
        m_volumeSnapped        = false;
        m_volumeInSnapRange    = false;
        m_volumeFreeDuringSnap = false;

        Settings::get()->setVolume(m_volume);
    }

    if(m_slidingVolume) {
        static float prevCursorX = 0.0f;

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