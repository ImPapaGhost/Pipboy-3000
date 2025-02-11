#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include "render.h"
#include "pipboy.h"

void render_health_background(SDL_Renderer *renderer) {
    SDL_Texture *background = IMG_LoadTexture(renderer, "STAT/BOXHP1.jpg");
    SDL_Rect background_rect = {110, 430, 135, 30};
    SDL_SetTextureColorMod(background, 0, 255, 0);
    SDL_RenderCopy(renderer, background, NULL, &background_rect);
    SDL_DestroyTexture(background);
}

void render_ap_bar(SDL_Renderer *renderer, PipState *state) {
    SDL_Texture *bar = IMG_LoadTexture(renderer, "STAT/BOX4.jpg");
    if (!bar) return;  // Prevent crashes if the texture fails to load

    // Default AP bar settings (STAT tab)
    int bar_width = 145;
    int bar_x = SCREEN_WIDTH - 245;         // Default position

    // Modify width and position of AP bar in DATA tab
    if (state->current_tab == TAB_DATA) {
        bar_width = 345;                    // Adjusted width
        bar_x = SCREEN_WIDTH - 445;         // Adjust position
    }

    // Define the bar rectangle
    SDL_Rect bar_rect = {bar_x, 430, bar_width, 30};

    // Render the bar
    SDL_SetTextureColorMod(bar, 0, 255, 0);
    SDL_RenderCopy(renderer, bar, NULL, &bar_rect);

    SDL_DestroyTexture(bar);
}

void render_date_time(SDL_Renderer *renderer, TTF_Font *font, PipState *state) {
    if (state->current_tab != TAB_DATA) {
        return;  // Only render if in DATA tab
    }

    SDL_Color color = {0, 255, 0, 255}; // Green text color
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    // Format date
    char date_text[20];
    strftime(date_text, sizeof(date_text), "%b %d %Y", tm_info);

    // Render date in the left corner where HP text was
    SDL_Surface *date_surface = TTF_RenderText_Solid(font, date_text, color);
    SDL_Texture *date_texture = SDL_CreateTextureFromSurface(renderer, date_surface);
    SDL_Rect date_rect = {115, 435, date_surface->w, date_surface->h}; // Adjust as needed
    SDL_RenderCopy(renderer, date_texture, NULL, &date_rect);
    SDL_FreeSurface(date_surface);
    SDL_DestroyTexture(date_texture);

    // Format time
    char time_text[10];
    strftime(time_text, sizeof(time_text), "%I:%M %p", tm_info);

    // Render time where XP text was
    SDL_Surface *time_surface = TTF_RenderText_Solid(font, time_text, color);
    SDL_Texture *time_texture = SDL_CreateTextureFromSurface(renderer, time_surface);
    SDL_Rect time_rect = {(SCREEN_WIDTH / 2)- 145, 435, time_surface->w, time_surface->h}; // Adjust as needed
    SDL_RenderCopy(renderer, time_texture, NULL, &time_rect);
    SDL_FreeSurface(time_surface);
    SDL_DestroyTexture(time_texture);
}
