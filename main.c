#include "chip8.h"
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define PIXEL_SCALE 10
#define INSTRUCTION_PER_FRAME 10
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_FREQUENCY 440
#define SAMPLE_COUNT (AUDIO_SAMPLE_RATE/AUDIO_FREQUENCY)

void render(SDL_Renderer* renderer, Chip8* chip8) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (size_t i = 0; i < sizeof(chip8->display); i++) {
        if (chip8->display[i] == 1) {
            SDL_Rect pixel_rect = { (i % 64) * PIXEL_SCALE, (i / 64) * PIXEL_SCALE, PIXEL_SCALE, PIXEL_SCALE };
            SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
    SDL_RenderPresent(renderer);
}

void update_keypad(Chip8* chip8, SDL_Event* event) {
    switch(event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            uint8_t flag = event->type == SDL_KEYUP ? 0 : 1;
            switch(event->key.keysym.sym) {
                case SDLK_1:
                    chip8->keypad[0x1] = flag;
                    break;
                case SDLK_2:
                    chip8->keypad[0x2] = flag;
                    break;
                case SDLK_3:
                    chip8->keypad[0x3] = flag;
                    break;
                case SDLK_4:
                    chip8->keypad[0xC] = flag;
                    break;
                case SDLK_q:
                    chip8->keypad[0x4] = flag;
                    break;
                case SDLK_w:
                    chip8->keypad[0x5] = flag;
                    break;
                case SDLK_e:
                    chip8->keypad[0x6] = flag;
                    break;
                case SDLK_r:
                    chip8->keypad[0xD] = flag;
                    break;
                case SDLK_a:
                    chip8->keypad[0x7] = flag;
                    break;
                case SDLK_s:
                    chip8->keypad[0x8] = flag;
                    break;
                case SDLK_d:
                    chip8->keypad[0x9] = flag;
                    break;
                case SDLK_f:
                    chip8->keypad[0xE] = flag;
                    break;
                case SDLK_z:
                    chip8->keypad[0xA] = flag;
                    break;
                case SDLK_x:
                    chip8->keypad[0x0] = flag;
                    break;
                case SDLK_c:
                    chip8->keypad[0xB] = flag;
                    break;
                case SDLK_v:
                    chip8->keypad[0xF] = flag;
                    break;
            }
        }
        break;
    }
}

void audio_callback(void* userdata, uint8_t* stream, int len) {
    (void)userdata;
    int16_t* samples = (int16_t*)stream;
    int num_samples = len / 2;
    static int counter = 0;

    for (int i = 0; i < num_samples; i++) {
        samples[i] = (counter < SAMPLE_COUNT / 2) ? 3000 : -3000;
        counter = (counter + 1) % SAMPLE_COUNT;
    }
}

SDL_AudioDeviceID init_audio_device() {
    SDL_AudioSpec spec = {0};
    spec.freq = AUDIO_SAMPLE_RATE;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 512;
    spec.callback = audio_callback;
    return SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Usage: %s <path_to_the_rom>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("Can't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("RChip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Can't create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Can't create renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Event event;
    Chip8 chip8 = init_chip8();
    if (load_rom(&chip8, argv[1]) != 0) {
        SDL_Quit();
        return 1;
    }

    SDL_AudioDeviceID audio_device = init_audio_device();

    uint8_t is_running = 1;
    Uint32 last_timer_update = SDL_GetTicks();
    while(is_running) {

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        is_running = 0;
                    }
                    break;
            }
            update_keypad(&chip8, &event);
        }

        for (int i = 0; i < INSTRUCTION_PER_FRAME; i++) {
            uint16_t instruction = fetch(&chip8);
            DecodedInstruction decoded = decode(instruction);
            execute(&chip8, &decoded);
        }

        Uint32 now = SDL_GetTicks();
        if (now - last_timer_update >= 16) {
            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) chip8.sound_timer--;
            last_timer_update = now;
            SDL_PauseAudioDevice(audio_device, chip8.sound_timer == 0);
        }
        render(renderer, &chip8);
        SDL_Delay(16);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
