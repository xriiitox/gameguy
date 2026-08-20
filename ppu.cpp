#include "ppu.h"
#include <sys/types.h>
#include <algorithm>

PPU::PPU(Bus* bus) {
    this->bus = bus;
    // ensure framebuffer empty
    std::fill(framebuffer, framebuffer + (160*144), 0u);
}

void PPU::tick() {
    mode_cycles += 4;

    switch (mode) {
        case 2: // OAM read (80t/20m)
            if (mode_cycles >= 80) {
                mode = 3;
                lx = 0;
                update_stat_line();
                // Pre-fill or prepare background fetcher
            }
            break;
        case 3: { // pixel transfer
            uint8_t scx = bus->read(0xFF43, false);
            int mode3_dur = 172 + (scx % 8);
            for (int dot = 0; dot < 4; dot++) {
                if (lx < 160) {
                    render_single_dot();
                    lx++;
                }
            }
            if (mode_cycles >= (80 + mode3_dur)) {
                mode = 0;
                update_stat_line();
            }
            break;
        }
        case 0: // hblank period
            if (mode_cycles >= 456) {
                mode_cycles -= 456;
                bus->ly++;

                if (bus->ly == 144) {
                    mode = 1; // enter vblank
                } else {
                    mode = 2;
                }
                update_stat_line();
            }
            break;
        case 1: // vblank
            if (mode_cycles >= 456) {
                mode_cycles -= 456;
                bus->ly++;
                if (bus->ly > 153) {
                    bus->ly = 0;
                    mode = 2;
                }
                update_stat_line();
            }
            break;
    }
}

void PPU::render_single_dot() {
    // tile coordinates
    uint8_t scan_x = lx + bus->read(0xFF43, false); // scx
    uint8_t scan_y = bus->ly + bus->read(0xFF42, false); // scy

    uint8_t tile_x = scan_x / 8;
    uint8_t tile_y = scan_y / 8;
    uint8_t pixel_x = scan_x % 8;
    uint8_t pixel_y = scan_y % 8;

    // background title id
    uint16_t tilemap = (bus->lcdc & 0x08) ? 0x9C00 : 0x9800;
    uint8_t tile_id = bus->vram[tilemap - 0x8000 + (tile_y * 32) + tile_x];

    // get low+high tile data bytes
    uint16_t tile_addr;
    if (bus->lcdc & 0x10) { // 0x8000 unsigned
        tile_addr = 0x8000 + (tile_id * 16) + (pixel_y * 2);
    } else {
        int8_t signed_id = (int8_t)tile_id;
        tile_addr = 0x9000 + (signed_id * 16) + (pixel_y * 2);
    }

    uint8_t byte1 = bus->vram[tile_addr - 0x8000];
    uint8_t byte2 = bus->vram[tile_addr - 0x8000 + 1];

    // decode color data per pixel
    uint8_t bit = 7 - pixel_x;
    uint8_t color_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

    // apply palette
    uint8_t bgp = bus->read(0xFF47, false);
    uint8_t final_color = (bgp >> (color_idx * 2)) & 0x03;
    int final_final_color;
    switch (final_color) {
        case 3: // darkest green
            final_final_color = 0xFF0F380F;
            break;
        case 2: // dark green
            final_final_color = 0xFF306230;
            break;
        case 1: // light green
            final_final_color = 0xFF8BAC0F;
            break;
        case 0: // lightest green
            final_final_color = 0xFF9BBC0F;
            break;
    }
    framebuffer[bus->ly*160 + lx] = final_final_color;

}

void PPU::update_stat_line() {
    uint8_t stat = bus->stat;
    uint8_t lyc  = bus->lyc;

    // 1. Update LYC==LY flag (Bit 2 of STAT)
    bool lyc_match = (bus->ly == lyc);
    if (lyc_match) {
        stat |= (1 << 2);
    } else {
        stat &= ~(1 << 2);
    }
    // Bus write/update STAT reg bit 2...

    // 2. Calculate the condition for each enable source
    bool mode0_int = (stat & (1 << 3)) && (mode == 0);
    bool mode1_int = (stat & (1 << 4)) && (mode == 1);
    bool mode2_int = (stat & (1 << 5)) && (mode == 2);
    bool lyc_int   = (stat & (1 << 6)) && lyc_match;

    // 3. Combined STAT signal (OR gate)
    bool current_stat_line = mode0_int || mode1_int || mode2_int || lyc_int;

    // 4. Request interrupt on RISING EDGE (0 -> 1 transition)
    if (current_stat_line && !stat_line) {
        bus->IF |= (1 << 1); // Request STAT interrupt
    }

    stat_line = current_stat_line;
}
