#include <SDL3/SDL_camera.h>
#include <SDL3/SDL_pixels.h>

#include <application.hpp>
#include <set>
#include <settings.hpp>
#include <unordered_map>

const char* formatName(SDL_PixelFormat format) {
    switch(format) {
    case SDL_PIXELFORMAT_UNKNOWN:       return "SDL_PIXELFORMAT_UNKNOWN";
    case SDL_PIXELFORMAT_INDEX1LSB:     return "SDL_PIXELFORMAT_INDEX1LSB";
    case SDL_PIXELFORMAT_INDEX1MSB:     return "SDL_PIXELFORMAT_INDEX1MSB";
    case SDL_PIXELFORMAT_INDEX2LSB:     return "SDL_PIXELFORMAT_INDEX2LSB";
    case SDL_PIXELFORMAT_INDEX2MSB:     return "SDL_PIXELFORMAT_INDEX2MSB";
    case SDL_PIXELFORMAT_INDEX4LSB:     return "SDL_PIXELFORMAT_INDEX4LSB";
    case SDL_PIXELFORMAT_INDEX4MSB:     return "SDL_PIXELFORMAT_INDEX4MSB";
    case SDL_PIXELFORMAT_INDEX8:        return "SDL_PIXELFORMAT_INDEX8";
    case SDL_PIXELFORMAT_RGB332:        return "SDL_PIXELFORMAT_RGB332";
    case SDL_PIXELFORMAT_XRGB4444:      return "SDL_PIXELFORMAT_XRGB4444";
    case SDL_PIXELFORMAT_XBGR4444:      return "SDL_PIXELFORMAT_XBGR4444";
    case SDL_PIXELFORMAT_XRGB1555:      return "SDL_PIXELFORMAT_XRGB1555";
    case SDL_PIXELFORMAT_XBGR1555:      return "SDL_PIXELFORMAT_XBGR1555";
    case SDL_PIXELFORMAT_ARGB4444:      return "SDL_PIXELFORMAT_ARGB4444";
    case SDL_PIXELFORMAT_RGBA4444:      return "SDL_PIXELFORMAT_RGBA4444";
    case SDL_PIXELFORMAT_ABGR4444:      return "SDL_PIXELFORMAT_ABGR4444";
    case SDL_PIXELFORMAT_BGRA4444:      return "SDL_PIXELFORMAT_BGRA4444";
    case SDL_PIXELFORMAT_ARGB1555:      return "SDL_PIXELFORMAT_ARGB1555";
    case SDL_PIXELFORMAT_RGBA5551:      return "SDL_PIXELFORMAT_RGBA5551";
    case SDL_PIXELFORMAT_ABGR1555:      return "SDL_PIXELFORMAT_ABGR1555";
    case SDL_PIXELFORMAT_BGRA5551:      return "SDL_PIXELFORMAT_BGRA5551";
    case SDL_PIXELFORMAT_RGB565:        return "SDL_PIXELFORMAT_RGB565";
    case SDL_PIXELFORMAT_BGR565:        return "SDL_PIXELFORMAT_BGR565";
    case SDL_PIXELFORMAT_RGB24:         return "SDL_PIXELFORMAT_RGB24";
    case SDL_PIXELFORMAT_BGR24:         return "SDL_PIXELFORMAT_BGR24";
    case SDL_PIXELFORMAT_XRGB8888:      return "SDL_PIXELFORMAT_XRGB8888";
    case SDL_PIXELFORMAT_RGBX8888:      return "SDL_PIXELFORMAT_RGBX8888";
    case SDL_PIXELFORMAT_XBGR8888:      return "SDL_PIXELFORMAT_XBGR8888";
    case SDL_PIXELFORMAT_BGRX8888:      return "SDL_PIXELFORMAT_BGRX8888";
    case SDL_PIXELFORMAT_ARGB8888:      return "SDL_PIXELFORMAT_ARGB8888";
    case SDL_PIXELFORMAT_RGBA8888:      return "SDL_PIXELFORMAT_RGBA8888";
    case SDL_PIXELFORMAT_ABGR8888:      return "SDL_PIXELFORMAT_ABGR8888";
    case SDL_PIXELFORMAT_BGRA8888:      return "SDL_PIXELFORMAT_BGRA8888";
    case SDL_PIXELFORMAT_XRGB2101010:   return "SDL_PIXELFORMAT_XRGB2101010";
    case SDL_PIXELFORMAT_XBGR2101010:   return "SDL_PIXELFORMAT_XBGR2101010";
    case SDL_PIXELFORMAT_ARGB2101010:   return "SDL_PIXELFORMAT_ARGB2101010";
    case SDL_PIXELFORMAT_ABGR2101010:   return "SDL_PIXELFORMAT_ABGR2101010";
    case SDL_PIXELFORMAT_RGB48:         return "SDL_PIXELFORMAT_RGB48";
    case SDL_PIXELFORMAT_BGR48:         return "SDL_PIXELFORMAT_BGR48";
    case SDL_PIXELFORMAT_RGBA64:        return "SDL_PIXELFORMAT_RGBA64";
    case SDL_PIXELFORMAT_ARGB64:        return "SDL_PIXELFORMAT_ARGB64";
    case SDL_PIXELFORMAT_BGRA64:        return "SDL_PIXELFORMAT_BGRA64";
    case SDL_PIXELFORMAT_ABGR64:        return "SDL_PIXELFORMAT_ABGR64";
    case SDL_PIXELFORMAT_RGB48_FLOAT:   return "SDL_PIXELFORMAT_RGB48_FLOAT";
    case SDL_PIXELFORMAT_BGR48_FLOAT:   return "SDL_PIXELFORMAT_BGR48_FLOAT";
    case SDL_PIXELFORMAT_RGBA64_FLOAT:  return "SDL_PIXELFORMAT_RGBA64_FLOAT";
    case SDL_PIXELFORMAT_ARGB64_FLOAT:  return "SDL_PIXELFORMAT_ARGB64_FLOAT";
    case SDL_PIXELFORMAT_BGRA64_FLOAT:  return "SDL_PIXELFORMAT_BGRA64_FLOAT";
    case SDL_PIXELFORMAT_ABGR64_FLOAT:  return "SDL_PIXELFORMAT_ABGR64_FLOAT";
    case SDL_PIXELFORMAT_RGB96_FLOAT:   return "SDL_PIXELFORMAT_RGB96_FLOAT";
    case SDL_PIXELFORMAT_BGR96_FLOAT:   return "SDL_PIXELFORMAT_BGR96_FLOAT";
    case SDL_PIXELFORMAT_RGBA128_FLOAT: return "SDL_PIXELFORMAT_RGBA128_FLOAT";
    case SDL_PIXELFORMAT_ARGB128_FLOAT: return "SDL_PIXELFORMAT_ARGB128_FLOAT";
    case SDL_PIXELFORMAT_BGRA128_FLOAT: return "SDL_PIXELFORMAT_BGRA128_FLOAT";
    case SDL_PIXELFORMAT_ABGR128_FLOAT: return "SDL_PIXELFORMAT_ABGR128_FLOAT";
    case SDL_PIXELFORMAT_YV12:          return "SDL_PIXELFORMAT_YV12";
    case SDL_PIXELFORMAT_IYUV:          return "SDL_PIXELFORMAT_IYUV";
    case SDL_PIXELFORMAT_YUY2:          return "SDL_PIXELFORMAT_YUY2";
    case SDL_PIXELFORMAT_UYVY:          return "SDL_PIXELFORMAT_UYVY";
    case SDL_PIXELFORMAT_YVYU:          return "SDL_PIXELFORMAT_YVYU";
    case SDL_PIXELFORMAT_NV12:          return "SDL_PIXELFORMAT_NV12";
    case SDL_PIXELFORMAT_NV21:          return "SDL_PIXELFORMAT_NV21";
    case SDL_PIXELFORMAT_P010:          return "SDL_PIXELFORMAT_P010";
    case SDL_PIXELFORMAT_EXTERNAL_OES:  return "SDL_PIXELFORMAT_EXTERNAL_OES";
    case SDL_PIXELFORMAT_MJPG:          return "SDL_PIXELFORMAT_MJPG";
    default:                            return "unknown";
    }
}

void Application::initCameras() {
    int cameraCount       = 0;
    SDL_CameraID* cameras = SDL_GetCameras(&cameraCount);

    if(cameras == nullptr || cameraCount <= 0) {
        SDL_Log("Couldn't enumerate camera devices or none plugged in: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    for(int i = 0; i < cameraCount; i++) {
        m_cameras.push_back(cameras[i]);
    }

    SDL_free(cameras);
}

static std::unordered_map<SDL_PixelFormat, uint16_t> formatScores = {
    { SDL_PIXELFORMAT_UNKNOWN,       0  },
    { SDL_PIXELFORMAT_MJPG,          1  },
    { SDL_PIXELFORMAT_EXTERNAL_OES,  2  },
    { SDL_PIXELFORMAT_INDEX1LSB,     3  },
    { SDL_PIXELFORMAT_INDEX1MSB,     4  },
    { SDL_PIXELFORMAT_INDEX2LSB,     5  },
    { SDL_PIXELFORMAT_INDEX2MSB,     6  },
    { SDL_PIXELFORMAT_INDEX4LSB,     7  },
    { SDL_PIXELFORMAT_INDEX4MSB,     8  },
    { SDL_PIXELFORMAT_INDEX8,        9  },
    { SDL_PIXELFORMAT_RGB332,        10 },
    { SDL_PIXELFORMAT_XRGB4444,      11 },
    { SDL_PIXELFORMAT_XBGR4444,      12 },
    { SDL_PIXELFORMAT_XRGB1555,      13 },
    { SDL_PIXELFORMAT_XBGR1555,      14 },
    { SDL_PIXELFORMAT_ARGB4444,      15 },
    { SDL_PIXELFORMAT_RGBA4444,      16 },
    { SDL_PIXELFORMAT_ABGR4444,      17 },
    { SDL_PIXELFORMAT_BGRA4444,      18 },
    { SDL_PIXELFORMAT_ARGB1555,      19 },
    { SDL_PIXELFORMAT_RGBA5551,      20 },
    { SDL_PIXELFORMAT_ABGR1555,      21 },
    { SDL_PIXELFORMAT_BGRA5551,      22 },
    { SDL_PIXELFORMAT_RGB565,        23 },
    { SDL_PIXELFORMAT_BGR565,        24 },
    { SDL_PIXELFORMAT_RGB24,         25 },
    { SDL_PIXELFORMAT_BGR24,         26 },
    { SDL_PIXELFORMAT_XRGB8888,      27 },
    { SDL_PIXELFORMAT_RGBX8888,      28 },
    { SDL_PIXELFORMAT_XBGR8888,      29 },
    { SDL_PIXELFORMAT_BGRX8888,      30 },
    { SDL_PIXELFORMAT_ARGB8888,      31 },
    { SDL_PIXELFORMAT_RGBA8888,      32 },
    { SDL_PIXELFORMAT_ABGR8888,      33 },
    { SDL_PIXELFORMAT_BGRA8888,      34 },
    { SDL_PIXELFORMAT_XRGB2101010,   35 },
    { SDL_PIXELFORMAT_XBGR2101010,   36 },
    { SDL_PIXELFORMAT_ARGB2101010,   37 },
    { SDL_PIXELFORMAT_ABGR2101010,   38 },
    { SDL_PIXELFORMAT_RGB48,         39 },
    { SDL_PIXELFORMAT_BGR48,         40 },
    { SDL_PIXELFORMAT_RGBA64,        41 },
    { SDL_PIXELFORMAT_ARGB64,        42 },
    { SDL_PIXELFORMAT_BGRA64,        43 },
    { SDL_PIXELFORMAT_ABGR64,        44 },
    { SDL_PIXELFORMAT_RGB48_FLOAT,   45 },
    { SDL_PIXELFORMAT_BGR48_FLOAT,   46 },
    { SDL_PIXELFORMAT_RGBA64_FLOAT,  47 },
    { SDL_PIXELFORMAT_ARGB64_FLOAT,  48 },
    { SDL_PIXELFORMAT_BGRA64_FLOAT,  49 },
    { SDL_PIXELFORMAT_ABGR64_FLOAT,  50 },
    { SDL_PIXELFORMAT_RGB96_FLOAT,   51 },
    { SDL_PIXELFORMAT_BGR96_FLOAT,   52 },
    { SDL_PIXELFORMAT_RGBA128_FLOAT, 53 },
    { SDL_PIXELFORMAT_ARGB128_FLOAT, 54 },
    { SDL_PIXELFORMAT_BGRA128_FLOAT, 55 },
    { SDL_PIXELFORMAT_ABGR128_FLOAT, 56 },
    { SDL_PIXELFORMAT_YV12,          57 },
    { SDL_PIXELFORMAT_IYUV,          58 },
    { SDL_PIXELFORMAT_YUY2,          59 },
    { SDL_PIXELFORMAT_UYVY,          60 },
    { SDL_PIXELFORMAT_YVYU,          61 },
    { SDL_PIXELFORMAT_NV12,          62 },
    { SDL_PIXELFORMAT_NV21,          63 },
    { SDL_PIXELFORMAT_P010,          64 },
};

void Application::openCamera() {
    if(m_cameraData->camera.device != nullptr) {
        closeCamera();
    }

    SDL_CameraID camID = Settings::get()->getSelectedCamera();
    if(camID == 0) {
        camID = m_cameras[0];
    }

    int numFormats           = 0;
    SDL_CameraSpec** formats = SDL_GetCameraSupportedFormats(camID, &numFormats);
    if(numFormats <= 0 || formats == nullptr) {
        return;
    }

    auto cmp = [](SDL_CameraSpec* a, SDL_CameraSpec* b) {
        if(formatScores[a->format] != formatScores[b->format]) {
            return formatScores[a->format] > formatScores[b->format];
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

        return false;
    };

    std::set<SDL_CameraSpec*, decltype(cmp)> specs;
    for(int i = 0; i < numFormats; i++) {
        specs.emplace(formats[i]);
    }

    SDL_CameraSpec* spec        = *specs.begin();
    m_cameraData->camera.device = SDL_OpenCamera(camID, spec);
}

void Application::closeCamera() {
    if(m_cameraData->camera.device != nullptr) {
        SDL_CloseCamera(m_cameraData->camera.device);
    }

    if(m_cameraData->camera.texture != nullptr) {
        SDL_DestroyTexture(m_cameraData->camera.texture);
    }
}