#include <cstdint>
#include <iostream>
#include "cpu.h"
#include "bus.h"
#include "gb.h"

CPU::CPU(Bus* bus, GameBoy* gb) {
    this->bus = bus;
    this->gb = gb;
}

// octal decoding
void CPU::opcode(uint8_t inst) {
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
                            break;
                        case 1: { // LD [a16], SP: load from stack pointer
                            uint8_t lsb = bus->read(pc++);
                            uint8_t msb = bus->read(pc++);
                            uint16_t num = (msb << 8) + lsb;
                            bus->write(num++, sp & 0x00FF);
                            bus->write(num, sp >> 8);
                            break;
                        }
                        case 2: // STOP n8: weird behaviour
                            std::cout << "STOP n8 not currently implemented" << std::endl;
                            pc++;
                            break;
                        case 3: { // JR e8: relative jump
                            int8_t e8 = bus->read(pc++);
                            pc += e8;
                            gb->tick();
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
                        switch (p) {
                            case 0: // LD A, [BC]
                                *reg.a = bus->read(reg.bc);
                                break;
                            case 1: // LD A, [DE]
                                *reg.a = bus->read(reg.de);
                                break;
                            case 2: // LD A, [HL+]
                                *reg.a = bus->read(reg.hl++);
                                break;
                            case 3: // LD A, [HL-]
                                *reg.a = bus->read(reg.hl--);
                                break;
                        }
                    } else { // load to addr
                        switch (p) {
                            case 0: // LD [BC], A
                                bus->write(reg.bc, *reg.a);
                                break;
                            case 1: // LD [DE], A
                                bus->write(reg.de, *reg.a);
                                break;
                            case 2: // LD [HL+], A
                                bus->write(reg.hl++, *reg.a);
                                break;
                            case 3: // LD [HL-], A
                                bus->write(reg.hl--, *reg.a);
                                break;
                        }
                    }
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
                    if (y == 6) {
                        bus->write(reg.hl, bus->read(pc++));
                        break;
                    }
                    *r[y]() = bus->read(pc++);
                    break;
                case 7: // accumulator/flag operations
                    flag_ops[y]();
                    break;
            }
            break;
        case 1: // 8bit loading (LD r8, r8)
            if (z == 6 && y == 6) { // HALT: halt system clock
                bool pending = (bus->ie & (bus->IF | 0xE0) & 0x1F) != 0;
                if (ime == 0 && pending) { halt_bug = true; break;}
                else if (ime != 0 && pending) { halted = false; break; }
                halted = true;
                break;
            }
            if (z == 6) {
                *r[y]() = bus->read(reg.hl);
                break;
            } else if (y == 6) {
                bus->write(reg.hl, *r[z]());
                break;
            }
            *r[y]() = *r[z]();
            break;
        case 2: // arithmetic/logic ops with register/mem loc
            alu[y](z, true);
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
                            uint8_t n8 = bus->read(pc++);
                            bus->write(0xFF00 | n8, *reg.a);
                            break;
                        }
                        case 5: // ADD SP, e8
                            add_sp_e();
                            break;
                        case 6: { // LD A, [0xFF00 + n8]
                            uint8_t n8 = bus->read(pc++);
                            *reg.a = bus->read(0xFF00 + n8);
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
                                uint8_t lsb = bus->read(sp++);
                                uint8_t msb = bus->read(sp++);
                                gb->tick();
                                pc = ((uint16_t)msb << 8) + lsb;
                                break;
                            }
                            case 1: {// RETI
                                ime = 1;
                                uint8_t lsb = bus->read(sp++);
                                uint8_t msb = bus->read(sp++);
                                gb->tick();
                                pc = ((uint16_t)msb << 8) + lsb;
                                break;
                            }
                            case 2: // JP HL
                                pc = reg.hl;
                                break;
                            case 3: // LD SP, HL
                                gb->tick();
                                sp = reg.hl;
                                break;
                        }
                    } else { // POP rp2[p]
                        uint8_t lsb = bus->read(sp);
                        sp++;
                        uint8_t msb = bus->read(sp);
                        sp++;
                        uint16_t val = ((uint16_t)msb << 8) | lsb;
                        if (p == 3) val &= 0xFFF0;
                        *rp2[p]() = val;
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
                            bus->write(0xFF00 + *reg.c, *reg.a);
                            break;
                        case 5: { // LD [n16], A
                            uint8_t lsb = bus->read(pc++);
                            uint8_t msb = bus->read(pc++);
                            uint16_t n16 = ((uint16_t)msb << 8) + lsb;
                            bus->write(n16, *reg.a);
                            break;
                        }
                        case 6: // LD A, [0xFF00 + C]
                            *reg.a = bus->read(0xFF00 + *reg.c);
                            break;
                        case 7: { // LD A, [n16]
                            uint8_t lsb = bus->read(pc++);
                            uint8_t msb = bus->read(pc++);
                            uint16_t n16 = ((uint16_t)msb << 8) + lsb;
                            *reg.a = bus->read(n16);
                            break;
                        }
                    }
                    break;
                case 3: // assorted ops
                    switch (y) {
                        case 0: { // JP n16
                            uint8_t lsb = bus->read(pc++);
                            uint8_t msb = bus->read(pc++);
                            uint16_t n16 = ((uint16_t)msb << 8) + lsb;
                            gb->tick();
                            pc = n16;
                            break;
                        }
                        case 1: // CB Prefix
                            cb(bus->read(pc++));
                            break;
                        case 6: // DI
                            ei_delay = 0;
                            ime = 0;
                            break;
                        case 7: // EI
                            if (ei_delay != 2) ei_delay = 1;
                            break;
                    }
                    break;
                case 4: // conditional call
                    switch (y) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            call_cc_nn(reg.CC[y]);
                            break;
                    }
                    break;
                case 5: // PUSH and various ops
                    if (q) { // CALL nn
                        call_nn();
                    } else { // PUSH rp2[p]
                        uint16_t val = *rp2[p]();
                        sp--;
                        bus->write(sp, (uint8_t)(val >> 8));
                        gb->tick();
                        sp--;
                        bus->write(sp, (uint8_t)(val & 0xFF));
                    }
                    break;
                case 6: // alu[y] n8: accumulator and immediate n8
                    alu[y](0, false);
                    break;
                case 7: // RST y*8
                    rst_y(y);
                    break;
            }
            break;

    }
}

void CPU::jr_cc_e8(std::function<bool()> cc) { // JR cc, e8: jump to pc+e8 if cc
    int8_t e8 = bus->read(pc++);
    if (cc()) {
        pc += e8;
        gb->tick();
    }
}

void CPU::add_hl_rpp(int p) { // ADD HL, rp[p]: add value of rp[p] to HL and store in HL
    uint16_t hl = reg.hl;
    uint16_t rpp = *rp[p]();
    uint32_t sum = hl + rpp;
    reg.hl = static_cast<uint16_t>(sum);
    gb->tick();
    reg.setFlag('n', 0);
    reg.setFlag('h', ((hl & 0x0FFF) + (rpp & 0x0FFF)) > 0x0FFF);
    reg.setFlag('c', sum > 0xFFFF);
}

void CPU::ld_rpp_nn(int p) { // LD rp[p], nn: load nn into rp[p]
    int lsb = bus->read(pc++);
    int msb = bus->read(pc++);
    *rp[p]() = (msb << 8) + lsb;
}

void CPU::dec16(int p) {
    (*rp[p]())--;
    gb->tick();
}

void CPU::inc16(int p) {
    (*rp[p]())++;
    gb->tick();
}

void CPU::inc8(int y) {
    if (y == 6) { // INC (HL)
        uint8_t old = bus->read(reg.hl, true);
        uint8_t val = old + 1;

        reg.setFlag('z', val == 0);
        reg.setFlag('n', 0);
        reg.setFlag('h', (old & 0x0F) == 0x0F);

        bus->write(reg.hl, val, true);
    } else { // INC r8
        uint8_t old = *r[y]();
        uint8_t val = old + 1;
        *r[y]() = val;

        reg.setFlag('z', val == 0);
        reg.setFlag('n', 0);
        reg.setFlag('h', (old & 0x0F) == 0x0F);
    }
}

void CPU::dec8(int y) {
    if (y == 6) { // DEC (HL)
            uint8_t old = bus->read(reg.hl, true);
            uint8_t val = old - 1;

            reg.setFlag('z', val == 0);
            reg.setFlag('n', 1);
            reg.setFlag('h', (old & 0x0F) == 0x00);

            // Cycle 3: Write back to memory
            bus->write(reg.hl, val, true);
        } else { // DEC r8
            uint8_t old = *r[y]();
            uint8_t val = old - 1;
            *r[y]() = val;

            reg.setFlag('z', val == 0);
            reg.setFlag('n', 1);
            reg.setFlag('h', (old & 0x0F) == 0x00);
        }
}

// Assorted accumulator/flag operations block
void CPU::rlca() { // RLCA: rotate A register left circular
    int b7 = (*reg.a & 0x80) >> 7;
    reg.setFlag('c', b7); // set C to bit 7
    *reg.a = *reg.a << 1; // rotate left 1 bit
    *reg.a |= b7; // set new b0 to former b7
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
}
void CPU::rrca() { // RRCA: rotate A circular right
    int b0 = *reg.a & 0x01;
    reg.setFlag('c', b0); // set C to bit 0
    *reg.a = *reg.a >> 1; // rotate right 1 bit
    *reg.a |= (b0 << 7); // set new b7 to former b0
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
}
void CPU::rla() { // RLA: rotate A register left (through carry)
    int b7 = (*reg.a & 0x80) >> 7;
    int oldC = (*reg.f & (1 << 4)) >> 4;
    *reg.a = (*reg.a << 1) + oldC;
    reg.setFlag('c', b7); // set C to bit 7
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
}
void CPU::rra() { // RRA: rotate A right (through carry)
    int b0 = *reg.a & 0x01;
    int c = (*reg.f & 0x10) >> 4;
    *reg.a = *reg.a >> 1; // rotate right 1 bit
    *reg.a |= (c << 7); // set new b7 to former C
    reg.setFlag('c', b0);
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
}
void CPU::daa() { // DAA: Decimal Adjust Accumulator
    if (reg.getFlag('n')) {
        int adj = 0;
        adj += reg.getFlag('h') ? 0x6 : 0;
        adj += reg.getFlag('c') ? 0x60 : 0;
        *reg.a -= adj;
    } else {
        int adj = 0;
        adj += (reg.getFlag('h') || (*reg.a & 0xF) > 0x9) ? 0x6 : 0;
        if (reg.getFlag('c') || *reg.a > 0x99) {
            adj += 0x60;
            reg.setFlag('c', 1);
        }
        *reg.a += adj;
    }
    reg.setFlag('h', 0);
    reg.setFlag('z', *reg.a == 0);
}
void CPU::cpl() { // CPL: bitwise NOT (ComPLement accumulator)
    *reg.a = ~(*reg.a);
    reg.setFlag('n', 1);
    reg.setFlag('h', 1);
}
void CPU::scf() { // SCF: set carry flag
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 1);
}
void CPU::ccf() { // CCF: Complement (flip) carry flag
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', !reg.getFlag('c'));
}

// alu ops
void CPU::add_a(int z, bool use_r) { // ADD A, r8/n8: add value from (r8/n8) to A and store in A
    uint8_t a = *reg.a;
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    uint16_t sum = a + r8;

    *reg.a = (uint8_t)sum;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((a & 0x0F) + (r8 & 0x0F)) > 0x0F);
    reg.setFlag('c', sum > 0xFF);
}
void CPU::adc_a(int z, bool use_r) { // ADC A, r8/n8: add value from carry and (r8/n8) to A and store in A
    uint16_t a = *reg.a;
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    uint16_t c = (reg.getFlag('c') ? 1 : 0);
    uint16_t sum = a + r8 + c;

    *reg.a = (uint8_t)sum;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((a & 0x0F) + (r8 & 0x0F) + c) > 0x0F);
    reg.setFlag('c', sum > 0xFF);
}
void CPU::sub_a(int z, bool use_r) { // SUB A, r8/n8: subtract (r8/n8) from A and store in A
    uint16_t a = *reg.a;
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    uint16_t diff = a - r8;

    *reg.a = (uint8_t)diff;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < (r8 & 0x0F));
    reg.setFlag('c', a < r8);
}
void CPU::sbc_a(int z, bool use_r) { // SBC A, r8/n8: subtract (r8/n8) and carry from A and store in A
    uint16_t a = *reg.a;
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    uint16_t c = reg.getFlag('c') ? 1 : 0;
    uint16_t diff = a - r8 - c;

    *reg.a = (uint8_t)diff;

    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < ((r8 & 0x0F) + c));
    reg.setFlag('c', a < (uint16_t)r8 + c);
}
void CPU::and_a(int z, bool use_r) { // AND A, r8/n8: bitwise and between A and (r8/n8) and store in A
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    *reg.a = *reg.a & r8;
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 1);
    reg.setFlag('c', 0);
}
void CPU::xor_a(int z, bool use_r) { // XOR A, r8/n8: bitwise xor between A and (r8/n8) and store in A
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    *reg.a = *reg.a ^ r8;
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 0);
}
void CPU::or_a(int z, bool use_r) { // OR A, r8/n8: bitwise or between A and (r8/n8) and store in A
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    *reg.a = *reg.a | r8;
    reg.setFlag('z', *reg.a == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 0);
}
void CPU::cp_a(int z, bool use_r) { // CP A, r8/n8: subtract (r8/n8) from A and update flags
    uint16_t a = *reg.a;
    uint8_t r8;
    if (z != 6)
        r8 = use_r ? *r[z]() : bus->read(pc++);
    else
        r8 = bus->read(reg.hl);
    uint16_t diff = a - r8;

    reg.setFlag('z', diff == 0);
    reg.setFlag('n', 1);
    reg.setFlag('h', (a & 0x0F) < (r8 & 0x0F));
    reg.setFlag('c', a < r8);
}

void CPU::ret_cc(std::function<bool()> cc) { // RET cc: conditional return
    gb->tick();

    if (cc()) {
        uint8_t lsb = bus->read(sp++);
        uint8_t msb = bus->read(sp++);
        pc = ((uint16_t)msb << 8) + (uint16_t)lsb;
        gb->tick();
    }
}

void CPU::add_sp_e() { // ADD SP, e8: add signed 8bit offset to stack pointer
    int8_t e8 = (int8_t)bus->read(pc++);
    uint16_t _sp = sp;
    uint16_t sum = _sp + e8;
    gb->tick();
    sp = sum;

    gb->tick();
    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((_sp & 0x0F) + ((uint8_t)e8 & 0x0F)) > 0x0F);
    reg.setFlag('c', ((_sp & 0xFF) + ((uint8_t)e8 & 0xFF)) > 0xFF);
}

void CPU::ld_hl_sp_e() { // LD HL, SP+e8: load to HL the value of e8 + sp
    int8_t e8 = (int8_t)bus->read(pc++);
    uint16_t sum = sp + e8;

    gb->tick();
    reg.hl = sum;

    reg.setFlag('z', 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', ((sp & 0x0F) + ((uint8_t)e8 & 0x0F)) > 0x0F);
    reg.setFlag('c', ((sp & 0xFF) + ((uint8_t)e8 & 0xFF)) > 0xFF);
}

void CPU::jp_cc_n16(std::function<bool()> cc) { // JP cc, n16: conditional jump to n16
    uint8_t lsb = bus->read(pc++);
    uint8_t msb = bus->read(pc++);
    uint16_t n16 = ((uint16_t)msb << 8) + lsb;
    if (cc()) {
        gb->tick();
        pc = n16;
    }
}

void CPU::call_cc_nn(std::function<bool()> cc) { // CALL cc, n16: conditional function call
    uint8_t lsb = bus->read(pc++);
    uint8_t msb = bus->read(pc++);
    uint16_t n16 = ((uint16_t)msb << 8) | lsb;
    if (cc()) {
        sp--;
        bus->write(sp--, (uint8_t)(pc >> 8));
        bus->write(sp, (uint8_t)pc);
        gb->tick();
        pc = n16;
    }
}

void CPU::call_nn() { // CALL nn: unconditional function call
    uint8_t lsb = bus->read(pc++);
    uint8_t msb = bus->read(pc++);
    uint16_t n16 = ((uint16_t)msb << 8) | lsb;
    sp--;
    bus->write(sp--, (uint8_t)(pc >> 8));
    bus->write(sp, (uint8_t)pc);
    gb->tick();
    pc = n16;
}

void CPU::rst_y(int y) { // RST y*8: unconditional function call to abs fixed addr
    uint8_t n = y*8;
    sp--;
    bus->write(sp--, (uint8_t)(pc >> 8));
    gb->tick();
    bus->write(sp, (uint8_t)pc);
    pc = 0x0000 | n;
}

void CPU::cb(uint8_t inst) { // $CB instruction prefix
    int x = inst >> 6;
    int y = (inst & 0x38) >> 3;
    int z = inst & 0x07;

    switch (x) {
        case 0:
            rot[y](z);
            break;
        case 1: // BIT y, r[z]
            bit(y, z);
            break;
        case 2:
            res(y, z);
            break;
        case 3:
            set(y, z);
            break;
    }
}

// $CB prefixed register roll/shifts or memory location tests
void CPU::rlc(int z) { // RLC r8: rotate left circular
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b7 = old >> 7;
    uint8_t newVal = (old << 1) | b7;
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b7);
}
void CPU::rrc(int z) { // RRC r8: rotate right circular
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b0 = old % 2;
    uint8_t newVal = (old >> 1) | (b0 << 7);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b0);
}
void CPU::rl(int z) { // RL r8: rotate left through carry
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b7 = old >> 7;
    uint8_t newVal = (old << 1) | reg.getFlag('c');
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b7);
}
void CPU::rr(int z) { // RR r8: rotate right through carry
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b0 = old % 2;
    uint8_t newVal = (old >> 1) | (reg.getFlag('c') << 7);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b0);
}
void CPU::sla(int z) { // SLA r8: shift left arithmetic
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b7 = old >> 7;
    uint8_t newVal = old << 1;
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b7);
}
void CPU::sra(int z) { // SRA r8: shift right arithmetic
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b0 = old & 0x01;
    uint8_t newVal = (old >> 1) | (old & 0x80);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b0);
}
void CPU::swap(int z) { // SWAP r8: swap high and low nibbles of r8
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    uint8_t lownib = old & 0x0F;
    uint8_t newVal = (old >> 4) | (lownib << 4);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', 0);
}
void CPU::srl(int z) { // SRL r8: shift right logical
    uint8_t old;
    if (z != 6)
        old = *r[z]();
    else
        old = bus->read(reg.hl);
    bool b0 = old % 2;
    uint8_t newVal = old >> 1;
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
    reg.setFlag('z', newVal == 0);
    reg.setFlag('n', 0);
    reg.setFlag('h', 0);
    reg.setFlag('c', b0);
}

// $CB prefixed bit operations
void CPU::bit(int y, int z) { // BIT y, r8: test bit y of r8
    uint8_t val;
    if (z != 6)
        val = *r[z]();
    else
        val = bus->read(reg.hl);
    bool set = (val & (1 << y)) >> y;

    reg.setFlag('z', !set);
    reg.setFlag('n', 0);
    reg.setFlag('h', 1);
}
void CPU::res(int y, int z) { // RES y, r8: reset bit y of r8 to zero
    uint8_t val;
    if (z != 6)
        val = *r[z]();
    else
        val = bus->read(reg.hl);
    uint8_t newVal = val & ~(1 << y);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
}
void CPU::set(int y, int z) { // SET y, r8: set bit y of r8 to 1
    uint8_t val;
    if (z != 6)
        val = *r[z]();
    else
        val = bus->read(reg.hl);
    uint8_t newVal = val | (1 << y);
    if (z != 6)
        *r[z]() = newVal;
    else
        bus->write(reg.hl, newVal);
}
