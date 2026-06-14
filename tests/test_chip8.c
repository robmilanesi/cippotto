#include "unity.h"
#include "../chip8.h"

void setUp(void) {}
void tearDown(void) {}

void test_init_chip8_pc_is_0x200(void) {
    Chip8 chip8 = init_chip8();
    TEST_ASSERT_EQUAL(0x200, chip8.pc);
}

void test_init_chip8_memory_is_zeroed(void) {
    Chip8 chip8 = init_chip8();
    TEST_ASSERT_EQUAL(0, chip8.v[0]);
    TEST_ASSERT_EQUAL(0, chip8.sp);
    TEST_ASSERT_EQUAL(0, chip8.delay_timer);
}

void test_decode_0x6A3F() {
    uint16_t instruction = 0x6A3F;
    DecodedInstruction actual = decode(instruction);
    TEST_ASSERT_EQUAL(instruction, actual.original);
    TEST_ASSERT_EQUAL(0x6, actual.nibble);
    TEST_ASSERT_EQUAL(0xA, actual.x);
    TEST_ASSERT_EQUAL(0x3, actual.y);
    TEST_ASSERT_EQUAL(0x3F, actual.nn);
    TEST_ASSERT_EQUAL(0xA3F, actual.nnn);
}

void test_execute_0x6XNN() {
    Chip8 chip8 = init_chip8();
    DecodedInstruction decoded = decode(0x6123);
    execute(&chip8, &decoded);
    TEST_ASSERT_EQUAL(0x23, chip8.v[1]);
}

void test_execute_0x1NNN() {
    Chip8 chip8 = init_chip8();
    DecodedInstruction decoded = decode(0x1123);
    execute(&chip8, &decoded);
    TEST_ASSERT_EQUAL(0x123, chip8.pc);
}

void test_execute_0x2NNN() {
    Chip8 chip8 = init_chip8();
    chip8.pc = 0x321;
    DecodedInstruction decoded = decode(0x2123);
    execute(&chip8, &decoded);
    TEST_ASSERT_EQUAL(0x321, chip8.stack[chip8.sp - 1]);
    TEST_ASSERT_EQUAL(1, chip8.sp);
    TEST_ASSERT_EQUAL(0x123, chip8.pc);
}

void test_execute_0x3NNN_skip() {
    Chip8 chip8 = init_chip8();
    chip8.v[1] = 0x23;
    DecodedInstruction decoded = decode(0x3123);
    execute(&chip8, &decoded);
    TEST_ASSERT_EQUAL(0x200 + 2, chip8.pc);
}

void test_execute_0x3NNN_noskip() {
    Chip8 chip8 = init_chip8();
    DecodedInstruction decoded = decode(0x3123);
    execute(&chip8, &decoded);
    TEST_ASSERT_EQUAL(0x200, chip8.pc);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_chip8_pc_is_0x200);
    RUN_TEST(test_init_chip8_memory_is_zeroed);
    RUN_TEST(test_decode_0x6A3F);
    RUN_TEST(test_execute_0x1NNN);
    RUN_TEST(test_execute_0x2NNN);
    RUN_TEST(test_execute_0x3NNN_skip);
    RUN_TEST(test_execute_0x3NNN_noskip);
    RUN_TEST(test_execute_0x6XNN);
    return UNITY_END();
}
