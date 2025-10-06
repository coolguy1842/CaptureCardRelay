#ifndef __CLAY_RENDERER_SDL3_HPP__
#define __CLAY_RENDERER_SDL3_HPP__

#include <SDL3/SDL.h>
#include <SDL3/SDL_camera.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <clay.h>

#include <vector>

struct Clay_Color;
struct Clay_RenderCommandArray;

struct Clay_SDL3RendererData {
    SDL_Renderer* renderer;
    TTF_TextEngine* textEngine;
    std::vector<TTF_Font*> fonts;
};

typedef enum {
    CUSTOM_ELEMENT_TYPE_CAMERA
} CustomElementType;

struct CameraData {
    SDL_Camera* device;
    SDL_Texture* texture;
};

typedef struct {
    CustomElementType type;
    union {
        CameraData camera;
    };
} CustomElementData;

Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData);

void SDL_Clay_RenderFillRoundedRect(Clay_SDL3RendererData* rendererData, const SDL_FRect rect, const float cornerRadius, const Clay_Color _color);
void SDL_Clay_RenderArc(Clay_SDL3RendererData* rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness, const Clay_Color color);
void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData* rendererData, Clay_RenderCommandArray* rcommands);

#endif