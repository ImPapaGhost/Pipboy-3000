#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include "render.h"
#include "pipboy.h"
#include "core.h"

void render_health_background(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    if (!resources || !resources->health_background) return;

    const int background_width = state && state->current_tab == TAB_DATA ? 135 : 180;
    SDL_Rect background_rect = {110, 430, background_width, 30};
    SDL_SetTextureColorMod(resources->health_background, 0, 255, 0);
    SDL_RenderCopy(renderer, resources->health_background, NULL, &background_rect);

    if (!state || state->current_tab == TAB_DATA || state->max_health <= 0) {
        return;
    }

    SDL_Rect meter = {115, 453, background_width - 10, 5};
    SDL_SetRenderDrawColor(renderer, 0, 35, 0, 255);
    SDL_RenderFillRect(renderer, &meter);

    int current_health = state->health;
    const int effective_max = pipboy_effective_max_health(state);
    if (current_health < 0) current_health = 0;
    if (current_health > effective_max) current_health = effective_max;
    const int health_width = meter.w * current_health / state->max_health;
    if (health_width > 0) {
        SDL_Rect health = {meter.x, meter.y, health_width, meter.h};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &health);
    }

    const int blocked_health = pipboy_radiation_blocked_health(state);
    const int radiation_width = meter.w * blocked_health / state->max_health;
    if (radiation_width > 0) {
        SDL_Rect radiation = {
            meter.x + meter.w - radiation_width,
            meter.y,
            radiation_width,
            meter.h
        };
        SDL_SetRenderDrawColor(renderer, 255, 96, 32, 255);
        SDL_RenderFillRect(renderer, &radiation);
        SDL_SetRenderDrawColor(renderer, 32, 8, 0, 255);
        for (int x = radiation.x + 2; x < radiation.x + radiation.w; x += 4) {
            SDL_RenderDrawLine(renderer, x, radiation.y, x, radiation.y + radiation.h - 1);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &meter);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

void render_ap_bar(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    if (!resources || !resources->box_background) return;

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
    SDL_SetTextureColorMod(resources->box_background, 0, 255, 0);
    SDL_RenderCopy(renderer, resources->box_background, NULL, &bar_rect);
}

void render_date_time(SDL_Renderer *renderer, const AppResources *resources, PipState *state) {
    if (state->current_tab != TAB_DATA) {
        return;  // Only render if in DATA tab
    }

    TTF_Font *font = resources->body_font;
    SDL_Color color = {0, 255, 0, 255}; // Green text color
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (!tm_info) return;

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
