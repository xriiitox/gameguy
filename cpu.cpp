#include "gb.h"
#include <cstdint>
#include <iostream>

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
                            t_cycle += 12;
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
                    if (q) { // load to addr
                        switch (p) {
                            case 0: // LD [BC], A
                                bus[reg.bc] = *reg.a;
                                break;
                            case 1: // LD [DE], A
                                bus[reg.de] = *reg.a;
                                break;
                            case 2: // LD [HL+], A
                                bus[reg.hl++] = *reg.a;
                                break;
                            case 3: // LD [HL-], A
                                bus[reg.hl--] = *reg.a;
                                break;
                        }
                    } else { // load to A
                        switch (p) {
                            case 0: // LD A, [BC]
                                *reg.a = bus[reg.bc];
                                break;
                            case 1: // LD A, [DE]
                                *reg.a = bus[reg.de];
                                break;
                            case 2: // LD A, [HL+]
                                *reg.a = bus[reg.hl++];
                                break;
                            case 3: // LD A, [HL-]
                                *reg.a = bus[reg.hl--];
                                break;
                        }
                    }
                    t_cycle += 8;
                    break;
                case 3: // 16bit inc/dec
                    if (q) { // dec
                        dec16(p);
                    } else { // inc
                        inc16(p);
                    }
                    break;
                case 4: // 8bit inc
                    inc8(y);
                    break;
                case 5: // 8bit dec
                    dec8(y);
                    break;
                case 6: // LD r[y], n
                    *r[y]() = bus[pc++];
                    t_cycle += 8;
                    break;
                case 7: // accumulator/flag operations
                    flag_ops[y]();
                    break;
            }
            break;
        case 1: // 8bit loading (LD r8, r8)
            if (z == 6 && y == 6) { // HALT: halt system clock
                // TODO?
                break;
            }
            *r[y]() = *r[z]();
            t_cycle += (z == 6) ^ (y == 6) ? 8 : 4;
            break;
        case 2: // arithmetic/logic ops with register/mem loc
            alu_r[y](z);
            break;
        case 3:
            switch (z) {
                case 0:
                    switch (y) {
                        case 0:
                        case 1:
                        case 2:
                        case 3: // RET cc[y]
                            ret_cc(reg.CC[y]);
                            break;
                        case 4: { // LD [0xFF00 + n8], A
                            uint8_t n8 = bus[pc++];
                            bus[0xFF00 + n8] = *reg.a;
                            t_cycle += 12;
                            break;
                        }
                        case 5: // ADD SP, e8
                            add_sp_e();
                            break;
                        case 6: { // LD A, [0xFF00 + n8]
                            uint8_t n8 = bus[pc++];
                            *reg.a = bus[0xFF00 + n8];
                            t_cycle += 12;
                            break;
                        }
                        case 7: // LD HL, SP + e8
                            ld_hl_sp_e();
                            break;
                    }
                    break;
                case 1: // POP + other ops
                    if (q) {
                        switch (p) {
                            case 0: { // RET
                                uint8_t lsb = bus[sp++];
                                uint8_t msb = bus[sp++];
                                pc = ((uint16_t)msb << 8) + lsb;
                                t_cycle += 20;
                                break;
                            }
                            case 1: {// RETI
                                uint8_t lsb = bus[sp++];
                                uint8_t msb = bus[sp++];
                                pc = ((uint16_t)msb << 8) + lsb;
                                ime = 1;
                                t_cycle += 20;
                                break;
                            }
                            case 2: // JP HL
                                pc = reg.hl;
                                t_cycle += 4;
                                break;
                            case 3: // LD SP, HL
                                sp = reg.hl;
                                t_cycle += 8;
                                break;
                        }
                    } else { // POP rp2[p]
                        uint8_t lsb = bus[sp++];
                        uint8_t msb = bus[sp++];
                        *rp2[p]() = ((uint16_t)msb << 8) + lsb;
                        t_cycle += 12;
                    }
                    break;
                case 2: // Conditional jump
                    switch (y) {
                        case 0:
                        case 1:
                        case 2:
                        case 3: // JP cc[y], n16
                            jp_cc_n16(reg.CC[y]);
                            break;
                        case 4: // LD [0xFF00 + C], A
                            bus[0xFF00 + *reg.c] = *reg.a;
                            t_cycle += 8;
                            break;
                        case 5: { // LD [n16], A
                            uint8_t lsb = bus[pc++];
                            uint8_t msb = bus[pc++];
                            uint16_t n16 = ((uint16_t)msb << 8) + lsb;
                            bus[n16] = *reg.a;
                            t_cycle += 16;
                            break;
                        }
                        case 6: // LD A, [0xFF00 + C]
                            *reg.a = bus[0xFF00 + *reg.c];
                            t_cycle += 8;
                            break;
                        case 7: { // LD A, [n16]
                            uint8_t lsb = bus[pc++];
                            uint8_t msb = bus[pc++];
                            uint16_t n16 = ((uint16_t)msb << 8) + lsb;
                            *reg.a = bus[n16];
                            t_cycle += 16;
                            break;
                        }
                    }
                    break;
                case 3: // assorted ops
                    switch (y) {
                        case 0: // JP n16
                            break;
                    }
                    break;
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

void GameBoy::dec16(int p) {
    (*rp[p]())--;
    t_cycle += 8;
}

void GameBoy::inc16(int p) {
    (*rp[p]())++;
    t_cycle += 8;
}

void GameBoy::inc8(int y) {
    (*r[y]())++;
    t_cycle += (y == 6) ? 12 : 4;
}

void GameBoy::dec8(int y) {
    (*r[y]())++;
    t_cycle += (y == 6) ? 12 : 4;
}

// Assorted accumulator/flag operations block
void GameBoy::rlca() { // RLCA: rotate A register left circular
    int b7 = (*reg.a & 0x80) >> 7;
    reg.setFlag('c', b7); // set C to bit 7
    *reg.a = *reg.a << 1; // rotate left 1 bit
    *reg.a |= b7; // set new b0 to former b7
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    t_cycle += 4;
}
void GameBoy::rrca() { // RRCA: rotate A circular right
    int b0 = *reg.a & 0x01;
    reg.setFlag('c', b0); // set C to bit 0
    *reg.a = *reg.a >> 1; // rotate right 1 bit
    *reg.a |= (b0 << 7); // set new b7 to former b0
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    t_cycle += 4;
}
void GameBoy::rla() { // RLA: rotate A register left (through carry)
    int b7 = (*reg.a & 0x80) >> 7;
    int oldC = (*reg.f & (1 << 4)) >> 4;
    *reg.a = (*reg.a << 1) + oldC;
    reg.setFlag('c', b7); // set C to bit 7
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    t_cycle += 4;
}
void GameBoy::rra() { // RRA: rotate A right (through carry)
    int b0 = *reg.a & 0x01;
    int c = (*reg.f & 0x10) >> 4;
    *reg.a = *reg.a >> 1; // rotate right 1 bit
    *reg.a |= (c << 7); // set new b7 to former C
    reg.setFlag('c', b0);
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    t_cycle += 4;
}
void GameBoy::daa() { // DAA: Decimal Adjust Accumulator
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
}
void GameBoy::cpl() { // CPL: bitwise NOT (ComPLement accumulator)
    *reg.a = ~(*reg.a);
    reg.setFlag('n', 1);
    reg.setFlag('h', 1);
    t_cycle += 4;
}
void GameBoy::scf() { // SCF: set carry flag
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 1);
    t_cycle += 4;
}
void GameBoy::ccf() { // CCF: Complement (flip) carry flag
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', !reg.getFlag('c'));
    t_cycle += 4;
}

// alu ops
void GameBoy::add_a(int z) { // ADD A, r8: add value from r8 to A and store in A
    uint8_t a = *reg.a;
    uint8_t r8 = *r[z]();
    uint16_t sum = a + r8;

    *reg.a = (uint8_t)sum;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((a & 0x0F) + (r8 & 0x0F)) > 0x0F);
    reg.setFlag('c', sum > 0xFF);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::adc_a(int z) { // ADC A, r8: add value from carry and r8 to A and store in A
    uint16_t a = *reg.a;
    uint16_t r8 = *r[z]();
    uint16_t c = (reg.getFlag('c') ? 1 : 0);
    uint16_t sum = a + r8 + c;

    *reg.a = (uint8_t)sum;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((a & 0x0F) + (r8 & 0x0F) + c) > 0x0F);
    reg.setFlag('c', sum > 0xFF);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::sub_a(int z) { // SUB A, r8: subtract r8 from A and store in A
    uint16_t a = *reg.a;
    uint16_t r8 = *r[z]();
    uint16_t diff = a - r8;

    *reg.a = (uint8_t)diff;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < (r8 & 0x0F));
    reg.setFlag('c', a < r8);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::sbc_a(int z) { // SBC A, r8: subtract r8 and carry from A and store in A
    uint16_t a = *reg.a;
    uint16_t r8 = *r[z]();
    uint16_t c = reg.getFlag('c') ? 1 : 0;
    uint16_t diff = a - r8 - c;

    *reg.a = (uint8_t)diff;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < ((r8 & 0x0F) + c));
    reg.setFlag('c', a < (uint16_t)r8 + c);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::and_a(int z) { // AND A, r8: bitwise and between A and r8 and store in A
    *reg.a = *reg.a & *r[z]();
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 1);
    reg.setFlag('c', 0);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::xor_a(int z) { // XOR A, r8: bitwise xor between A and r8 and store in A
    *reg.a = *reg.a ^ *r[z]();
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 0);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::or_a(int z) { // OR A, r8: bitwise or between A and r8 and store in A
    *reg.a = *reg.a | *r[z]();
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 0);
    t_cycle += (z == 6) ? 8 : 4;
}
void GameBoy::cp_a(int z) { // CP A, r8: subtract r8 from A and update flags
    uint16_t a = *reg.a;
    uint16_t r8 = *r[z]();
    uint16_t diff = a - r8;

    reg.setFlag('z', diff == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < (r8 & 0x0F));
    reg.setFlag('c', a < r8);
    t_cycle += (z == 6) ? 8 : 4;
}

void GameBoy::ret_cc(std::function<bool()> cc) { // RET cc: conditional return
    if (cc()) {
        uint8_t lsb = bus[sp++];
        uint8_t msb = bus[sp++];
        pc = ((uint16_t)msb << 8) + (uint16_t)lsb;
        t_cycle += 20;
    } else t_cycle += 8;
}

void GameBoy::add_sp_e() { // ADD SP, e8: add signed 8bit offset to stack pointer
    int8_t e8 = (int8_t)bus[pc++];
    uint16_t _sp = sp;
    uint16_t sum = _sp + e8;

    sp = sum;

    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((_sp & 0x0F) + ((uint8_t)e8 & 0x0F)) > 0x0F);
    reg.setFlag('c', ((_sp & 0xFF) + ((uint8_t)e8 & 0xFF)) > 0xFF);
    t_cycle += 16;
}

void GameBoy::ld_hl_sp_e() { // LD HL, SP+e8: load to HL the value of e8 + sp
    int8_t e8 = (int8_t)bus[pc++];
    uint16_t sum = sp + e8;

    reg.hl = sum;

    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((sp & 0x0F) + ((uint8_t)e8 & 0x0F)) > 0x0F);
    reg.setFlag('c', ((sp & 0xFF) + ((uint8_t)e8 & 0xFF)) > 0xFF);
    t_cycle += 12;
}

void GameBoy::jp_cc_n16(std::function<bool()> cc) { // JP cc, nn: conditional jump to nn
    uint8_t lsb = bus[pc++];
    uint8_t msb = bus[pc++];
    uint16_t n16 = ((uint16_t)msb << 8) + lsb;
    if (cc()) {
        pc = n16;
        t_cycle += 16;
    } else t_cycle += 12;
}
