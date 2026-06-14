#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define PIXEL_SCALE 10
#define INSTRUCTION_PER_FRAME 10
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_FREQUENCY 440
#define SAMPLE_COUNT (AUDIO_SAMPLE_RATE/AUDIO_FREQUENCY)

typedef struct {
    uint8_t memory[4096];
    uint8_t v[16];
    uint16_t pc;
    uint16_t i;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t display[64 * 32];
    uint16_t keypad[16];

} Chip8;

typedef struct {
    uint16_t original;
    uint8_t nibble;
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint16_t nn;
    uint16_t nnn;
} DecodedInstruction;

Chip8 init_chip8() {
    uint8_t fontset[80] = {
        0xF0,0x90,0x90,0x90,0xF0, // 0
        0x20,0x60,0x20,0x20,0x70, // 1
        0xF0,0x10,0xF0,0x80,0xF0, // 2
        0xF0,0x10,0xF0,0x10,0xF0, // 3
        0x90,0x90,0xF0,0x10,0x10, // 4
        0xF0,0x80,0xF0,0x10,0xF0, // 5
        0xF0,0x80,0xF0,0x90,0xF0, // 6
        0xF0,0x10,0x20,0x40,0x40, // 7
        0xF0,0x90,0xF0,0x90,0xF0, // 8
        0xF0,0x90,0xF0,0x10,0xF0, // 9
        0xF0,0x90,0xF0,0x90,0x90, // A
        0xE0,0x90,0xE0,0x90,0xE0, // B
        0xF0,0x80,0x80,0x80,0xF0, // C
        0xE0,0x90,0x90,0x90,0xE0, // D
        0xF0,0x80,0xF0,0x80,0xF0, // E
        0xF0,0x80,0xF0,0x80,0x80  // F
    };
    Chip8 chip8;
    memset(&chip8, 0, sizeof(chip8));
    for (int i = 0; i < sizeof(chip8.memory); i++) {
        chip8.memory[i] = 0;
    }
    chip8.pc = 0x200;
    memcpy(chip8.memory, fontset, 80);
    return chip8;
}

int load_rom(Chip8* chip8, char* path) {
    printf("Opening rom at: %s\n", path);
    char* permissions = "rb";
    int mem_start_index = 0x200;
    FILE* fptr = fopen(path, permissions);
    if (fptr == NULL) {
        perror("An error occurred opening rom file");
        printf("Rom path: %s\n", path);
        return 1;
    }
    fread(&chip8->memory[mem_start_index], sizeof(uint8_t), sizeof(chip8->memory) - mem_start_index, fptr);
    fclose(fptr);
    return 0;
}

uint16_t fetch(Chip8* chip8) {
    uint16_t instruction = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;
    return instruction;
}

DecodedInstruction decode(uint16_t instruction) {
    DecodedInstruction decoded;
    decoded.original = instruction;
    decoded.nibble = instruction >> 12;
    decoded.x = (instruction & 0xF00) >> 8;
    decoded.y = (instruction & 0x0F0) >> 4;
    decoded.n = (instruction & 0x00F);
    decoded.nn = (instruction & 0x0FF);
    decoded.nnn = instruction & 0xFFF;
    return decoded;
}

void execute(Chip8* chip8, DecodedInstruction* instruction) {
    switch(instruction->nibble) {
        case 0x0:
            switch (instruction->original) {
                case 0x00E0:
                    memset(chip8->display, 0, sizeof chip8->display);
                    break;
                case 0x00EE:
                    chip8->sp--;
                    chip8->pc = chip8->stack[chip8->sp];
                    break;
            }
            break;
        case 0x1:
            chip8->pc = instruction->nnn;
            break;
        case 0x2:
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = instruction->nnn;
            break;
        case 0x3:
            if (chip8->v[instruction->x] == instruction->nn) {
                chip8->pc += 2;
            }
            break;
        case 0x4:
            if (chip8->v[instruction->x] != instruction->nn) {
                chip8->pc += 2;
            }
            break;
        case 0x5:
            if (chip8->v[instruction->x] == chip8->v[instruction->y]) {
                chip8->pc += 2;
            }
            break;
        case 0x6:
            chip8->v[instruction->x] = instruction->nn;
            break;
        case 0x7:
            chip8->v[instruction->x] += instruction->nn;
            break;
        case 0x8:
            switch(instruction->n) {
                case 0:
                    chip8->v[instruction->x] = chip8->v[instruction->y];
                    break;
                case 1:
                    chip8->v[instruction->x] = chip8->v[instruction->x] | chip8->v[instruction->y];
                    break;
                case 2:
                    chip8->v[instruction->x] = chip8->v[instruction->x] & chip8->v[instruction->y];
                    break;
                case 3:
                    chip8->v[instruction->x] = chip8->v[instruction->x] ^ chip8->v[instruction->y];
                    break;
                case 4: {
                    uint16_t sum = chip8->v[instruction->x] + chip8->v[instruction->y];
                    uint8_t result = sum & 0xFF;
                    uint8_t carry = (sum > 0xFF) ? 1 : 0;
                    chip8->v[instruction->x] = result;
                    chip8->v[0xF] = carry;
                    break;
                }
                case 5: {
                    int16_t diff = chip8->v[instruction->x] - chip8->v[instruction->y];
                    chip8->v[instruction->x] = diff;
                    chip8->v[0xF] = (diff < 0) ? 0 : 1;
                    break;
                }
                case 6: {
                    uint8_t vf = chip8->v[instruction->y] & 0x1;
                    uint8_t result = chip8->v[instruction->y] >> 1;
                    chip8->v[instruction->x] = result;
                    chip8->v[0xF] = vf;
                    break;
                }
                case 7: {
                    int16_t diff = chip8->v[instruction->y] - chip8->v[instruction->x];
                    chip8->v[instruction->x] = diff;
                    chip8->v[0xF] = (diff < 0) ? 0 : 1;
                    break;
                }
                case 0xE: {
                    uint8_t vf = (chip8->v[instruction->y] & 0x80) >> 7;
                    uint8_t result = chip8->v[instruction->y] << 1;
                    chip8->v[instruction->x] = result;
                    chip8->v[0xF] = vf;
                    break;
                }
                default:
                    printf("Unhandled opcode: 0x%04X (nibble 0x%X)\n", instruction->original, instruction->nibble);
                    break;
            }
            break;
        case 0x9:
            if (chip8->v[instruction->x] != chip8->v[instruction->y]) {
                chip8->pc += 2;
            }
            break;
        case 0xA:
            chip8->i = instruction->nnn;
            break;
        case 0xC: {
            int random_value = rand() % (0xFF + 1);
            chip8->v[instruction->x] = random_value & instruction->nn;
            break;
        }
        case 0xD: {
            uint8_t vx = chip8->v[instruction->x] % 64;
            uint8_t vy = chip8->v[instruction->y] % 32;
            chip8->v[0xF] = 0;
            for (int row = 0; row < instruction->n; row++) {
                uint8_t sprite = chip8->memory[chip8->i + row];
                for (int bit = 0; bit < 8; bit++) {
                    uint16_t x = vx + bit;
                    uint16_t y = vy + row;
                    uint16_t index = y * 64 + x ;
                    uint8_t font_pixel = (sprite & (0x80 >> bit)) > 0;
                    uint8_t display_byte = chip8->display[index];
                    uint8_t new_value = display_byte ^ font_pixel;
                    if (x >= 64 || y >= 32 ) {
                        continue;
                    }
                    chip8->display[index] = new_value;
                    if (font_pixel == 1 && new_value == 0) {
                        chip8->v[0xF] = 1;
                    }
                }

            }
            break;
        }
        case 0xE:
            switch(instruction->nn) {
                case 0x9E:
                    if (chip8->keypad[chip8->v[instruction->x]]) {
                        chip8->pc += 2;
                    }
                    break;
                case 0xA1:
                    if (!chip8->keypad[chip8->v[instruction->x]]) {
                        chip8->pc += 2;
                    }
                    break;
                default:
                    printf("Unhandled opcode: 0x%04X (nibble 0x%X)\n", instruction->original, instruction->nibble);
                    break;
            }
            break;
        case 0xF:
            switch(instruction->nn) {
                case 0x07:
                    chip8->v[instruction->x] = chip8->delay_timer;
                    break;
                case 0x15:
                    chip8->delay_timer = chip8->v[instruction->x];
                    break;
                case 0x18:
                    chip8->sound_timer = chip8->v[instruction->x];
                    break;
                case 0x29:
                    chip8->i = chip8->v[instruction->x] * 5;
                    break;
                case 0x33:
                    chip8->memory[chip8->i] = chip8->v[instruction->x] / 100;
                    chip8->memory[chip8->i + 1] = (chip8->v[instruction->x] / 10) % 10;
                    chip8->memory[chip8->i + 2] = chip8->v[instruction->x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= instruction->x; i++) {
                        chip8->memory[chip8->i + i] = chip8->v[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= instruction->x; i++) {
                        chip8->v[i] = chip8->memory[chip8->i + i];
                    }
                    break;
                case 0x1E:
                    chip8->i += chip8->v[instruction->x];
                    break;
                case 0x0A: {
                    int key_pressed = -1;
                    for (int k = 0; k < 16; k++) {
                        if (chip8->keypad[k]) {
                            key_pressed = k;
                            break;
                        }
                    }
                    if (key_pressed == -1) {
                        chip8->pc -= 2;
                    } else {
                        chip8->v[instruction->x] = key_pressed;
                    }
                    break;
                }
                case 0xB:
                    chip8->pc = instruction->nnn + chip8->v[0];
                    break;
                default:
                    printf("Unhandled opcode: 0x%04X (nibble 0x%X)\n", instruction->original, instruction->nibble);
                    break;
            }
            break;
            default:
                printf("Unhandled opcode: 0x%04X (nibble 0x%X)\n", instruction->original, instruction->nibble);
                break;
    }
}

void render(SDL_Renderer* renderer, Chip8* chip8) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < sizeof(chip8->display); i++) {
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
