#include <application.hpp>
#include <format>
#include <mutex>
#include <set>
#include <vector>

void Application::initCameras() {
    m_cameras.clear();

    int cameraCount       = 0;
    SDL_CameraID* cameras = SDL_GetCameras(&cameraCount);

    if(cameras == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't enumerate camera devices: %s", SDL_GetError());
        setShouldQuit(true);

        return;
    }

    for(int i = 0; i < cameraCount; i++) {
        SDL_CameraID id = cameras[i];

        const char* name = SDL_GetCameraName(id);
        if(name == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get name of camera with id: %d: %s", id, SDL_GetError());
            name = "";
        }

        m_cameras.push_back({ id, name });
    }
}

uint16_t getColorScore(const SDL_Colorspace& color) {
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
};

uint16_t getFormatScore(const SDL_PixelFormat& format) {
    // dont remember how i came to these
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
};

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

    int numFormats           = 0;
    SDL_CameraSpec** formats = SDL_GetCameraSupportedFormats(camID, &numFormats);
    if(numFormats <= 0 || formats == nullptr) {
        return;
    }

    SDL_Colorspace preferredColorspace = m_settings.getPreferredColorspace();
    SDL_PixelFormat preferredFormat    = m_settings.getPreferredPixelFormat();

    auto cmp = [preferredColorspace, preferredFormat](SDL_CameraSpec* a, SDL_CameraSpec* b) {
        if(preferredColorspace != SDL_COLORSPACE_UNKNOWN && a->colorspace != b->colorspace) {
            if(a->colorspace == preferredColorspace) {
                return true;
            }
            else if(b->colorspace == preferredColorspace) {
                return false;
            }
        }

        if(preferredFormat != SDL_PIXELFORMAT_UNKNOWN && a->format != b->format) {
            if(a->format == preferredFormat) {
                return true;
            }
            else if(b->format == preferredFormat) {
                return false;
            }
        }

        if(a->width != b->width) {
            return a->width > b->width;
        }

        if(a->height != b->height) {
            return a->height > b->height;
        }

        if(a->framerate_numerator / a->framerate_denominator != b->framerate_numerator / b->framerate_denominator) {
            return a->framerate_numerator / a->framerate_denominator > b->framerate_numerator / b->framerate_denominator;
        }

        if(getColorScore(a->colorspace) != getColorScore(b->colorspace)) {
            return getColorScore(a->colorspace) > getColorScore(b->colorspace);
        }

        if(getFormatScore(a->format) != getFormatScore(b->format)) {
            return getFormatScore(a->format) > getFormatScore(b->format);
        }

        return false;
    };

    std::set<SDL_CameraSpec*, decltype(cmp)> specs(cmp);
    for(int i = 0; i < numFormats; i++) {
        specs.emplace(formats[i]);
    }

    SDL_Log("Possible Camera Specs:");
    for(SDL_CameraSpec* spec : specs) {
        SDL_Log("  %dx%d@%0.2f - Format: %s - Colorspace: %s", spec->width, spec->height, spec->framerate_numerator / static_cast<float>(spec->framerate_denominator), pixelFormatName(spec->format), colorspaceName(spec->colorspace));
    }

    SDL_CameraSpec* spec = *specs.begin();
    const char* name     = SDL_GetCameraName(camID);
    if(name == nullptr) {
        name = "(null)";
    }

    SDL_Log("\n");
    SDL_Log("Opening camera: %s", name);
    auto lock = std::unique_lock(m_cameraData->camera.mutex);

    m_cameraData->camera.approved = false;
    m_currentCamera               = { camID, name };

    m_cameraData->camera.device = SDL_OpenCamera(camID, spec);
    if(m_cameraData->camera.device == nullptr) {
        closeCamera(false);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Opening camera: %s", SDL_GetError());
    }
}

void Application::closeCamera(bool shouldLock) {
    std::unique_lock<std::mutex> lock;
    if(shouldLock) {
        lock = std::unique_lock(m_cameraData->camera.mutex);
    }

    m_cameraData->camera.approved = false;
    m_currentCamera               = { .id = 0, .name = "(null)" };

    if(m_cameraData->camera.device != nullptr) {
        SDL_CloseCamera(m_cameraData->camera.device);
        m_cameraData->camera.device = nullptr;
    }

    if(m_cameraData->camera.texture != nullptr) {
        SDL_DestroyTexture(m_cameraData->camera.texture);
        m_cameraData->camera.texture = nullptr;
    }
}

void Application::setCamera(Application::CameraInfo info) {
    m_settings.setSelectedCamera(info.id);

    const char* cameraName = "(null)";
    if(info.id != 0) {
        if(info.name != nullptr) {
            cameraName = info.name;
        }

        if(m_currentCamera.id != info.id) {
            openCamera();
        }
    }
    else {
        closeCamera();
    }

    changeStatus(std::format("Camera: {}", cameraName), std::chrono::milliseconds(1500));
}