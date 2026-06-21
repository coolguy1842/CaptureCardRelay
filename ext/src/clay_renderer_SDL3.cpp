#include <clay_renderer_SDL3.hpp>
#include <map>
#include <stack>

Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    std::vector<TTF_Font*>& fonts = *reinterpret_cast<std::vector<TTF_Font*>*>(userData);
    TTF_Font* font                = fonts[config->fontId];
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if(!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return Clay_Dimensions{
        (float)width,
        (float)height,
    };
}

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static int NUM_CIRCLE_SEGMENTS = 16;

void SDL_Clay_RenderArc(Clay_SDL3RendererData* rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness, const Clay_Color color) {
    SDL_SetRenderDrawColor(rendererData->renderer, color.r, color.g, color.b, color.a);

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd   = endAngle * (SDL_PI_F / 180.0f);

    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(radius * 1.5f)); // increase circle segments for larger circles, 1.5 is arbitrary.

    const float angleStep     = (radEnd - radStart) / (float)numCircleSegments;
    const float thicknessStep = 0.4f; // arbitrary value to avoid overlapping lines. Changing THICKNESS_STEP or numCircleSegments might cause artifacts.

    for(float t = thicknessStep; t < thickness - thicknessStep; t += thicknessStep) {
        std::vector<SDL_FPoint> points(numCircleSegments + 1);
        const float clampedRadius = SDL_max(radius - t, 1.0f);

        for(int i = 0; i <= numCircleSegments; i++) {
            const float angle = radStart + i * angleStep;
            points[i]         = (SDL_FPoint){
                SDL_roundf(center.x + SDL_cosf(angle) * clampedRadius),
                SDL_roundf(center.y + SDL_sinf(angle) * clampedRadius)
            };
        }

        SDL_RenderLines(rendererData->renderer, points.data(), points.size());
    }
}

void SDL_Clay_RenderFilledArc(Clay_SDL3RendererData* rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const Clay_Color _color) {
    const SDL_FColor color = { _color.r / 255, _color.g / 255, _color.b / 255, _color.a / 255 };
    int indexCount = 0, vertexCount = 0;

    const float radStart = startAngle * (SDL_PI_F / 180.0f);
    const float radEnd   = endAngle * (SDL_PI_F / 180.0f);

    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(radius * 1.5f));

    const float angleStep = (radEnd - radStart) / (float)numCircleSegments;

    const int totalVertices = 2 + numCircleSegments;
    const int totalIndices  = 3 + (numCircleSegments * 3);

    std::vector<SDL_Vertex> vertices(totalVertices);
    std::vector<int> indices(totalIndices);

    const float clampedRadius = SDL_max(radius, 1.0f);
    vertices[vertexCount++]   = { .position = { center.x, center.y }, .color = color, .tex_coord = { 0, 0 } };

    for(int i = 0; i <= numCircleSegments; i++) {
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

static std::map<int, const char*> idToName = {
    { CLAY_ID("Settings").id, "Settings" },
    { CLAY_ID("CamerasContainer").id, "Cameras" },
    { CLAY_ID("RecordingDevicesContainer").id, "Recording Devices" },
    { CLAY_ID("Volume").id, "Volume" },
    { CLAY_ID("VolumeSlider").id, "Volume Slider" },
    { CLAY_ID("VolumeSliderHandle").id, "Volume Slider Handle" },
};

std::stack<SDL_Rect> m_scissorStack;
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

            TTF_Text* text = TTF_CreateText(rendererData->textEngine, font, config->stringContents.chars, config->stringContents.length);
            TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
            TTF_DrawRendererText(text, rect.x, rect.y);
            TTF_DestroyText(text);

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
                SDL_Clay_RenderArc(rendererData, (SDL_FPoint){ centerX, centerY }, clampedRadii.topLeft, 180.0f, 270.0f, config->width.top, config->color);
            }

            if(config->cornerRadius.topRight > 0) {
                const float centerX = rect.x + rect.w - clampedRadii.topRight - 1.5f;
                const float centerY = rect.y + clampedRadii.topRight - 0.5f;
                SDL_Clay_RenderArc(rendererData, (SDL_FPoint){ centerX, centerY }, clampedRadii.topRight, 270.0f, 360.0f, config->width.top, config->color);
            }

            if(config->cornerRadius.bottomLeft > 0) {
                const float centerX = rect.x + clampedRadii.bottomLeft - 0.5f;
                const float centerY = rect.y + rect.h - clampedRadii.bottomLeft - 0.5f;
                SDL_Clay_RenderArc(rendererData, (SDL_FPoint){ centerX, centerY }, clampedRadii.bottomLeft, 90.0f, 180.0f, config->width.bottom, config->color);
            }

            if(config->cornerRadius.bottomRight > 0) {
                const float centerX = rect.x + rect.w - clampedRadii.bottomRight - 1.5f;
                const float centerY = rect.y + rect.h - clampedRadii.bottomRight - 0.5f;
                SDL_Clay_RenderArc(rendererData, (SDL_FPoint){ centerX, centerY }, clampedRadii.bottomRight, 0.0f, 90.0f, config->width.bottom, config->color);
            }

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
            Clay_BoundingBox bounds = rcmd->boundingBox;

            if(!m_scissorStack.empty()) {
                SDL_Rect parent = m_scissorStack.top();

                int minX = SDL_max(parent.x, bounds.x);
                int maxX = SDL_min(parent.x + parent.w, bounds.x + bounds.width);

                if(maxX - minX < 1) {
                    m_scissorStack.push({ 0, 0, 0, 0 });
                    goto setRect;
                }

                int minY = SDL_max(parent.y, bounds.y);
                int maxY = SDL_min(parent.y + parent.h, bounds.y + bounds.height);
                if(maxY - minY < 1) {
                    m_scissorStack.push({ 0, 0, 0, 0 });
                    goto setRect;
                }

                SDL_Rect scissor = {
                    .x = minX,
                    .y = minY,
                    .w = maxX - minX,
                    .h = SDL_max(1, maxY - minY),
                };

                m_scissorStack.push(scissor);
            }
            else {
                m_scissorStack.push({ (int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height });
            }

        setRect:
            SDL_SetRenderClipRect(rendererData->renderer, &m_scissorStack.top());

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
            if(m_scissorStack.empty()) {
                break;
            }

            m_scissorStack.pop();
            SDL_SetRenderClipRect(rendererData->renderer, !m_scissorStack.empty() ? &m_scissorStack.top() : nullptr);

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
                SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(rendererData->renderer, rcmd->renderData.custom.backgroundColor.r, rcmd->renderData.custom.backgroundColor.g, rcmd->renderData.custom.backgroundColor.b, rcmd->renderData.custom.backgroundColor.a);

                SDL_RenderFillRect(rendererData->renderer, &rect);

                SDL_Texture*& tex = data->camera.texture;
                SDL_CameraSpec spec;

                auto lock = std::unique_lock(data->camera.mutex);
                if(data->camera.device == nullptr || SDL_GetCameraPermissionState(data->camera.device) != 1 || !SDL_GetCameraFormat(data->camera.device, &spec)) {
                    SDL_DestroyTexture(tex);
                    tex = nullptr;

                    break;
                }

                if(tex == nullptr || tex->format != spec.format || tex->w != spec.width || tex->h != spec.height) {
                    SDL_DestroyTexture(tex);
                    tex = SDL_CreateTexture(rendererData->renderer, spec.format, SDL_TEXTUREACCESS_STREAMING, spec.width, spec.height);
                }

                SDL_Surface* surface = SDL_AcquireCameraFrame(data->camera.device, nullptr);
                if(surface != nullptr) {
                    SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
                    SDL_ReleaseCameraFrame(data->camera.device, surface);
                }

                lock.unlock();

                if(tex != nullptr) {
                    SDL_FRect destRect = rect;
                    switch(data->camera.displayMode) {
                    case CameraDisplayMode::CONTAIN:
                    case CameraDisplayMode::COVER:   {
                        // most logic here from: https://github.com/nrkn/object-fit-math/blob/master/src/fitter.ts
                        float widthRatio  = rect.w / tex->w;
                        float heightRatio = rect.h / tex->h;

                        // min of width vs height ratios
                        float ratio = data->camera.displayMode == CameraDisplayMode::CONTAIN
                                          ? CLAY__MIN(widthRatio, heightRatio)
                                          : CLAY__MAX((rect.w / tex->w), (rect.h / tex->h));

                        destRect.w = tex->w * ratio;
                        destRect.h = tex->h * ratio;
                        destRect.x = (rect.w - destRect.w) / 2.0f;
                        destRect.y = (rect.h - destRect.h) / 2.0f;

                        break;
                    }
                    case CameraDisplayMode::FILL: break;
                    case CameraDisplayMode::NONE:
                        destRect = {
                            .x = 0,
                            .y = 0,
                            .w = static_cast<float>(tex->w),
                            .h = static_cast<float>(tex->h),
                        };

                        break;
                    default: break;
                    }

                    SDL_RenderTexture(rendererData->renderer, tex, NULL, &destRect);
                }

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