#include "bus.h"
#include "gb.h"
#include <algorithm>

Bus::Bus(GameBoy* gb) {
    this->gb = gb;

    div = (uint8_t*)&gb->sysclk + 1;

    std::fill(vram, vram+sizeof(vram), 0);

    wram1[0] = 0x48;
}

void Bus::initialize_dmg_vram() {
    std::fill(std::begin(vram), std::end(vram), 0);

    uint16_t destination = 0x10;
    for (uint16_t source = 0x0104; source < 0x0134; ++source) {
        const uint8_t logo_byte = gb->mapper->read(source);
        for (int shift : {4, 0}) {
            const uint8_t nibble = (logo_byte >> shift) & 0x0F;
            uint8_t expanded = 0;
            for (int bit = 3; bit >= 0; --bit) {
                expanded |= static_cast<uint8_t>(((nibble >> bit) & 1) << (bit * 2 + 1));
                expanded |= static_cast<uint8_t>(((nibble >> bit) & 1) << (bit * 2));
            }
            vram[destination++] = expanded;
            ++destination;
            vram[destination++] = expanded;
            ++destination;
        }
    }

    constexpr uint8_t trademark[] = {
        0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C
    };
    for (uint8_t value : trademark) {
        vram[destination] = value;
        destination += 2;
    }

    constexpr uint8_t background_map[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x19, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x18
    };
    std::copy(std::begin(background_map), std::end(background_map),
              vram + (0x9904 - 0x8000));
}

bool is_external_bus(uint16_t addr) {
    return (addr <= 0x7FFF) ||
           (addr >= 0xA000 && addr <= 0xFDFF);
}

bool is_vram_bus(uint16_t addr) {
    return (addr >= 0x8000 && addr <= 0x9FFF);
}

void Bus::write(uint16_t addr, uint8_t val, bool tk) {
    if (tk) gb->tick();
    if (gb->dma.bus_locked) {
            if (addr == 0xFF46) { write_dma(val); return; }
            if (addr >= 0xFF30 && addr <= 0xFF3F) {
                gb->apu->channel3.wave_ram[addr - 0xFF30] = val;
                return;
            }
            if (addr < 0xFF80 || addr > 0xFFFE) return;
        }
    if (addr <= 0x7FFF) { gb->mapper->write(addr, val); return; }
    if (addr <= 0x9FFF) { write_vram(addr, val); return; } // vram
    if (addr <= 0xBFFF) { gb->mapper->write(addr, val); return; } // external ram
    if (addr <= 0xCFFF) { wram1[addr - 0xC000] = val; return; }
    if (addr <= 0xDFFF) { wram2[addr - 0xD000] = val; return; }
    if (addr <= 0xEFFF) { wram1[addr - 0xE000] = val; return; } // echo ram section 1
    if (addr <= 0xFDFF) { wram2[addr - 0xF000] = val; return; } // echo ram section 2
    if (addr <= 0xFE9F) { oam[addr - 0xFE00] = val; return; } // oam
    // controller
    if (addr == 0xFF00) { joyp = (joyp & 0x0F) | (val & 0xF0); return; }
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
    if (addr >= 0xFF10 && addr <= 0xFF26) {
        gb->apu->write(addr, val);
        return;
    }
    if (addr >= 0xFF30 && addr <= 0xFF3F) { gb->apu->channel3.wave_ram[addr - 0xFF30] = val; return; }
    if (addr == 0xFF40) { lcdc = val; return; }
    if (addr == 0xFF41) { // leave last few bits read only
        stat = (stat & 0x07) | (val & 0xF8);
        gb->ppu_stat_line();
        return;
    }
    if (addr == 0xFF42) { scy = val; return; }
    if (addr == 0xFF43) { scx = val; return; }
    if (addr == 0xFF44) { ly = 0; gb->ppu_stat_line(); return; } // reset scanline
    if (addr == 0xFF45) {
        lyc = val;
        gb->ppu_stat_line();
        return;
    }
    if (addr == 0xFF46) { write_dma(val); return; }
    if (addr == 0xFF47) { bgp = val; return; }
    if (addr == 0xFF48) { obp0 = val; return; }
    if (addr == 0xFF49) { obp1 = val; return; }
    if (addr == 0xFF4A) { wy = val; return; }
    if (addr == 0xFF4B) { wx = val; return;}
    if (addr >= 0xFF80 && addr <= 0xFFFE) {
        hram[addr - 0xFF80] = val;
        return;
    } // high ram
    if (addr == 0xFFFF) { ie = val; return; }
}

uint8_t Bus::read(uint16_t addr, bool tk, bool bypass) {
    bool isdma = gb->dma.bus_locked;
    if (tk) gb->tick();
    // if (tk && (!isdma || (addr >= 0xFF80 && addr <= 0xFFFE))) gb->tick(); // tick if read from hram during dma
    // only allow reads from hram or source addr during dma
    if (!bypass && isdma) {
        if (addr >= 0xFF30 && addr <= 0xFF3F) {
            return gb->apu->channel3.wave_ram[addr - 0xFF30];
        }
        if (addr < 0xFF80 || addr > 0xFFFE) {
            return gb->dma.index == 0 ? 0xFF : oam[gb->dma.index - 1];
        }
    }
    if (addr <= 0x7FFF) return gb->mapper->read(addr);
    if (addr <= 0x9FFF) return read_vram(addr);
    if (addr <= 0xBFFF) return gb->mapper->read(addr);
    if (addr <= 0xCFFF) return wram1[addr - 0xC000];
    if (addr <= 0xDFFF) return wram2[addr - 0xD000];
    if (addr <= 0xEFFF) return wram1[addr - 0xE000];
    if (addr <= 0xFDFF) return wram2[addr - 0xF000];
    if (addr <= 0xFE9F) return oam[addr - 0xFE00];
    // read io regs
    if (addr == 0xFF00) {
        bool d_pad = !((joyp & 0x10) >> 4);
        bool button = !((joyp & 0x20) >> 5);

        uint8_t res = 0xC0 | joyp | 0x0F;

        if (d_pad) {
            res &= (0xF0 | gb->d_pad_state);
        }
        if (button) {
            res &= (0xF0 | gb->buttons_state);
        }

        return res;
    }
    if (addr == 0xFF01) return serial_port.sb;
    if (addr == 0xFF02) { return serial_port.sc | 0x7E; }
    if (addr == 0xFF04) return *div; // pointer to upper 8 bytes of sysclk
    if (addr == 0xFF05) return timers.tima;
    if (addr == 0xFF06) return timers.tma;
    if (addr == 0xFF07) { return timers.tac | 0xF8; }
    if (addr == 0xFF0F) { return IF | 0xE0; }
    if (addr >= 0xFF10 && addr <= 0xFF26) {
        return gb->apu->read(addr);
    }
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        return gb->apu->channel3.wave_ram[addr - 0xFF30];
    }
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

    gb->sysclk = 0;

    bool new_bit = get_timer_bit_at(0, timers.tac);

    if (old_bit && !new_bit) {
        timers.tima++;
        if (timers.tima == 0) {
            timers.tima = timers.tma;
            IF |= (1 << 2);
            timers.tima_just_reloaded = true;
        }
    }

    gb->apu->frame_sequencer_counter = 0;
    gb->apu->frame_sequencer_step = 7;
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
    gub->dma.pending_source = val << 8;
    if (gub->dma.pending_source >= 0xE000) {
        gub->dma.pending_source -= 0x2000; // Map 0xE000-0xFFFF down to 0xC000-0xDF00
    }
    if (gub->dma.active) {
        gub->dma.pending_restart = true;
        gub->dma.start_delay = 1;
    } else {
        gub->dma.source = gub->dma.pending_source;
        gub->dma.index = 0;
        gub->dma.start_delay = 1;
        gub->dma.active = true;
        dma = val;
    }
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

    bool old_bit = ((gb->sysclk-4) & (1 << 8)) != 0;
    bool new_bit = ((gb->sysclk) & (1 << 8)) != 0;

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
    if ((lcdc & 0x80) && *gb->ppuMode == 3) {
        return 0xFF;
    }
    return vram[addr - 0x8000];
}

void Bus::write_vram(uint16_t addr, uint8_t val) {
    if ((lcdc & 0x80) && *gb->ppuMode == 3) {
        return;
    }
    vram[addr - 0x8000] = val;
}

void Bus::write_tac(uint8_t val) {
    bool old_bit = get_timer_bit();
    timers.tac = val;
    bool new_bit = get_timer_bit();

    if (old_bit && !new_bit) {
        timers.tima++;
        if (timers.tima == 0) {
            timers.tima = timers.tma;
            IF |= (1 << 2);
            timers.tima_just_reloaded = true;
        }
    }
}

bool Bus::get_timer_bit() {
    uint16_t sysclk = gb->sysclk;
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
