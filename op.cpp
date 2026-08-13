#include "gb.h"
#include <iostream>

using namespace std;

void GameBoy::opcode(uint8_t inst) {
    switch (inst) {
        case 0x00: // nop
            t_cycle += 4;
            break;
        case 0x01: {// LD BC,n16: load 16bit number into BC
            int lsb = bus[pc++];
            int msb = bus[pc++];
            reg.bc = (msb << 8) + lsb;
            t_cycle += 12;
            pc += 2;
            break;
        }
        case 0x02: // LD [BC], A: copy val in A to mem addr in BC
            bus[reg.bc] = *reg.a;
            t_cycle += 2;
            break;
        case 0x03: // INC BC: increment value in BC
            reg.bc++;
            t_cycle += 8;
            break;
        case 0x04: { // INC B: increment value in B
            int lowerPrev = (*reg.b) & 0x0F;
            (*reg.b)++;
            if (*reg.b == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                flag('h', 1);
            }
            flag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x05: { // DEC B: decrement value in B
            int lowerPrev = (*reg.b) & 0x0F;
            (*reg.b)--;
            if (*reg.b == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x00) {
                flag('h', 1);
            }
            flag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x06: // LD B, n8: load 8bit number into B
            *reg.b = bus[pc++];
            t_cycle += 8;
            break;
        case 0x07: { // RLCA: rotate A register left circular
            int b7 = (*reg.a & 0x80) >> 7;
            flag('c', b7); // set C to bit 7
            *reg.a = *reg.a << 1; // rotate left 1 bit
            *reg.a |= b7; // set new b0 to former b7
            flag('z', 0);
            flag('n', 0);
            flag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x08: { // LD [a16], SP: load from stack pointer
            uint8_t lsb = bus[pc++];
            uint8_t msb = bus[pc++];
            uint16_t num = (msb << 8) + lsb;
            bus[num++] = sp & 0x00FF;
            bus[num] = sp >> 8;
            t_cycle += 20;
            break;
        }
        case 0x09: { // ADD HL, BC: add value of BC to HL and store in HL
            uint16_t hl = reg.hl;
            uint16_t bc = reg.bc;
            uint32_t sum = hl + bc;
            reg.hl = static_cast<uint16_t>(sum);
            flag('n', 0);
            flag('h', ((hl & 0x0FFF) + (bc & 0x0FFF)) > 0x0FFF);
            flag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x0A: // LD A, [BC]: load value at addr in BC to A
            *reg.a = bus[reg.bc];
            t_cycle += 8;
            break;
        case 0x0B: // DEC BC: decrement value in BC
            reg.bc--;
            break;
        case 0x0C: { // INC C: increment value in C
            int lowerPrev = (*reg.c) & 0x0F;
            (*reg.c)++;
            if (*reg.c == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                flag('h', 1);
            }
            flag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x0D: { // DEC C: decrement value in C
            int lowerPrev = (*reg.b) & 0x0F;
            (*reg.b)--;
            if (*reg.b == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x00) {
                flag('h', 1);
            }
            flag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x0E: // LD C, n8: load 8 bit num to C
            *reg.c = bus[pc++];
            t_cycle += 8;
            break;
        case 0x0F: { // RRCA: rotate A circular right
            int b0 = *reg.a & 0x01;
            flag('c', b0); // set C to bit 0
            *reg.a = *reg.a >> 1; // rotate right 1 bit
            *reg.a |= (b0 << 7); // set new b7 to former b0
            flag('z', 0);
            flag('n', 0);
            flag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x10: // STOP n8: freaky behavior
            cout << "STOP n8 not currently implemented" << endl;
            break;
        case 0x11: { // LD DE, n16: load 16bit number into DE
            int lsb = bus[pc++];
            int msb = bus[pc++];
            reg.de = (msb << 8) + lsb;
            t_cycle += 12;
            pc += 2;
            break;
        }
        case 0x12: // LD [DE], A: load value at addr in DE to A
            bus[reg.bc] = *reg.a;
            t_cycle += 2;
            break;
        case 0x13: // INC DE: increment value in DE
            reg.de++;
            t_cycle += 8;
            break;
        case 0x14: { // INC D: increment value in D
            int lowerPrev = (*reg.d) & 0x0F;
            (*reg.d)++;
            if (*reg.d == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                flag('h', 1);
            }
            flag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x15: { // DEC D: decrement value in D
            int lowerPrev = (*reg.d) & 0x0F;
            (*reg.d)--;
            if (*reg.d == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x00) {
                flag('h', 1);
            }
            flag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x16: // LD D, n8: load 8bit number into D
            *reg.d = bus[pc++];
            t_cycle += 8;
            break;
        case 0x17: { // RLA: rotate A register left (through carry)
            int b7 = (*reg.a & 0x80) >> 7;
            int oldC = (*reg.f & (1 << 4)) >> 4;
            *reg.a = (*reg.a << 1) + oldC;
            flag('c', b7); // set C to bit 7
            flag('z', 0);
            flag('n', 0);
            flag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x18: { // JR e8: Relative jump
            int8_t e8 = bus[pc++];
            pc += e8;
            break;
        }
        case 0x19: { // ADD HL, DE: add value of DE to HL and store in HL
            uint16_t hl = reg.hl;
            uint16_t de = reg.de;
            uint32_t sum = hl + de;
            reg.hl = static_cast<uint16_t>(sum);
            flag('n', 0);
            flag('h', ((hl & 0x0FFF) + (de & 0x0FFF)) > 0x0FFF);
            flag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x1A: // LD A, [DE]: load value at addr in DE to A
            *reg.a = bus[reg.de];
            t_cycle += 8;
            break;
        case 0x1B: // DEC DE: decrement value in DE
            reg.de--;
            break;
        case 0x1C: { // INC E: increment value in E
            int lowerPrev = (*reg.e) & 0x0F;
            (*reg.e)++;
            if (*reg.e == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                flag('h', 1);
            }
            flag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x1D: { // DEC E: decrement value in E
            int lowerPrev = (*reg.e) & 0x0F;
            (*reg.e)--;
            if (*reg.e == 0) {
                flag('z', 1);
            }
            if (lowerPrev == 0x00) {
                flag('h', 1);
            }
            flag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x1E: // LD E, n8: load 8 bit num to E
            *reg.e = bus[pc++];
            t_cycle += 8;
            break;
        case 0x1F: { // RRA: rotate A right (through carry)
            int b0 = *reg.a & 0x01;
            int c = (*reg.f & 0x10) >> 4;
            *reg.a = *reg.a >> 1; // rotate right 1 bit
            *reg.a |= (c << 7); // set new b7 to former C
            flag('c', b0);
            flag('z', 0);
            flag('n', 0);
            flag('h', 0);
            t_cycle += 4;
            break;
        }
        default:
            cout << "Not yet implemented." << endl;
    }

}

void GameBoy::flag(char fl, bool set) {
    if (set) {
        switch (fl) {
            case 'z':
                *reg.f |= (1 << 7);
                break;
            case 'n':
                *reg.f |= (1 << 6);
                break;
            case 'h':
                *reg.f |= (1 << 5);
                break;
            case 'c':
                *reg.f |= (1 << 4);
                break;
            default:
                cout << "something is wrong" << endl;
        }
    } else {
        switch (fl) {
            case 'z':
                *reg.f &= ~(1 << 7);
                break;
            case 'n':
                *reg.f &= ~(1 << 6);
                break;
            case 'h':
                *reg.f &= ~(1 << 5);
                break;
            case 'c':
                *reg.f &= ~(1 << 4);
                break;
            default:
                cout << "something is wrong";
        }
    }
}
