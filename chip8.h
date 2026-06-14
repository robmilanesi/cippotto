#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

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

Chip8 init_chip8();
int load_rom(Chip8* chip8, char* path);
uint16_t fetch(Chip8* chip8);
DecodedInstruction decode(uint16_t instruction);
void execute(Chip8* chip8, DecodedInstruction* instruction);

#endif
