# Cippotto

A CHIP-8 emulator written in C using SDL2 as graphic library.

![IBM Logo](/docs/ibm.png "IBM Logo")

## Why?
My main goal was to learn C, getting into emulators and system programming a bit.

## Features

- Full fetch-decode-execute cycle
- 64×32 pixel display scaled to 640×320 (10× scale)
- 16-key hexadecimal keypad input
- Delay and sound timers running at ~60 Hz
- Built-in fontset (0–F)
- Runs at 10 instructions per frame (~600 instructions/second)
- Audio support

## To Be added
 - Quirks: currently the implementation is very basic, no config allowed for newer version functionalities.
 - Save states
 - Super Chip8 improvements
 - Better rendering (configurable, currently it's very basic)
 - Color palette configurable
 - Interactive debugger

## Implemented opcodes

| Opcode | Description |
|--------|-------------|
| `00E0` | Clear display |
| `00EE` | Return from subroutine |
| `1NNN` | Jump to address NNN |
| `2NNN` | Call subroutine at NNN |
| `3XNN` | Skip if VX == NN |
| `4XNN` | Skip if VX != NN |
| `5XY0` | Skip if VX == VY |
| `6XNN` | Set VX = NN |
| `7XNN` | Set VX += NN |
| `8XY0` | Set VX = VY |
| `8XY1` | Set VX = VX OR VY |
| `8XY2` | Set VX = VX AND VY |
| `8XY3` | Set VX = VX XOR VY |
| `8XY4` | Set VX += VY, VF = carry |
| `8XY5` | Set VX -= VY, VF = no-borrow |
| `8XY6` | Shift VY right, store in VX, VF = shifted bit |
| `8XY7` | Set VX = VY - VX, VF = no-borrow |
| `8XYE` | Shift VY left, store in VX, VF = shifted bit |
| `9XY0` | Skip if VX != VY |
| `ANNN` | Set I = NNN |
| `BNNN` | Jump to NNN + V0 |
| `CXNN` | Set VX = random AND NN |
| `DXYN` | Draw N-row sprite at (VX, VY), VF = collision |
| `EX9E` | Skip if key VX is pressed |
| `EXA1` | Skip if key VX is not pressed |
| `FX07` | Set VX = delay timer |
| `FX0A` | Wait for keypress, store in VX |
| `FX15` | Set delay timer = VX |
| `FX18` | Set sound timer = VX |
| `FX1E` | Set I += VX |
| `FX29` | Set I = sprite address for digit VX |
| `FX33` | Store BCD of VX at I, I+1, I+2 |
| `FX55` | Store V0–VX in memory starting at I |
| `FX65` | Load V0–VX from memory starting at I |

## Dev Dependencies

- [SDL2](https://www.libsdl.org/)

On Debian/Ubuntu:

```sh
sudo apt install libsdl2-dev
```

## Building

```sh
gcc main.c -o chip8 -lSDL2
```

## Usage

```sh
./chip8 <path-to-rom>
```

Example:

```sh
./chip8 roms/pong.ch8
```

Press `Escape` to quit.

## Keypad mapping

The original CHIP-8 keypad (0–F) maps to the keyboard as follows:

```
CHIP-8    Keyboard
1 2 3 C   1 2 3 4
4 5 6 D   Q W E R
7 8 9 E   A S D F
A 0 B F   Z X C V
```

## Tests
This project has been tested thanks to mostly [Timendus test suite](https://github.com/Timendus/chip8-test-suite), corax89 and probably many others, Thank you!

![Timendus logo](/docs/logo.png "Timendus logo")

![Test](/docs/test.png "Test")

## License
[MIT](LICENSE)
