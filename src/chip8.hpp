#pragma once

#include <string>

namespace c8 {

class chip8 {
public:
    chip8();
    ~chip8();

    void load_rom(const std::string& path);
    void tick();
    void dump() const;
    void print_frame_buffer() const;

    void reset();
    void run();
private:
    uint8_t ram[0x1000];
    uint16_t pc;
    uint8_t reg_v[0x10];
    uint16_t reg_i;
    uint16_t stack[0x10];
    uint8_t sp;

    uint8_t keys[16];
    uint8_t frame_buffer[64 * 32];

    uint8_t st;
    uint8_t dt;
};

}
