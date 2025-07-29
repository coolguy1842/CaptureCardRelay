#include <application.hpp>
#include <settings.hpp>

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
    registerEventHandler(SDL_EVENT_CAMERA_DEVICE_APPROVED, [this](SDL_Event* event, void*) {
        if(event->cdevice.which != SDL_GetCameraID(m_camera.device)) {
            return true;
        }

        m_camera.approved = true;

        SDL_GetCameraFormat(m_camera.device, &m_camera.spec);
        m_camera.texture = SDL_CreateTexture(m_renderer, m_camera.spec.format, SDL_TEXTUREACCESS_STREAMING, m_camera.spec.width, m_camera.spec.height);

        SDL_SetWindowSize(m_window, m_camera.spec.width, m_camera.spec.height);
        SDL_GetWindowSize(m_window, &m_width, &m_height);

        return true;
    });
}

void Application::openCamera() {
    if(m_camera.device != nullptr) {
        closeCamera();
    }

    SDL_CameraID camID = Settings::get()->getSelectedCamera();
    if(camID == 0) {
        camID = m_cameras[0];
    }

    m_camera.approved = false;
    m_camera.device   = SDL_OpenCamera(camID, nullptr);
    if(m_camera.device == nullptr) {
        SDL_Log("Couldn't open the selected camera: %s", SDL_GetError());

        setShouldQuit(true);
        return;
    }

    SDL_Log("Opened camera: %s", SDL_GetCameraName(camID));
}

void Application::closeCamera() {
    if(m_camera.device != nullptr) {
        SDL_CloseCamera(m_camera.device);
    }

    if(m_camera.texture != nullptr) {
        SDL_DestroyTexture(m_camera.texture);
    }

    m_camera.spec = {};
}