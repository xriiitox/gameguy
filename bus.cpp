#include "bus.h"
#include "gb.h"
#include "mem.h"
#include <algorithm>
#include <numeric>

Bus::Bus(void* gb) {
    this->gb = gb;

    div = (uint8_t*)&((GameBoy*)gb)->sysclk + 1;

    for (auto bank : bankx) {
        bank.fill(0);
    }
}

bool is_external_bus(uint16_t addr) {
    return (addr <= 0x7FFF) ||
           (addr >= 0xA000 && addr <= 0xFDFF);
}

bool is_vram_bus(uint16_t addr) {
    return (addr >= 0x8000 && addr <= 0x9FFF);
}

void Bus::write(uint16_t addr, uint8_t val, bool tk) {
    if (tk) ((GameBoy*)this->gb)->tick();
    if (((GameBoy*)this->gb)->dma.bus_locked) {
            uint16_t dma_src = ((GameBoy*)this->gb)->dma.source;

            if (addr >= 0xFE00 && addr <= 0xFE9F) return; // OAM locked
            if (is_external_bus(dma_src) && is_external_bus(addr)) return;
            if (is_vram_bus(dma_src) && is_vram_bus(addr)) return;
        }
    if (addr <= 0x1FFF) { ram_en = val; return; } // MBC RAM enable
    if (addr >= 0x2000 && addr <= 0x3FFF) { switch_rom_bank(val, this); return; }
    if (addr <= 0x7FFF) return; // ROM is obviously read only
    if (addr <= 0x9FFF) { write_vram(addr, val); return; } // vram
    if (addr <= 0xBFFF) { eram[addr - 0xA000] = val; return; } // external ram
    if (addr <= 0xCFFF) { wram1[addr - 0xC000] = val; return; }
    if (addr <= 0xDFFF) { wram2[addr - 0xD000] = val; return; }
    if (addr <= 0xEFFF) { wram1[addr - 0xE000] = val; return; } // echo ram section 1
    if (addr <= 0xFDFF) { wram2[addr - 0xF000] = val; return; } // echo ram section 2
    if (addr <= 0xFE9F) { oam[addr - 0xFE00] = val; return; } // oam
    // controller
    if (addr == 0xFF00) { joyp = val & 0xF0; return; }
    // serial
    if (addr == 0xFF01) { serial_port.sb = val; return; }
    if (addr == 0xFF02) {
        if (val != 0x81) {
            write_sc(val);
            return;
        }
        std::cout << (char)read(0xFF01, false) << std::flush;
        serial_port.sb = 0xFF;
        serial_port.sc = val & ~0x80;
        IF |= (1 << 3);
    }
    if (addr == 0xFF04) { write_div(); return; }
    if (addr == 0xFF05) { write_tima(val); return; }
    if (addr == 0xFF06) { write_tma(val); return; }
    if (addr == 0xFF07) { write_tac(val); return; }
    if (addr == 0xFF0F) { IF = val; return; }
    if (addr == 0xFF10) { nr10 = val; return; }
    if (addr == 0xFF11) { nr11 = val; return; }
    if (addr == 0xFF12) { nr12 = val; return; }
    if (addr == 0xFF13) { nr13 = val; return; }
    if (addr == 0xFF14) { nr14 = val; return; }
    if (addr == 0xFF16) { nr21 = val; return; }
    if (addr == 0xFF17) { nr22 = val; return; }
    if (addr == 0xFF18) { nr23 = val; return; }
    if (addr == 0xFF19) { nr24 = val; return; }
    if (addr == 0xFF1A) { nr30 = val; return; }
    if (addr == 0xFF1B) { nr31 = val; return; }
    if (addr == 0xFF1C) { nr32 = val; return; }
    if (addr == 0xFF1D) { nr33 = val; return; }
    if (addr == 0xFF1E) { nr34 = val; return; }
    if (addr == 0xFF20) { nr41 = val; return; }
    if (addr == 0xFF21) { nr42 = val; return; }
    if (addr == 0xFF22) { nr43 = val; return; }
    if (addr == 0xFF23) { nr44 = val; return; }
    if (addr == 0xFF24) { nr50 = val; return; }
    if (addr == 0xFF25) { nr51 = val; return; }
    if (addr == 0xFF26) { nr52 = val & 0xF0; return; }
    if (addr <= 0xFF3F) { wav_ram[addr - 0xFF30] = val; return; }
    if (addr == 0xFF40) { lcdc = val; return; }
    if (addr == 0xFF41) { // leave last few bits read only
        stat = (stat & 0x07) | (val & 0x78);
        ((GameBoy*)gb)->ppu_stat_line();
        return;
    }
    if (addr == 0xFF42) { scy = val; return; }
    if (addr == 0xFF43) { scx = val; return; }
    if (addr == 0xFF44) { ly = 0; return; } // reset scanline
    if (addr == 0xFF45) {
        lyc = val;
        ((GameBoy*)gb)->ppu_stat_line();
        return;
    }
    if (addr == 0xFF46) { write_dma(val); return; }
    if (addr == 0xFF47) { bgp = val; return; }
    if (addr == 0xFF48) { obp0 = val; return; }
    if (addr == 0xFF49) { obp1 = val; return; }
    if (addr == 0xFF4A) { wy = val; return; }
    if (addr == 0xFF4B) { wx = val; return;}
    if (addr <= 0xFFFE) { hram[addr - 0xFF80] = val; return; } // high ram
    if (addr == 0xFFFF) { ie = val; return; }
}

uint8_t Bus::read(uint16_t addr, bool tk, bool bypass) {
    bool isdma = ((GameBoy*)this->gb)->dma.bus_locked;
    if (tk) ((GameBoy*)gb)->tick();
    // if (tk && (!isdma || (addr >= 0xFF80 && addr <= 0xFFFE))) ((GameBoy*)this->gb)->tick(); // tick if read from hram during dma
    // only allow reads from hram or source addr during dma
    if (!bypass && isdma) {
            uint16_t dma_src = ((GameBoy*)this->gb)->dma.source;

            // OAM is always locked for CPU reads during DMA
            if (addr >= 0xFE00 && addr <= 0xFE9F) {
                return 0xFF;
            }

            // Check for conflict on External Bus
            if (is_external_bus(dma_src) && is_external_bus(addr)) {
                return 0xFF;
            }

            // Check for conflict on VRAM Bus
            if (is_vram_bus(dma_src) && is_vram_bus(addr)) {
                return 0xFF;
            }
        }
    if (addr <= 0x3FFF) return bank0[addr];
    if (addr <= 0x7FFF) return bankx[sel_bank][addr - 0x4000];
    if (addr <= 0x9FFF) return read_vram(addr);
    if (addr <= 0xBFFF) return eram[addr - 0xA000];
    if (addr <= 0xCFFF) return wram1[addr - 0xC000];
    if (addr <= 0xDFFF) return wram2[addr - 0xD000];
    if (addr <= 0xEFFF) return wram1[addr - 0xE000];
    if (addr <= 0xFDFF) return wram2[addr - 0xF000];
    if (addr <= 0xFE9F) return oam[addr - 0xFE00];
    // read io regs
    if (addr == 0xFF00) { return joyp | 0xC0; }
    if (addr == 0xFF01) return serial_port.sb;
    if (addr == 0xFF02) { return serial_port.sc | 0x7E; }
    if (addr == 0xFF04) return *div; // pointer to upper 8 bytes of sysclk
    if (addr == 0xFF05) return timers.tima;
    if (addr == 0xFF06) return timers.tma;
    if (addr == 0xFF07) { return timers.tac | 0xF8; }
    if (addr == 0xFF0F) { return IF | 0xE0; }
    if (addr == 0xFF10) { return nr10 | 0x80; }
    if (addr == 0xFF11) { return nr11; }
    if (addr == 0xFF12) { return nr12; }
    if (addr == 0xFF13) { return nr13; }
    if (addr == 0xFF14) { return nr14; }
    if (addr == 0xFF16) { return nr21; }
    if (addr == 0xFF17) { return nr22; }
    if (addr == 0xFF18) { return nr23; }
    if (addr == 0xFF19) { return nr24; }
    if (addr == 0xFF1A) { return nr30 | 0x7F; }
    if (addr == 0xFF1B) { return nr31; }
    if (addr == 0xFF1C) { return nr32 | 0x9F; }
    if (addr == 0xFF1D) { return nr33; }
    if (addr == 0xFF1E) { return nr34; }
    if (addr == 0xFF20) { return nr41 | 0xC0; }
    if (addr == 0xFF21) { return nr42; }
    if (addr == 0xFF22) { return nr43; }
    if (addr == 0xFF23) { return nr44 | 0x3F; }
    if (addr == 0xFF24) { return nr50; }
    if (addr == 0xFF25) { return nr51; }
    if (addr == 0xFF26) { return nr52 | 0x70; }
    if (addr == 0xFF40) return lcdc;
    if (addr == 0xFF41) { return stat | 0x80; }
    if (addr == 0xFF42) return scy;
    if (addr == 0xFF43) return scx;
    if (addr == 0xFF44) return ly;
    if (addr == 0xFF45) return lyc;
    if (addr == 0xFF46) return dma;
    if (addr == 0xFF47) return bgp;
    if (addr == 0xFF48) return obp0;
    if (addr == 0xFF49) return obp1;
    if (addr == 0xFF4A) return wy;
    if (addr == 0xFF4B) return wx;
    if (addr>= 0xFF80 && addr <= 0xFFFE) return hram[addr - 0xFF80];
    if (addr == 0xFFFF) return ie;

    if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return 0xFF;
    }

    return 0xFF;
}

void Bus::write_div() {
    bool old_bit = get_timer_bit();

    ((GameBoy*)gb)->sysclk = 0;

    bool new_bit = get_timer_bit_at(0, timers.tac);

    if (old_bit && !new_bit) {
        timers.tima++;
        if (timers.tima == 0) {
            timers.reload_delay = 4;
        }
    }

    // reset apu frame sequencer
}

void Bus::write_tima(uint8_t val) {
    if (timers.reload_delay > 0) {
        timers.reload_delay = 0;
        timers.tima = val;
    } else if (!timers.tima_just_reloaded) {
        timers.tima = val;
    }
}

void Bus::write_tma(uint8_t val) {
    timers.tma = val;
    if (timers.reload_delay > 0 || timers.tima_just_reloaded) {
        timers.tima = val;
    }
}

void Bus::write_dma(uint8_t val) {
    GameBoy* gub = (GameBoy*)gb;
    uint16_t src = val << 8;
    if (src >= 0xE000) {
        src -= 0x2000; // Map 0xE000-0xFFFF down to 0xC000-0xDF00
    }
    gub->dma.source = src;
    gub->dma.index = 0;
    gub->dma.start_delay = 1;
    gub->dma.active = true;
    gub->dma.bus_locked = false;
    dma = val;
}

void Bus::write_sc(uint8_t val) {
    serial_port.sc = val | 0x7E;

    serial_port.int_clock = (val & 0x01) != 0;

    if (val & 0x80) {
        serial_port.transfer_active = true;
        serial_port.bit_count = 0;
    }
}

void Bus::tick_serial() {
    if (!serial_port.transfer_active || !serial_port.int_clock) {
        return;
    }

    bool old_bit = ((((GameBoy*)gb)->sysclk-4) & (1 << 8)) != 0;
    bool new_bit = ((((GameBoy*)gb)->sysclk) & (1 << 8)) != 0;

    if (old_bit && !new_bit) {
        uint8_t incoming = 1;
        serial_port.sb = (serial_port.sb << 1) | incoming;

        serial_port.bit_count++;

        if (serial_port.bit_count == 8) {
            serial_port.transfer_active = false;
            serial_port.sc &= ~0x80;
            serial_port.bit_count = 0;

            IF |= (1 << 3); // serial interrupt
        }
    }
}

uint8_t Bus::read_vram(uint16_t addr) {
    if ((lcdc & 0x80) && *((GameBoy*)gb)->ppuMode == 3) {
        return 0xFF;
    }
    return vram[addr - 0x8000];
}

void Bus::write_vram(uint16_t addr, uint8_t val) {
    if ((lcdc & 0x80) && *((GameBoy*)gb)->ppuMode == 3) {
        return;
    }
    vram[addr - 0x8000] = val;
}

void Bus::write_tac(uint8_t val) {
    bool old_bit = get_timer_bit();
    bool new_bit = get_timer_bit_at(((GameBoy*)gb)->sysclk, val);

    if (old_bit && !new_bit) {
        timers.tima++;
        if (timers.tima == 0) {
            timers.reload_delay = 4;
        }
    }
    timers.tac = val;
}

bool Bus::get_timer_bit() {
    uint16_t sysclk = ((GameBoy*)gb)->sysclk;
    if (!(timers.tac & 0x04)) return false;

    switch (timers.tac & 0x03) {
        case 0: return (sysclk & (1 << 9)) != 0;
        case 1: return (sysclk & (1 << 3)) != 0;
        case 2: return (sysclk & (1 << 5)) != 0;
        case 3: return (sysclk & (1 << 7)) != 0;
    }
    return false;
}

bool Bus::get_timer_bit_at(uint16_t clk, uint8_t tac_val) {
    if (!(tac_val & 0x04)) return false;
    static const uint8_t bit_shifts[4] = {9, 3, 5, 7};
    return (clk & (1 << bit_shifts[tac_val & 0x03])) != 0;
}
