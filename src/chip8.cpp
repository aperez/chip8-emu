#include "chip8.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cstdlib>

namespace c8 {

uint8_t sprites[] =
    { 0xF0, 0x90, 0x90, 0x90, 0xF0, //0
      0x20, 0x60, 0x20, 0x20, 0x70, //1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
      0x90, 0x90, 0xF0, 0x10, 0x10, //4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
      0xF0, 0x10, 0x20, 0x40, 0x40, //7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
      0xF0, 0x90, 0xF0, 0x90, 0x90, //A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
      0xF0, 0x80, 0x80, 0x80, 0xF0, //C
      0xE0, 0x90, 0x90, 0x90, 0xE0, //D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
      0xF0, 0x80, 0xF0, 0x80, 0x80  //F
    };

chip8::chip8() {
    reset();
}

chip8::~chip8() {
}

void chip8::reset() {
    std::fill_n(ram, 0x1000, 0);
    std::fill_n(reg_v, 0x10, 0);
    std::fill_n(stack, 0x10, 0);

    std::fill_n(keys, 16, 0);
    std::fill_n(frame_buffer, 64*32, 0);

    std::copy(sprites, std::end(sprites), ram);

    pc = 0x200;
    reg_i = 0;
    sp = 0;
    st = 0;
    dt = 0;

    srand(0);
}

void chip8::load_rom(const std::string& path) {
    std::ifstream romfile(path, std::ios::binary);

    if (romfile.is_open()) {
        romfile.seekg(0, std::ios::end);
        size_t romsize = romfile.tellg();

        romfile.seekg(0, std::ios::beg);
        romfile.read((char*) &ram[0x200], romsize);

        romfile.close();
    }
}

void chip8::run() {
    //dump();
    for (int i = 0; i < 300; ++i ) {
        tick();
    }

    //dump();
    print_frame_buffer();
}

inline void unhandled_op(uint16_t op) {
    std::cout << "unhandled operation: 0x" << std::hex << std::uppercase << std::setw(4) << (int) op << std::endl;
}

#define addr(op) (op & 0x0FFF)
#define reg_vx(op) reg_v[(op & 0x0F00) >> 8]
#define reg_vy(op) reg_v[(op & 0x00F0) >> 4]
#define value(op) (op & 0x00FF)
#define frame_pixel(xpos, ypos) frame_buffer[xpos + (ypos * 64)]

void chip8::tick() {

    uint16_t opcode = (ram[pc & 0xFFF] << 8) | ram[(pc+1) & 0xFFF];
    pc += 2;

    switch (opcode & 0xF000) {
    case 0x0000: {
        if (opcode == 0x00E0) { std::fill_n(frame_buffer, 64*32, 0); }
        else if (opcode == 0x00EE) { pc = stack[--sp]; }
        else { unhandled_op(opcode); }
        break;
    }
    case 0x1000: { pc = addr(opcode); break; }
    case 0x2000: { stack[sp++] = pc; pc = addr(opcode); break; }
    case 0x3000: { if (reg_vx(opcode) == value(opcode)) { pc += 2; } break; }
    case 0x4000: { if (reg_vx(opcode) != value(opcode)) { pc += 2; } break; }
    case 0x5000: { if (reg_vx(opcode) == reg_vy(opcode)) { pc += 2; } break; }
    case 0x6000: { reg_vx(opcode) = value(opcode); break; }
    case 0x7000: { reg_vx(opcode) += value(opcode); break; }
    case 0x8000: {
        switch (opcode & 0x000F) {
        case 0x0000: { reg_vx(opcode) = reg_vy(opcode); break; }
        case 0x0001: { reg_vx(opcode) |= reg_vy(opcode); break; }
        case 0x0002: { reg_vx(opcode) &= reg_vy(opcode); break; }
        case 0x0003: { reg_vx(opcode) ^= reg_vy(opcode); break; }
        case 0x0004: {
            uint16_t result = reg_vx(opcode) + reg_vy(opcode);
            reg_v[0xF] = result > 0x00FF ? 1 : 0;
            reg_vx(opcode) = result & 0x00FF;
            break;
        }
        case 0x0005: {
            reg_v[0xF] = reg_vx(opcode) > reg_vy(opcode) ? 1 : 0;
            reg_vx(opcode) -= reg_vy(opcode);
            break;
        }
        case 0x0006: { reg_v[0xF] = reg_vx(opcode) & 0x1; reg_vx(opcode) >>= 1; break; }
        case 0x0007: {
            reg_v[0xF] = reg_vy(opcode) > reg_vx(opcode) ? 1 : 0;
            reg_vx(opcode) = reg_vy(opcode) - reg_vx(opcode);
            break;
        }
        case 0x000E: { reg_v[0xF] = reg_vx(opcode) >> 7; reg_vx(opcode) <<= 1; break; }
        default: { unhandled_op(opcode); break; }}
        break;
    }
    case 0x9000: { if (reg_vx(opcode) != reg_vy(opcode)) { pc += 2; } break; }
    case 0xA000: { reg_i = addr(opcode); break; }
    case 0xB000: { pc = addr(opcode) + reg_v[0]; break; }
    case 0xC000: { reg_vx(opcode) = rand() & value(opcode); break; }
    case 0xD000: {
        reg_v[0xF] = 0;
        for (uint16_t h = 0; h < (opcode & 0x000F); ++h) {
            uint8_t ypos = (reg_vy(opcode) + h) % 32;
            for (uint8_t x = 0; x < 8; ++x) {
                uint8_t xpos = (reg_vx(opcode) + x) % 64;
                if ((ram[reg_i + h] & (0x80 >> x)) != 0) {
                    reg_v[0xF] |= frame_pixel(xpos, ypos);
                    frame_pixel(xpos, ypos) ^= 1;
                }
            }
        }
        break;
    }
    case 0xE000: {
        switch (value(opcode)) {
        case 0x009E: { if (keys[reg_vx(opcode)] != 0) { pc += 2; } break; }
        case 0x00A1: { if (keys[reg_vx(opcode)] == 0) { pc += 2; } break; }
        default: { unhandled_op(opcode); break; }}
        break;
    }
    case 0xF000: {
        switch (value(opcode)) {
        case 0x0007: { reg_vx(opcode) = dt; break; }
        case 0x000A: {
            pc -= 2;
            for (size_t k = 0; k < 16; k++) {
                if (keys[k] == 1) {
                    reg_vx(opcode) = k;
                    pc += 2;
                    break;
                }
            }
            break;
        }
        case 0x0015: { dt = reg_vx(opcode); break; }
        case 0x0018: { st = reg_vx(opcode); break; }
        case 0x001E: {
            reg_v[0xF] = (reg_i + reg_vx(opcode)) > 0xFFF ? 1 : 0;
            reg_i += reg_vx(opcode);
            break;
        }
        case 0x0029: { reg_i = reg_vx(opcode) * 0x5; break; }
        case 0x0033: {
            ram[reg_i] = reg_vx(opcode) / 100;
            ram[reg_i + 1] = (reg_vx(opcode) / 10) % 10;
            ram[reg_i + 2] = reg_vx(opcode) % 10;
            break;
        }
        case 0x0055: {
            for (size_t n = 0; n < ((opcode & 0x0F00) >> 8); ++n) {
                ram[reg_i + n] = reg_v[n];
            }
            break;
        }
        case 0x0065: {
            for (size_t n = 0; n < ((opcode & 0x0F00) >> 8); ++n) {
                reg_v[n] = ram[reg_i + n];
            }
            break;
        }
        default: { unhandled_op(opcode); break; }}
        break;
    }
    default: { unhandled_op(opcode); break; }}

    if (dt > 0) { --dt; }
    if (st > 0) { --st; }
}

void chip8::dump() const {
    std::ios::fmtflags flags(std::cout.flags());

    std::cout.flags(std::ios::hex | std::ios::uppercase);
    std::cout.fill('0');

    for (size_t i = 0; i < 4096; i += 2) {
        if (i % 0x20 == 0) {
            std::cout << "\n0x" << std::setw(3) << i << ": ";
        } else if (i % 0x10 == 0) {
            std::cout << "| ";
        }
        std::cout << std::setw(2) << (int) ram[i]
                  << std::setw(2) << (int) ram[i+1] << " ";
    }
    std::cout << std::endl;

    std::cout << "V: ";
    for (size_t i = 0; i < 16; ++i) {
        std::cout << std::setw(4) << (int) reg_v[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Stack: ";
    for (size_t i = 0; i < 16; ++i) {
        std::cout << std::setw(4) << (int) stack[i] << " ";
    }
    std::cout << std::endl;

    std::cout.flags(flags);
}

void chip8::print_frame_buffer() const {

    for (size_t x = 0; x < 64; ++x) {
        std::cout << '-';
    }
    std::cout << "--\n";

    for (size_t y = 0; y < 32; ++y) {
        std::cout << '|';
        for (size_t x = 0; x < 64; ++x) {
            if(frame_pixel(x, y) == 1) {
                std::cout << '#';
            }
            else {
                std::cout << ' ';
            }
        }
        std::cout << "|\n";
    }

    for (size_t x = 0; x < 64; ++x) {
        std::cout << '-';
    }
    std::cout << "--" << std::endl;
}

}
