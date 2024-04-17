#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
/* #include <SDL2/SDL_ttf.h> */
#include <flecs.h>
#include <math.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define MUSIC_PATH "assets/walkin.ogg"
#define FONT_PATH "assets/Pacifico.ttf"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

typedef struct {
    float x, y;
} vec2;

typedef vec2 Position;
typedef vec2 Velocity;

static int press_up = 0;
static int press_down = 0;
static int press_left = 0;
static int press_right = 0;

const float speed = 0.4f;
const float decel = speed * 0.6f;
const float max_speed = 6.0f;
const float bounce_back = 2.0f;

void
MoveBody(ecs_iter_t *it)
{
    Position *p = ecs_field(it, Position, 1);
    Velocity *v = ecs_field(it, Velocity, 2);

    int i;
    for(i = 0; i < it->count; i++) {
        p[i].x += v[i].x;
        p[i].y += v[i].y;

        if(p[i].x > SCREEN_WIDTH - 32) {
            p[i].x = SCREEN_WIDTH - 32;
            v[i].x = -bounce_back;
        }

        if(p[i].x < 0.0f) {
            p[i].x = 0.0f;
            v[i].x = bounce_back;
        }

        if(p[i].y > SCREEN_HEIGHT - 32) {
            p[i].y = SCREEN_HEIGHT - 32;
            v[i].y = -bounce_back;
        }

        if(p[i].y < 0.0f) {
            p[i].y = 0.0f;
            v[i].y = bounce_back;
        }
    }
}

void
ControlVelocity(ecs_iter_t *it)
{
    Velocity *v = ecs_field(it, Velocity, 1);
    int i;
    for(i = 0; i < it->count; i++) {
        if(press_up) v[i].y -= speed;
        if(press_down) v[i].y += speed;
        if(press_left) v[i].x -= speed;
        if(press_right) v[i].x += speed;

        if(!press_left && !press_right) {
            if(v[i].x > decel) v[i].x -= decel;
            else if(v[i].x < -decel) v[i].x += decel;
            else v[i].x = 0.0f;
        }

        if(!press_up && !press_down) {
            if(v[i].y > decel) v[i].y -= decel;
            else if(v[i].y < -decel) v[i].y += decel;
            else v[i].y = 0.0f;
        }


        if(v[i].x > max_speed) v[i].x = max_speed;
        if(v[i].x < -max_speed) v[i].x = -max_speed;

        if(v[i].y > max_speed) v[i].y = max_speed;
        if(v[i].y < -max_speed) v[i].y = -max_speed;
    }
}

void
RenderBody(ecs_iter_t *it)
{
    Position *p = ecs_field(it, Position, 1);

    int i;
    for(i = 0; i < it->count; i++) {
        SDL_Rect rect = {p[i].x, p[i].y, 32, 32};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    }
}

int
main(int argc, char **argv)
{
    SDL_Init(
        SDL_INIT_VIDEO |
        SDL_INIT_AUDIO |
        SDL_INIT_GAMECONTROLLER);

    /* TTF_Init(); */

    Mix_OpenAudio(
        44100,
        MIX_DEFAULT_FORMAT,
        MIX_DEFAULT_CHANNELS,
        2048);

    window = SDL_CreateWindow(
        "Luksamuk PSP Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    /* TTF_Font *font = TTF_OpenFont(FONT_PATH, 32); */
    /* SDL_Color text_color = {0xff, 0xff, 0xff, 0xff}; */
    /* SDL_Color bg_color = {0x00, 0x00, 0x00, 0xff}; */
    /* SDL_Rect text_rect; */
    /* SDL_Surface *surface = TTF_RenderText(font, "Now Loading...", text_color, bg_color); */
    /* SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface); */
    /* text_rect.w = surface->w; */
    /* text_rect.h = surface->h; */
    /* SDL_FreeSurface(surface); */
    /* text_rect.x = (SCREEN_WIDTH - text_rect.w) / 2; */
    /* text_rect.y = text_rect.h + 30; */
    /* SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff); */
    /* SDL_RenderClear(renderer); */
    /* SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff); */
    /* SDL_RenderCopy(renderer, texture, NULL, &text_rect); */
    /* SDL_RenderPresent(renderer); */
    

    Mix_Music *ogg_file = NULL;
    ogg_file = Mix_LoadMUS(MUSIC_PATH);

    int running = 1;
    int playing = 0;

    SDL_Event event;
    ecs_world_t *world = ecs_init_w_args(argc, argv);
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_SYSTEM(world, MoveBody, EcsOnUpdate, Position, Velocity);
    ECS_SYSTEM(world, ControlVelocity, EcsOnUpdate, Velocity);
    ECS_SYSTEM(world, RenderBody, EcsPostUpdate, Position);

    ecs_entity_t e = ecs_new_id(world);
    ecs_set(world, e, Position, {32, 32});
    ecs_set(world, e, Velocity, {0, 0});
    
    const int desired_fps = 30;
    int last_measure = SDL_GetTicks();

    while(running) {
        if(ogg_file && !playing) {
            printf("Ok\n");
            Mix_PlayMusic(ogg_file, 1000);
            playing = 1;
        }
        
        if(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_CONTROLLERDEVICEADDED:
                SDL_GameControllerOpen(event.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_START:
                    running = 0;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    press_left = 1;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    press_right = 1;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    press_up = 1;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    press_down = 1;
                    break;
                }
                break;
            case SDL_CONTROLLERBUTTONUP:
                switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    press_left = 0;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    press_right = 0;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    press_up = 0;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    press_down = 0;
                    break;
                }
                break;
            }
        }

/* #if !defined(__PSP__) */
        if((SDL_GetTicks() - last_measure) < (1000 / desired_fps)) {
            continue;
        }
        last_measure = SDL_GetTicks();
/* #endif */
        
        SDL_RenderClear(renderer);
        ecs_progress(world, 1.0f / desired_fps);
        SDL_RenderPresent(renderer);
    }

    if(ogg_file) {
        Mix_FreeMusic(ogg_file);
    }

    ecs_fini(world);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    /* TTF_Quit(); */
    SDL_Quit();
    
    return 0;
}
