#include "gb.h"
#include <cstdint>
#include <iostream>

/*void GameBoy::opcode(uint8_t inst) {
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
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x05: { // DEC B: decrement value in B
            int lowerPrev = (*reg.b) & 0x0F;
            (*reg.b)--;
            if (*reg.b == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x06: // LD B, n8: load 8bit number into B
            *reg.b = bus[pc++];
            t_cycle += 8;
            break;
        case 0x07: { // RLCA: rotate A register left circular
            int b7 = (*reg.a & 0x80) >> 7;
            reg.setFlag('c', b7); // set C to bit 7
            *reg.a = *reg.a << 1; // rotate left 1 bit
            *reg.a |= b7; // set new b0 to former b7
            reg.setFlag('z', 0);
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x09: { // ADD HL, BC: add value of BC to HL and store in HL
            uint16_t hl = reg.hl;
            uint16_t bc = reg.bc;
            uint32_t sum = hl + bc;
            reg.hl = static_cast<uint16_t>(sum);
            reg.setFlag('n', 0);
            reg.setFlag('h', ((hl & 0x0FFF) + (bc & 0x0FFF)) > 0x0FFF);
            reg.setFlag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x0A: // LD A, [BC]: load value at addr in BC to A
            *reg.a = bus[reg.bc];
            t_cycle += 8;
            break;
        case 0x0B: // DEC BC: decrement value in BC
            reg.bc--;
            t_cycle += 8;
            break;
        case 0x0C: { // INC C: increment value in C
            int lowerPrev = (*reg.c) & 0x0F;
            (*reg.c)++;
            if (*reg.c == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x0D: { // DEC C: decrement value in C
            int lowerPrev = (*reg.b) & 0x0F;
            (*reg.b)--;
            if (*reg.b == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x0E: // LD C, n8: load 8 bit num to C
            *reg.c = bus[pc++];
            t_cycle += 8;
            break;
        case 0x0F: { // RRCA: rotate A circular right
            int b0 = *reg.a & 0x01;
            reg.setFlag('c', b0); // set C to bit 0
            *reg.a = *reg.a >> 1; // rotate right 1 bit
            *reg.a |= (b0 << 7); // set new b7 to former b0
            reg.setFlag('z', 0);
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            t_cycle += 4;
            break;
        }

        case 0x11: { // LD DE, n16: load 16bit number into DE
            int lsb = bus[pc++];
            int msb = bus[pc++];
            reg.de = (msb << 8) + lsb;
            t_cycle += 12;
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
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x15: { // DEC D: decrement value in D
            int lowerPrev = (*reg.d) & 0x0F;
            (*reg.d)--;
            if (*reg.d == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
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
            reg.setFlag('c', b7); // set C to bit 7
            reg.setFlag('z', 0);
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x19: { // ADD HL, DE: add value of DE to HL and store in HL
            uint16_t hl = reg.hl;
            uint16_t de = reg.de;
            uint32_t sum = hl + de;
            reg.hl = static_cast<uint16_t>(sum);
            reg.setFlag('n', 0);
            reg.setFlag('h', ((hl & 0x0FFF) + (de & 0x0FFF)) > 0x0FFF);
            reg.setFlag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x1A: // LD A, [DE]: load value at addr in DE to A
            *reg.a = bus[reg.de];
            t_cycle += 8;
            break;
        case 0x1B: // DEC DE: decrement value in DE
            reg.de--;
            t_cycle += 8;
            break;
        case 0x1C: { // INC E: increment value in E
            int lowerPrev = (*reg.e) & 0x0F;
            (*reg.e)++;
            if (*reg.e == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x1D: { // DEC E: decrement value in E
            int lowerPrev = (*reg.e) & 0x0F;
            (*reg.e)--;
            if (*reg.e == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
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
            reg.setFlag('c', b0);
            reg.setFlag('z', 0);
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            t_cycle += 4;
            break;
        }
        case 0x20: { // JR NZ, e8: jump to pc+e8 if z not set
            int8_t e8 = bus[pc++];
            if (!reg.getFlag('z')) {
                pc += e8;
                t_cycle += 12;
            } else t_cycle += 8;
            break;
        }
        case 0x21: { // LD HL, n16: load 16bit number into HL
            int lsb = bus[pc++];
            int msb = bus[pc++];
            reg.hl = (msb << 8) + lsb;
            t_cycle += 12;
            break;
        }
        case 0x22: // LD [HL+], A: load val from a to addr stored in HL, then increment HL
            bus[reg.hl++] = *reg.a;
            t_cycle += 8;
            break;
        case 0x23: // INC HL: increment value in HL
            reg.hl++;
            t_cycle += 8;
            break;
        case 0x24: { // INC H: increment value in H
            int lowerPrev = (*reg.h) & 0x0F;
            (*reg.h)++;
            if (*reg.h == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x25: { // DEC H: decrement value in H
            int lowerPrev = (*reg.h) & 0x0F;
            (*reg.h)--;
            if (*reg.h == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x26: // LD H, n8: load 8bit number into H
            *reg.h = bus[pc++];
            t_cycle += 8;
            break;
        case 0x27: // DAA: Decimal Adjust Accumulator
            if (reg.getFlag('n')) {
                int adj = 0;
                adj += reg.getFlag('h') ? 0x6 : 0;
                adj += reg.getFlag('c') ? 0x60 : 0;
                *reg.a -= adj;
            } else {
                int adj = 0;
                adj += (reg.getFlag('h') || (*reg.a & 0xF) > 0x9) ? 0x6 : 0;
                if (reg.getFlag('c') || *reg.a > 0x99) {
                    adj += 60;
                    reg.setFlag('c', 1);
                }
                *reg.a += adj;
            }
            reg.setFlag('h', 0);
            if (*reg.a == 0) reg.setFlag('z', 1);
            t_cycle += 4;
            break;
        case 0x28: { // JR Z, e8: jump to pc+e8 if z set
            int8_t e8 = bus[pc++];
            if (reg.getFlag('z')) {
                pc += e8;
                t_cycle += 12;
            } else t_cycle += 8;
            break;
        }
        case 0x29: { // ADD HL, HL: add value of HL to HL and store in HL
            uint16_t hl = reg.hl;
            uint32_t sum = 2*hl;
            reg.hl = static_cast<uint16_t>(sum);
            reg.setFlag('n', 0);
            reg.setFlag('h', (2*(hl & 0x0FFF)) > 0x0FFF);
            reg.setFlag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x2A: // LD A, [HL+]: load val from addr in HL to A then increment HL
            *reg.a = bus[reg.hl++];
            t_cycle += 8;
            break;
        case 0x2B: // DEC HL: decrement value in HL
            reg.hl--;
            t_cycle += 8;
            break;
        case 0x2C: { // INC L: increment value in L
            int lowerPrev = (*reg.l) & 0x0F;
            (*reg.l)++;
            if (*reg.l == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x2D: { // DEC L: decrement value in L
            int lowerPrev = (*reg.l) & 0x0F;
            (*reg.l)--;
            if (*reg.l == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x2E: // LD L, n8: load 8 bit num to L
            *reg.l = bus[pc++];
            t_cycle += 8;
            break;
        case 0x2F: // CPL: bitwise NOT (ComPLement accumulator)
            *reg.a = ~(*reg.a);
            reg.setFlag('n', 1);
            reg.setFlag('h', 1);
            break;
        case 0x31: { // LD SP, n16: load 16bit number into sp
            int lsb = bus[pc++];
            int msb = bus[pc++];
            sp = (msb << 8) + lsb;
            t_cycle += 12;
            break;
        }
        case 0x32: // LD [HL-], A: load val from A to addr in HL then decrement HL
            bus[reg.hl--] = *reg.a;
            t_cycle += 8;
            break;
        case 0x33: // INC SP: increment stack pointer
            sp++;
            t_cycle += 8;
            break;
        case 0x34: { // INC [HL]: incrememnt value at addr stored in HL
            uint8_t data = bus[reg.hl];
            int lowerPrev = data & 0x0F;
            data++;
            bus[reg.hl] = data;
            if (data == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 12;
            break;
        }
        case 0x35: { // DEC [HL]: decrement value at addr stored in HL
            uint8_t data = bus[reg.hl];
            int lowerPrev = data & 0x0F;
            data--;
            bus[reg.hl] = data;
            if (data == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 12;
            break;
        }
        case 0x36: // LD [HL], n8: load 8bit number to addr stored in HL
            bus[reg.hl] = bus[pc++];
            t_cycle += 12;
            break;
        case 0x37: // SCF: set carry flag
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            reg.setFlag('c', 1);
            t_cycle += 4;
            break;
        case 0x38: { // JR C, e8: jump to pc+e8 if c set
            int8_t e8 = bus[pc++];
            if (reg.getFlag('c')) {
                pc += e8;
                t_cycle += 12;
            } else t_cycle += 8;
            break;
        }
        case 0x39: { // ADD HL, SP: add value of SP to HL and store in HL
            uint16_t hl = reg.hl;
            uint16_t _sp = this->sp;
            uint32_t sum = hl + sp;
            reg.hl = static_cast<uint16_t>(sum);
            reg.setFlag('n', 0);
            reg.setFlag('h', ((hl & 0x0FFF) + (sp & 0x0FFF)) > 0x0FFF);
            reg.setFlag('c', sum > 0xFFFF);
            t_cycle += 8;
            break;
        }
        case 0x3A: // LD A, [HL-]: load val from addr in HL to A then decrement HL
            *reg.a = bus[reg.hl--];
            t_cycle += 8;
            break;
        case 0x3B: // DEC SP: decrement stack pointer
            sp--;
            t_cycle += 8;
            break;
        case 0x3C: { // INC A: increment value in A
            int lowerPrev = (*reg.a) & 0x0F;
            (*reg.a)++;
            if (*reg.a == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x0F) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 0);
            t_cycle += 4;
            break;
        }
        case 0x3D: { // DEC A: decrement value in A
            int lowerPrev = (*reg.a) & 0x0F;
            (*reg.a)--;
            if (*reg.a == 0) {
                reg.setFlag('z', 1);
            }
            if (lowerPrev == 0x00) {
                reg.setFlag('h', 1);
            }
            reg.setFlag('n', 1);
            t_cycle += 4;
            break;
        }
        case 0x3E: // LD A, n8: load 8bit number to A
            *reg.l = bus[pc++];
            t_cycle += 8;
            break;
        case 0x3F: // CCF: Complement (flip) carry flag
            reg.setFlag('n', 0);
            reg.setFlag('h', 0);
            reg.setFlag('c', !reg.getFlag('c'));
            t_cycle += 4;
            break;
        case 0x40:
        default:
            std::cout << "Not yet implemented." << std::endl;
    }

}*/

// octal decoding
void GameBoy::opcode(uint8_t inst) {
    int x = inst >> 6;
    int y = (inst & 0x38) >> 3;
    int z = inst & 0x07;
    int p = y >> 1;
    int q = y % 2;

    switch (x) {
        case 0:
            switch (z) {
                case 0: // Relative jumps / assorted operations
                    switch (y) {
                        case 0: // nop
                            t_cycle += 4;
                            break;
                        case 1: { // LD [a16], SP: load from stack pointer
                            uint8_t lsb = bus[pc++];
                            uint8_t msb = bus[pc++];
                            uint16_t num = (msb << 8) + lsb;
                            bus[num++] = sp & 0x00FF;
                            bus[num] = sp >> 8;
                            t_cycle += 20;
                            break;
                        }
                        case 2: // STOP n8: weird behaviour
                            std::cout << "STOP n8 not currently implemented" << std::endl;
                            pc++;
                            t_cycle += 4;
                            break;
                        case 3: { // JR e8: relative jump
                            int8_t e8 = bus[pc++];
                            pc += e8;
                            break;
                        }
                        case 4:
                        case 5:
                        case 6:
                        case 7: // JR cc[y-4], e8
                            jr_cc_e8(reg.CC[y-4]);
                            break;
                    }
                    break;
                case 1: // 16-bit load immediate/add
                    if (q) { // ADD HL, rp[p]
                        add_hl_rpp(p);
                    } else { // LD rp[p], nn
                        ld_rpp_nn(p);
                    }
                    break;
                case 2: // Indirect loading
                    if (q) { // load to A

                    } else { // load from A

                    }
            }
            break;
    }
}

void GameBoy::jr_cc_e8(std::function<bool()> cc) { // JR cc, e8: jump to pc+e8 if cc
    int8_t e8 = bus[pc++];
    if (cc()) {
        pc += e8;
        t_cycle += 12;
    } else t_cycle += 8;
}

void GameBoy::add_hl_rpp(int p) { // ADD HL, rp[p]: add value of rp[p] to HL and store in HL
    uint16_t hl = reg.hl;
    uint16_t rpp = *rp[p]();
    uint32_t sum = hl + rpp;
    reg.hl = static_cast<uint16_t>(sum);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((hl & 0x0FFF) + (rpp & 0x0FFF)) > 0x0FFF);
    reg.setFlag('c', sum > 0xFFFF);
    t_cycle += 8;
}

void GameBoy::ld_rpp_nn(int p) { // LD rp[p], nn: load nn into rp[p]
    int lsb = bus[pc++];
    int msb = bus[pc++];
    *rp[p]() = (msb << 8) + lsb;
    t_cycle += 12;
}
