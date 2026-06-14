#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    for (size_t i = 0; i < sizeof(chip8.memory); i++) {
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
