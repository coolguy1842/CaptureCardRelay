#include <clay_renderer_SDL3.hpp>
#include <cstddef>
#include <cstring>
#include <map>
#include <stack>
#include <unordered_map>

// for windows compiling
CustomElementData::~CustomElementData() {}

Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    std::vector<TTF_Font*>& fonts = *reinterpret_cast<std::vector<TTF_Font*>*>(userData);
    TTF_Font* font                = fonts[config->fontId];
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if(!TTF_GetStringSize(font, text.chars, static_cast<size_t>(text.length), &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return Clay_Dimensions{
        (float)width,
        (float)height,
    };
}

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static size_t NUM_CIRCLE_SEGMENTS = 16;

void SDL_Clay_RenderArc(Clay_SDL3RendererData* rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness, const Clay_Color color) {
    SDL_SetRenderDrawColor(rendererData->renderer, color.r, color.g, color.b, color.a);

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd   = endAngle * (SDL_PI_F / 180.0f);

    const size_t numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (radius * 1.5f)); // increase circle segments for larger circles, 1.5 is arbitrary.

    const float angleStep     = (radEnd - radStart) / static_cast<float>(numCircleSegments);
    const float thicknessStep = 0.4f; // arbitrary value to avoid overlapping lines. Changing THICKNESS_STEP or numCircleSegments might cause artifacts.

    for(float t = thicknessStep; t < thickness - thicknessStep; t += thicknessStep) {
        std::vector<SDL_FPoint> points(numCircleSegments + 1);
        const float clampedRadius = SDL_max(radius - t, 1.0f);

        for(size_t i = 0; i <= numCircleSegments; i++) {
            const float angle = radStart + i * angleStep;
            points[i]         = SDL_FPoint{
                SDL_roundf(center.x + SDL_cosf(angle) * clampedRadius),
                SDL_roundf(center.y + SDL_sinf(angle) * clampedRadius)
            };
        }

        SDL_RenderLines(rendererData->renderer, points.data(), points.size());
    }
}

void SDL_Clay_RenderFilledArc(Clay_SDL3RendererData* rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const Clay_Color _color) {
    const SDL_FColor color = { _color.r / 255, _color.g / 255, _color.b / 255, _color.a / 255 };
    size_t indexCount = 0, vertexCount = 0;

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd   = endAngle * (SDL_PI_F / 180.0f);

    const size_t numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (radius * 1.5f));

    const float angleStep = (radEnd - radStart) / (float)numCircleSegments;

    const size_t totalVertices = 2 + numCircleSegments;
    const size_t totalIndices  = 3 + (numCircleSegments * 3);

    std::vector<SDL_Vertex> vertices(totalVertices);
    std::vector<int> indices(totalIndices);

    const float clampedRadius = SDL_max(radius, 1.0f);
    vertices[vertexCount++]   = { .position = { center.x, center.y }, .color = color, .tex_coord = { 0, 0 } };

    for(size_t i = 0; i <= numCircleSegments; i++) {
        const float angle = radStart + i * angleStep;
        float x           = center.x + SDL_cosf(angle) * clampedRadius;
        float y           = center.y + SDL_sinf(angle) * clampedRadius;

        vertices[vertexCount++] = { .position = { x, y }, .color = color, .tex_coord = { 0, 0 } };
        if(vertexCount > 1) {
            indices[indexCount++] = 0;
            indices[indexCount++] = vertexCount - 1;
            indices[indexCount++] = vertexCount - 2;
        }
    }

    SDL_RenderGeometry(rendererData->renderer, NULL, vertices.data(), vertexCount, indices.data(), indexCount);
}

void SDL_Clay_RenderFillRoundedRect(Clay_SDL3RendererData* rendererData, const SDL_FRect rect, const Clay_CornerRadius radius, const Clay_Color color) {
    SDL_SetRenderDrawColor(rendererData->renderer, color.r, color.g, color.b, color.a);

    const float minRadius                = CLAY__MIN(rect.w, rect.h) / 2.0f;
    const Clay_CornerRadius clampedRadii = {
        .topLeft     = CLAY__MIN(radius.topLeft, minRadius),
        .topRight    = CLAY__MIN(radius.topRight, minRadius),
        .bottomLeft  = CLAY__MIN(radius.bottomLeft, minRadius),
        .bottomRight = CLAY__MIN(radius.bottomRight, minRadius)
    };

    const float top    = CLAY__MAX(clampedRadii.topLeft, clampedRadii.topRight);
    const float bottom = CLAY__MAX(clampedRadii.bottomLeft, clampedRadii.bottomRight);

    const float topX    = rect.x + clampedRadii.topLeft;
    const float bottomX = rect.x + clampedRadii.bottomLeft;
    const float y       = rect.y + top;

    const float topWidth    = rect.w - (clampedRadii.topLeft + clampedRadii.topRight);
    const float bottomWidth = rect.w - (clampedRadii.bottomLeft + clampedRadii.bottomRight);

    const float height = rect.h - (top + bottom);

    SDL_FRect topRect    = { .x = topX, .y = rect.y, .w = topWidth, .h = top };
    SDL_FRect middleRect = { .x = rect.x, .y = y, .w = rect.w, .h = height };
    SDL_FRect bottomRect = { .x = bottomX, .y = y + height, .w = bottomWidth, .h = bottom };

    if(clampedRadii.topLeft > 0.0f) SDL_Clay_RenderFilledArc(rendererData, { topX, y }, clampedRadii.topLeft, 180.0f, 270.0f, color); // top left
    SDL_RenderFillRect(rendererData->renderer, &topRect);
    if(clampedRadii.topRight > 0.0f) SDL_Clay_RenderFilledArc(rendererData, { topX + topWidth, y }, clampedRadii.topRight, 270.0f, 360.0f, color); // top right

    SDL_RenderFillRect(rendererData->renderer, &middleRect);

    if(clampedRadii.bottomLeft > 0.0f) SDL_Clay_RenderFilledArc(rendererData, { bottomX, y + height }, clampedRadii.bottomLeft, 90.0f, 180.0f, color); // bottom left
    SDL_RenderFillRect(rendererData->renderer, &bottomRect);
    if(clampedRadii.bottomRight > 0.0f) SDL_Clay_RenderFilledArc(rendererData, { bottomX + bottomWidth, y + height }, clampedRadii.bottomRight, 0.0f, 90.0f, color); // bottom right
}

inline bool SDL_Clay_FRectEqual(const SDL_FRect& a, const SDL_FRect& b) {
    return a.x == b.x &&
           a.y == b.y &&
           a.w == b.w &&
           a.h == b.h;
}

static std::map<int, const char*> idToName = {
    { CLAY_ID("Settings").id, "Settings" },
    { CLAY_ID("CamerasContainer").id, "Cameras" },
    { CLAY_ID("RecordingDevicesContainer").id, "Recording Devices" },
    { CLAY_ID("Volume").id, "Volume" },
    { CLAY_ID("VolumeSlider").id, "Volume Slider" },
    { CLAY_ID("VolumeSliderHandle").id, "Volume Slider Handle" },
};

// Clay_ElementID.id: text
static std::unordered_map<uint32_t, TTF_Text*> s_textMap;
static std::stack<SDL_Rect> s_scissorStack;

void SDL_Clay_Exit() {
    while(!s_scissorStack.empty()) {
        s_scissorStack.pop();
    }

    s_textMap.clear();
}
void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData* rendererData, Clay_RenderCommandArray* rcommands) {
    for(int32_t i = 0; i < rcommands->length; i++) {
        Clay_RenderCommand* rcmd            = Clay_RenderCommandArray_Get(rcommands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect                = { (float)((int)bounding_box.x), (float)((int)bounding_box.y), (float)((int)bounding_box.width), (float)((int)bounding_box.height) };

        switch(rcmd->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleRenderData* config = &rcmd->renderData.rectangle;
            SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(rendererData->renderer, config->backgroundColor.r, config->backgroundColor.g, config->backgroundColor.b, config->backgroundColor.a);

            if(
                config->cornerRadius.topLeft > 0 ||
                config->cornerRadius.topRight > 0 ||
                config->cornerRadius.bottomLeft > 0 ||
                config->cornerRadius.bottomRight > 0
            ) {
                SDL_Clay_RenderFillRoundedRect(rendererData, rect, config->cornerRadius, config->backgroundColor);
                break;
            }

            SDL_RenderFillRect(rendererData->renderer, &rect);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_TextRenderData* config = &rcmd->renderData.text;

            TTF_Font* font = rendererData->fonts[config->fontId];
            TTF_SetFontSize(font, config->fontSize);

            auto it = s_textMap.find(rcmd->id);
            if(it == s_textMap.end()) {
                TTF_Text* text = TTF_CreateText(rendererData->textEngine, font, config->stringContents.chars, static_cast<size_t>(config->stringContents.length));
                TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);

                it = s_textMap.emplace(rcmd->id, text).first;
            }
            else {
                TTF_Text* text = it->second;
                TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
                TTF_SetTextString(it->second, config->stringContents.chars, static_cast<size_t>(config->stringContents.length));
            }

            TTF_DrawRendererText(it->second, rect.x, rect.y);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            Clay_BorderRenderData* config = &rcmd->renderData.border;

            const float minRadius                = SDL_min(rect.w, rect.h) / 2.0f;
            const Clay_CornerRadius clampedRadii = {
                .topLeft     = SDL_min(config->cornerRadius.topLeft, minRadius),
                .topRight    = SDL_min(config->cornerRadius.topRight, minRadius),
                .bottomLeft  = SDL_min(config->cornerRadius.bottomLeft, minRadius),
                .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius)
            };

            // edges
            SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
            if(config->width.left > 0) {
                const float starting_y = rect.y + clampedRadii.topLeft - 1;
                const float length     = rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft + 1;
                SDL_FRect line         = { rect.x, starting_y, (float)config->width.left, length };
                SDL_RenderFillRect(rendererData->renderer, &line);
            }

            if(config->width.right > 0) {
                const float starting_x = rect.x + rect.w - (float)config->width.right - 0.5f;
                const float starting_y = rect.y + clampedRadii.topRight - 1;
                const float length     = rect.h - clampedRadii.topRight - clampedRadii.bottomRight + 2;
                SDL_FRect line         = { starting_x, starting_y, (float)config->width.right, length };
                SDL_RenderFillRect(rendererData->renderer, &line);
            }

            if(config->width.top > 0) {
                const float starting_x = rect.x + clampedRadii.topLeft - 1;
                const float length     = rect.w - clampedRadii.topLeft - clampedRadii.topRight + 1;
                SDL_FRect line         = { starting_x, rect.y, length, (float)config->width.top };
                SDL_RenderFillRect(rendererData->renderer, &line);
            }

            if(config->width.bottom > 0) {
                const float starting_x = rect.x + clampedRadii.bottomLeft - 1;
                const float starting_y = rect.y + rect.h - (float)config->width.bottom;
                const float length     = rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight + 2;
                SDL_FRect line         = { starting_x, starting_y, length, (float)config->width.bottom };
                SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
                SDL_RenderFillRect(rendererData->renderer, &line);
            }

            // corners
            if(config->cornerRadius.topLeft > 0) {
                const float centerX = rect.x + clampedRadii.topLeft - 0.5f;
                const float centerY = rect.y + clampedRadii.topLeft - 0.5f;
                SDL_Clay_RenderArc(rendererData, { centerX, centerY }, clampedRadii.topLeft, 180.0f, 270.0f, config->width.top, config->color);
            }

            if(config->cornerRadius.topRight > 0) {
                const float centerX = rect.x + rect.w - clampedRadii.topRight - 1.5f;
                const float centerY = rect.y + clampedRadii.topRight - 0.5f;
                SDL_Clay_RenderArc(rendererData, { centerX, centerY }, clampedRadii.topRight, 270.0f, 360.0f, config->width.top, config->color);
            }

            if(config->cornerRadius.bottomLeft > 0) {
                const float centerX = rect.x + clampedRadii.bottomLeft - 0.5f;
                const float centerY = rect.y + rect.h - clampedRadii.bottomLeft - 0.5f;
                SDL_Clay_RenderArc(rendererData, { centerX, centerY }, clampedRadii.bottomLeft, 90.0f, 180.0f, config->width.bottom, config->color);
            }

            if(config->cornerRadius.bottomRight > 0) {
                const float centerX = rect.x + rect.w - clampedRadii.bottomRight - 1.5f;
                const float centerY = rect.y + rect.h - clampedRadii.bottomRight - 0.5f;
                SDL_Clay_RenderArc(rendererData, { centerX, centerY }, clampedRadii.bottomRight, 0.0f, 90.0f, config->width.bottom, config->color);
            }

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            Clay_BoundingBox bounds = rcmd->boundingBox;

            if(!s_scissorStack.empty()) {
                SDL_Rect parent = s_scissorStack.top();

                int minX = SDL_max(parent.x, bounds.x);
                int maxX = SDL_min(parent.x + parent.w, bounds.x + bounds.width);

                if(maxX - minX < 1) {
                    s_scissorStack.push({ 0, 0, 0, 0 });
                    goto setRect;
                }

                int minY = SDL_max(parent.y, bounds.y);
                int maxY = SDL_min(parent.y + parent.h, bounds.y + bounds.height);
                if(maxY - minY < 1) {
                    s_scissorStack.push({ 0, 0, 0, 0 });
                    goto setRect;
                }

                SDL_Rect scissor = {
                    .x = minX,
                    .y = minY,
                    .w = maxX - minX,
                    .h = SDL_max(1, maxY - minY),
                };

                s_scissorStack.push(scissor);
            }
            else {
                s_scissorStack.push({ (int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height });
            }

        setRect:
            SDL_SetRenderClipRect(rendererData->renderer, &s_scissorStack.top());

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
            if(s_scissorStack.empty()) {
                break;
            }

            s_scissorStack.pop();
            SDL_SetRenderClipRect(rendererData->renderer, !s_scissorStack.empty() ? &s_scissorStack.top() : nullptr);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
            SDL_Texture* texture = (SDL_Texture*)rcmd->renderData.image.imageData;

            const SDL_FRect dest = { rect.x, rect.y, rect.w, rect.h };
            SDL_RenderTexture(rendererData->renderer, texture, NULL, &dest);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            CustomElementData* data = reinterpret_cast<CustomElementData*>(rcmd->renderData.custom.customData);
            if(data == nullptr) {
                continue;
            }

            switch(data->type) {
            case CUSTOM_ELEMENT_TYPE_CAMERA: {
                SDL_SetRenderDrawColor(rendererData->renderer, rcmd->renderData.custom.backgroundColor.r, rcmd->renderData.custom.backgroundColor.g, rcmd->renderData.custom.backgroundColor.b, rcmd->renderData.custom.backgroundColor.a);
                SDL_RenderFillRect(rendererData->renderer, &rect);

                CameraData& camera   = data->camera;
                SDL_Texture*& tex    = camera.texture;
                SDL_CameraSpec& spec = camera.spec;

                auto lock = std::unique_lock(camera.mutex);
                if(camera.device == nullptr || !camera.approved) {
                cleanupCamera:
                    camera.__prevDisplayMode = static_cast<CameraDisplayMode>(-1);
                    camera.__prevRect        = { 0, 0, 0, 0 };
                    camera.__displayRect     = { 0, 0, 0, 0 };

                    if(camera.__pixels != nullptr) {
                        SDL_free(camera.__pixels);
                        camera.__pixels = nullptr;
                    }

                    if(tex != nullptr) {
                        SDL_DestroyTexture(tex);
                        tex = nullptr;
                    }

                    break;
                }

                if(
                    tex == nullptr || tex->w != spec.width || tex->h != spec.height ||
                    (camera.textureFormat == SDL_PIXELFORMAT_UNKNOWN && tex->format != spec.format) ||
                    (camera.textureFormat != SDL_PIXELFORMAT_UNKNOWN && tex->format != camera.textureFormat)
                ) {
                    camera.__prevDisplayMode = static_cast<CameraDisplayMode>(-1);

                    if(camera.__pixels != nullptr) {
                        SDL_free(camera.__pixels);
                        camera.__pixels = nullptr;
                    }

                    if(tex != nullptr) {
                        SDL_DestroyTexture(tex);
                        tex = nullptr;
                    }

                    SDL_PropertiesID props = SDL_CreateProperties();
                    if(props == 0) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create properties for texture: %s\n", SDL_GetError());
                        goto cleanupCamera;
                    }

                    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, spec.width);
                    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, spec.height);
                    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
                    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, spec.colorspace);

                    SDL_PixelFormat format = camera.textureFormat;
                    // use cameras pixel format
                    if(format == SDL_PIXELFORMAT_UNKNOWN) {
                        format = spec.format;
                    }

                    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, format);
                    if((tex = SDL_CreateTextureWithProperties(rendererData->renderer, props)) == nullptr) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s\n", SDL_GetError());
                        SDL_DestroyProperties(props);

                        goto cleanupCamera;
                    }

                    SDL_DestroyProperties(props);
                    camera.__pitch      = static_cast<size_t>(((tex->w * SDL_BYTESPERPIXEL(tex->format)) + 3) & ~3);
                    camera.__pixelsSize = static_cast<size_t>(tex->h) * camera.__pitch;
                    camera.__pixels     = SDL_malloc(camera.__pixelsSize);

                    if(camera.__pixels == nullptr) {
                        goto cleanupCamera;
                    }
                }

                SDL_Surface* surface = SDL_AcquireCameraFrame(camera.device, NULL);
                if(surface != nullptr) {
                    switch(tex->format) {
                    case SDL_PIXELFORMAT_RGB24: {
                        if(!SDL_ConvertPixels(
                               tex->w, tex->h,
                               surface->format, surface->pixels, surface->pitch,
                               tex->format, camera.__pixels, camera.__pitch
                           )) {
                            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error converting pixels: %s\n", SDL_GetError());
                        }

                        SDL_ReleaseCameraFrame(camera.device, surface);

                        void* pixels;
                        int pitch;

                        SDL_LockTexture(tex, NULL, &pixels, &pitch);
                        SDL_memmove(pixels, camera.__pixels, camera.__pixelsSize);
                        SDL_UnlockTexture(tex);
                        break;
                    }
                    default:
                        SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
                        SDL_ReleaseCameraFrame(camera.device, surface);
                        break;
                    }
                }

                lock.unlock();
                if(!SDL_Clay_FRectEqual(camera.__prevRect, rect) || camera.__prevDisplayMode != camera.displayMode) {
                    SDL_FRect displayRect = rect;

                    switch(camera.displayMode) {
                    case DISPLAY_MODE_CONTAIN:
                    case DISPLAY_MODE_COVER:   {
                        // most logic here from: https://github.com/nrkn/object-fit-math/blob/master/src/fitter.ts
                        float widthRatio  = rect.w / tex->w;
                        float heightRatio = rect.h / tex->h;

                        // min of width vs height ratios
                        float ratio = camera.displayMode == DISPLAY_MODE_CONTAIN
                                          ? CLAY__MIN(widthRatio, heightRatio)
                                          : CLAY__MAX((rect.w / tex->w), (rect.h / tex->h));

                        displayRect.w = tex->w * ratio;
                        displayRect.h = tex->h * ratio;
                        displayRect.x = (rect.w - displayRect.w) / 2.0f;
                        displayRect.y = (rect.h - displayRect.h) / 2.0f;

                        break;
                    }
                    case DISPLAY_MODE_FILL: break;
                    case DISPLAY_MODE_NONE:
                        displayRect = {
                            .x = 0,
                            .y = 0,
                            .w = static_cast<float>(tex->w),
                            .h = static_cast<float>(tex->h),
                        };

                        break;
                    default: break;
                    }

                    camera.__prevRect        = rect;
                    camera.__prevDisplayMode = camera.displayMode;
                    camera.__displayRect     = displayRect;
                }

                SDL_RenderTexture(rendererData->renderer, tex, NULL, &camera.__displayRect);
                break;
            }
            default:
                SDL_Log("Unknown custom render command type: %d", rcmd->commandType);
                break;
            }

            break;
        }
        default: {
            SDL_Log("Unknown render command type: %d", rcmd->commandType);
            break;
        }
        }
    }
}