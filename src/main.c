#include <SDL2/SDL.h>
#include <flecs.h>
#include <math.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

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

const float speed = 0.2f;
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
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

    window = SDL_CreateWindow(
        "SDL2 Example",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Rect square = {216, 96, 34, 64};

    int running = 1;

    SDL_Event event;
    ecs_world_t *world = ecs_init_w_args(argc, argv);
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_SYSTEM(world, MoveBody, EcsOnUpdate, Position, Velocity);
    ECS_SYSTEM(world, ControlVelocity, EcsOnUpdate, Velocity);
    ECS_SYSTEM(world, RenderBody, EcsOnUpdate, Position);

    ecs_entity_t e = ecs_new_id(world);
    ecs_set(world, e, Position, {32, 32});
    ecs_set(world, e, Velocity, {0, 0});

    const int desired_fps = 60;
    int last_measure = SDL_GetTicks();

    while(running) {
        if((SDL_GetTicks() - last_measure) < (1000 / desired_fps)) {
            continue;
        }
        last_measure = SDL_GetTicks();
        
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

        SDL_RenderClear(renderer);
        
        ecs_progress(world, 0);

        SDL_RenderPresent(renderer);
    }

    return ecs_fini(world);
}
