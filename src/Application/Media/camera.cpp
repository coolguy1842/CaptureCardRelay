#include <SDL3/SDL_stdinc.h>
#include <application.hpp>
#include <format>
#include <set>
#include <span>

#ifndef __EMSCRIPTEN__

namespace {
uint16_t getColorScore(const SDL_Colorspace& color) {
    // NOLINTBEGIN(bugprone-branch-clone)
    switch(color) {
    case SDL_COLORSPACE_SRGB:           return 2;
    case SDL_COLORSPACE_SRGB_LINEAR:    return 2;
    case SDL_COLORSPACE_HDR10:          return 4;
    case SDL_COLORSPACE_JPEG:           return 2;
    case SDL_COLORSPACE_BT601_LIMITED:  return 1;
    case SDL_COLORSPACE_BT601_FULL:     return 2;
    case SDL_COLORSPACE_BT709_LIMITED:  return 1;
    case SDL_COLORSPACE_BT709_FULL:     return 2;
    case SDL_COLORSPACE_BT2020_LIMITED: return 2;
    case SDL_COLORSPACE_BT2020_FULL:    return 3;
    case SDL_COLORSPACE_UNKNOWN:
    default:                            return 0;
    };
    // NOLINTEND(bugprone-branch-clone)
};

uint16_t getFormatScore(const SDL_PixelFormat& format) {
    // dont remember how i came to these
    // NOLINTBEGIN(bugprone-branch-clone)
    switch(format) {
    case SDL_PIXELFORMAT_MJPG:          return 1;
    case SDL_PIXELFORMAT_EXTERNAL_OES:  return 2;
    case SDL_PIXELFORMAT_INDEX1LSB:     return 3;
    case SDL_PIXELFORMAT_INDEX1MSB:     return 4;
    case SDL_PIXELFORMAT_INDEX2LSB:     return 5;
    case SDL_PIXELFORMAT_INDEX2MSB:     return 6;
    case SDL_PIXELFORMAT_INDEX4LSB:     return 7;
    case SDL_PIXELFORMAT_INDEX4MSB:     return 8;
    case SDL_PIXELFORMAT_INDEX8:        return 9;
    case SDL_PIXELFORMAT_RGB332:        return 1;
    case SDL_PIXELFORMAT_XRGB4444:      return 1;
    case SDL_PIXELFORMAT_XBGR4444:      return 1;
    case SDL_PIXELFORMAT_XRGB1555:      return 1;
    case SDL_PIXELFORMAT_XBGR1555:      return 1;
    case SDL_PIXELFORMAT_ARGB4444:      return 1;
    case SDL_PIXELFORMAT_RGBA4444:      return 1;
    case SDL_PIXELFORMAT_ABGR4444:      return 1;
    case SDL_PIXELFORMAT_BGRA4444:      return 1;
    case SDL_PIXELFORMAT_ARGB1555:      return 1;
    case SDL_PIXELFORMAT_RGBA5551:      return 2;
    case SDL_PIXELFORMAT_ABGR1555:      return 2;
    case SDL_PIXELFORMAT_BGRA5551:      return 2;
    case SDL_PIXELFORMAT_RGB565:        return 2;
    case SDL_PIXELFORMAT_BGR565:        return 2;
    case SDL_PIXELFORMAT_RGB24:         return 2;
    case SDL_PIXELFORMAT_BGR24:         return 2;
    case SDL_PIXELFORMAT_XRGB8888:      return 2;
    case SDL_PIXELFORMAT_RGBX8888:      return 2;
    case SDL_PIXELFORMAT_XBGR8888:      return 2;
    case SDL_PIXELFORMAT_BGRX8888:      return 3;
    case SDL_PIXELFORMAT_ARGB8888:      return 3;
    case SDL_PIXELFORMAT_RGBA8888:      return 3;
    case SDL_PIXELFORMAT_ABGR8888:      return 3;
    case SDL_PIXELFORMAT_BGRA8888:      return 3;
    case SDL_PIXELFORMAT_XRGB2101010:   return 3;
    case SDL_PIXELFORMAT_XBGR2101010:   return 3;
    case SDL_PIXELFORMAT_ARGB2101010:   return 3;
    case SDL_PIXELFORMAT_ABGR2101010:   return 3;
    case SDL_PIXELFORMAT_RGB48:         return 3;
    case SDL_PIXELFORMAT_BGR48:         return 4;
    case SDL_PIXELFORMAT_RGBA64:        return 4;
    case SDL_PIXELFORMAT_ARGB64:        return 4;
    case SDL_PIXELFORMAT_BGRA64:        return 4;
    case SDL_PIXELFORMAT_ABGR64:        return 4;
    case SDL_PIXELFORMAT_RGB48_FLOAT:   return 4;
    case SDL_PIXELFORMAT_BGR48_FLOAT:   return 4;
    case SDL_PIXELFORMAT_RGBA64_FLOAT:  return 4;
    case SDL_PIXELFORMAT_ARGB64_FLOAT:  return 4;
    case SDL_PIXELFORMAT_BGRA64_FLOAT:  return 4;
    case SDL_PIXELFORMAT_ABGR64_FLOAT:  return 5;
    case SDL_PIXELFORMAT_RGB96_FLOAT:   return 5;
    case SDL_PIXELFORMAT_BGR96_FLOAT:   return 5;
    case SDL_PIXELFORMAT_RGBA128_FLOAT: return 5;
    case SDL_PIXELFORMAT_ARGB128_FLOAT: return 5;
    case SDL_PIXELFORMAT_BGRA128_FLOAT: return 5;
    case SDL_PIXELFORMAT_ABGR128_FLOAT: return 5;
    case SDL_PIXELFORMAT_YV12:          return 5;
    case SDL_PIXELFORMAT_IYUV:          return 5;
    case SDL_PIXELFORMAT_YUY2:          return 5;
    case SDL_PIXELFORMAT_UYVY:          return 6;
    case SDL_PIXELFORMAT_YVYU:          return 6;
    case SDL_PIXELFORMAT_NV12:          return 6;
    case SDL_PIXELFORMAT_NV21:          return 6;
    case SDL_PIXELFORMAT_P010:          return 7;
    case SDL_PIXELFORMAT_UNKNOWN:
    default:                            return 0;
    };
    // NOLINTEND(bugprone-branch-clone)
};

SDL_Colorspace s_preferredColorspace   = SDL_COLORSPACE_UNKNOWN;
SDL_PixelFormat s_preferredPixelFormat = SDL_PIXELFORMAT_UNKNOWN;
bool compareCameraSpecs(const SDL_CameraSpec& a, const SDL_CameraSpec& b) {
    if(s_preferredColorspace != SDL_COLORSPACE_UNKNOWN && a.colorspace != b.colorspace) {
        if(a.colorspace == s_preferredColorspace) {
            return true;
        }

        if(b.colorspace == s_preferredColorspace) {
            return false;
        }
    }

    if(s_preferredPixelFormat != SDL_PIXELFORMAT_UNKNOWN && a.format != b.format) {
        if(a.format == s_preferredPixelFormat) {
            return true;
        }

        if(b.format == s_preferredPixelFormat) {
            return false;
        }
    }

    if(a.width != b.width) {
        return a.width > b.width;
    }

    if(a.height != b.height) {
        return a.height > b.height;
    }

    if(a.framerate_numerator / a.framerate_denominator != b.framerate_numerator / b.framerate_denominator) {
        return a.framerate_numerator / a.framerate_denominator > b.framerate_numerator / b.framerate_denominator;
    }

    if(getColorScore(a.colorspace) != getColorScore(b.colorspace)) {
        return getColorScore(a.colorspace) > getColorScore(b.colorspace);
    }

    if(getFormatScore(a.format) != getFormatScore(b.format)) {
        return getFormatScore(a.format) > getFormatScore(b.format);
    }

    return false;
}

}; // namespace

#endif

void Application::initCameras() {
    std::vector<SDL_CameraID> cameras;
    m_cameras.clear();

    {
        int cameraCount          = 0;
        SDL_CameraID* camerasRaw = SDL_GetCameras(&cameraCount);

        auto camerasSpan = std::span(camerasRaw, static_cast<size_t>(cameraCount));
        cameras.assign(camerasSpan.begin(), camerasSpan.end());

        if(camerasRaw == nullptr) {
            SDL_free(camerasRaw);

            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enumerate camera devices: %s", SDL_GetError());
            setShouldQuit(true);

            return;
        }

        SDL_free(camerasRaw);
    }

    for(const SDL_CameraID& camera : cameras) {
        const char* name = SDL_GetCameraName(camera);

        if(name == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get name of camera with id: %d: %s", camera, SDL_GetError());
            name = "(null)";
        }

        m_cameras.push_back({ .id = camera, .name = name });
    }
}

void Application::openCamera() {
    // cleanup old data
    closeCamera();
    initCameras();

    if(m_cameras.empty()) {
        return;
    }

    SDL_CameraID camID = m_settings.getSelectedCamera();
    if(camID == 0) {
        camID = m_cameras[0].id;
    }

#ifndef __EMSCRIPTEN__
    s_preferredColorspace  = m_settings.getPreferredColorspace();
    s_preferredPixelFormat = m_settings.getPreferredPixelFormat();
    std::set<SDL_CameraSpec, decltype(&compareCameraSpecs)> specs(&compareCameraSpecs);

    {
        int numFormats              = 0;
        SDL_CameraSpec** formatsRaw = SDL_GetCameraSupportedFormats(camID, &numFormats);

        for(const SDL_CameraSpec* spec : std::span(formatsRaw, static_cast<size_t>(numFormats))) {
            specs.emplace(*spec);
        }

        SDL_free(reinterpret_cast<void*>(formatsRaw));
    }

    if(specs.empty()) {
        return;
    }

    SDL_Log("Possible Camera Specs:");
    for(const SDL_CameraSpec& spec : specs) {
        SDL_Log("  %dx%d@%0.2f - Format: %s - Colorspace: %s", spec.width, spec.height, static_cast<float>(spec.framerate_numerator) / static_cast<float>(spec.framerate_denominator), pixelFormatName(spec.format), colorspaceName(spec.colorspace));
    }

    const SDL_CameraSpec spec     = *specs.begin();
    const SDL_CameraSpec* specPtr = &spec;

    specs.clear();

    SDL_Log("Requesting Camera Spec: %dx%d@%0.2f - Format: %s - Colorspace: %s", spec.width, spec.height, static_cast<float>(spec.framerate_numerator) / static_cast<float>(spec.framerate_denominator), pixelFormatName(spec.format), colorspaceName(spec.colorspace));
#else
    const SDL_CameraSpec* specPtr = nullptr;
#endif

    const char* name = SDL_GetCameraName(camID);
    if(name == nullptr) {
        name = "(null)";
    }

    SDL_Log("\n");
    SDL_Log("Opening camera: %s", name);

    m_camera.id   = camID;
    m_camera.name = name;

    m_camera.approved = false;

    m_camera.device = SDL_OpenCamera(camID, specPtr);
    if(m_camera.device == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Opening camera: %s", SDL_GetError());
        closeCamera();

        return;
    }

    updateCameraTexture();
}

void Application::closeCamera() {
    m_camera.approved = false;

    if(m_camera.device != nullptr) {
        SDL_CloseCamera(m_camera.device);
        m_camera.device = nullptr;
    }

    cleanupCameraTexture();
}

void Application::setCamera(Application::CameraInfo info) {
    m_settings.setSelectedCamera(info.id);

    if(info.id != 0) {
        if(m_camera.id != info.id) {
            openCamera();
        }
    }
    else {
        closeCamera();
    }

    changeStatus(std::format("Camera: {}", info.name), std::chrono::milliseconds(1500));
}

void Application::updateCameraDisplayRect() {
    const SDL_FRect rect = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height) };
    m_camera.displayRect = rect;

    if(m_camera.texture == nullptr) {
        return;
    }

    const CameraDisplayMode mode = m_settings.getDisplayMode();
    switch(mode) {
    case DISPLAY_MODE_CONTAIN:
    case DISPLAY_MODE_COVER:   {
        // most logic here from: https://github.com/nrkn/object-fit-math/blob/master/src/fitter.ts
        const float widthRatio  = rect.w / static_cast<float>(m_camera.texture->w);
        const float heightRatio = rect.h / static_cast<float>(m_camera.texture->h);

        // min of width vs height ratios
        const float ratio = mode == DISPLAY_MODE_CONTAIN
                                ? CLAY__MIN(widthRatio, heightRatio)
                                : CLAY__MAX((rect.w / m_camera.texture->w), (rect.h / m_camera.texture->h));

        SDL_FRect newRect;
        newRect.w = static_cast<float>(m_camera.texture->w) * ratio;
        newRect.h = static_cast<float>(m_camera.texture->h) * ratio;
        newRect.x = (rect.w - newRect.w) / 2.0f;
        newRect.y = (rect.h - newRect.h) / 2.0f;

        m_camera.displayRect = newRect;
        break;
    }
    case DISPLAY_MODE_FILL: break;
    case DISPLAY_MODE_NONE:
        m_camera.displayRect = {
            .x = 0,
            .y = 0,
            .w = static_cast<float>(m_camera.texture->w),
            .h = static_cast<float>(m_camera.texture->h),
        };

        break;
    default: break;
    }
}

void Application::cleanupCameraTexture() {
    if(m_camera.texture != nullptr) {
        SDL_DestroyTexture(m_camera.texture);
        m_camera.texture = nullptr;
    }

    m_camera.pixels.clear();
    m_camera.pitch = 0;
}

void Application::updateCameraTexture() {
    cleanupCameraTexture();
    if(m_camera.device == nullptr || !m_camera.approved) {
        return;
    }

    const SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create properties for texture: %s", SDL_GetError());
        cleanupCameraTexture();

        return;
    }

    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, m_camera.spec.width);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, m_camera.spec.height);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, m_camera.spec.colorspace);

    auto format = static_cast<SDL_PixelFormat>(m_settings.getPixelFormat());
    if(format == SDL_PIXELFORMAT_UNKNOWN) {
        // use cameras pixel format
        format = m_camera.spec.format;
    }

    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, format);

    m_camera.texture = SDL_CreateTextureWithProperties(m_renderData.renderer, props);
    if(m_camera.texture == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s", SDL_GetError());
        SDL_DestroyProperties(props);

        cleanupCameraTexture();
    }

    SDL_DestroyProperties(props);
    SDL_SetTextureScaleMode(m_camera.texture, m_settings.getScaleMode());

    // make sure texture starts blank, doesnt work on mjpeg but dont care too much
    SDL_Surface* surface = nullptr;
    if(SDL_LockTextureToSurface(m_camera.texture, nullptr, &surface)) {
        SDL_ClearSurface(surface, 0.0, 0.0, 0.0, 1.0);
        SDL_UnlockTexture(m_camera.texture);
    }

    if(format != m_camera.spec.format) {
        // for some reason writing to a temporary buffer first is faster when using SDL_ConvertPixels, pitch calculation from sdl3s internal source code
        m_camera.pitch = ((m_camera.texture->w * SDL_BYTESPERPIXEL(m_camera.texture->format)) + 3) & ~3;
        m_camera.pixels.resize(static_cast<size_t>(m_camera.texture->h) * static_cast<size_t>(m_camera.pitch));
    }

    updateCameraDisplayRect();

#ifndef __EMSCRIPTEN__
    updateFrameLimiter(m_frameLimitInfo);
#endif
}

void Application::renderCameraToTexture() {
    if(!m_camera.approved || m_camera.texture == nullptr) {
        return;
    }

    SDL_Surface* surface = SDL_AcquireCameraFrame(m_camera.device, nullptr);
    if(surface != nullptr) {
        switch(m_camera.texture->format) {
        case SDL_PIXELFORMAT_RGB24: {
            if(!SDL_ConvertPixels(
                   m_camera.texture->w, m_camera.texture->h,
                   surface->format, surface->pixels, surface->pitch,
                   m_camera.texture->format, m_camera.pixels.data(), m_camera.pitch
               )) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error converting pixels: %s", SDL_GetError());
                break;
            }

            SDL_ReleaseCameraFrame(m_camera.device, surface);

            void* pixels = nullptr;
            int pitch    = 0;

            SDL_LockTexture(m_camera.texture, nullptr, &pixels, &pitch);
            SDL_memmove(pixels, m_camera.pixels.data(), m_camera.pixels.size());
            SDL_UnlockTexture(m_camera.texture);

            return;
        }
        default:
            SDL_UpdateTexture(m_camera.texture, nullptr, surface->pixels, surface->pitch);
            break;
        }
    }

    SDL_ReleaseCameraFrame(m_camera.device, surface);
}