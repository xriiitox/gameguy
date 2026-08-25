#include "ppu.h"
#include <sys/types.h>
#include <algorithm>
#include <vector>
#include <map>
#include "gb.h"

PPU::PPU(Bus* bus, GameBoy* gub) {
    this->bus = bus;
    gb = gub;
    // ensure framebuffer empty
    std::fill(framebuffer, framebuffer + (160*144), 0u);
}

void PPU::tick() {
    // lcd disable (lcdc bit 7)
    if (!(bus->lcdc & 0x80)) {
        bus->stat = (bus->stat & 0xFC);
        bus->ly = 0;
        mode_cycles = 0;
        mode = 0;
        window_counter = 0;
        update_stat_line();
        return;
    }

    mode_cycles += 4;

    switch (mode) {
        case 2: // OAM read (80t/20m)
            if (mode_cycles >= 80) {
                mode = 3;
                lx = 0;
                scx_latch = bus->read(0xFF43, false);
                int mod8 = scx_latch % 8;
                int extra_dots = 0;
                if (mod8 >= 1 && mod8 <= 4) {
                    extra_dots = 4;
                } else if (mod8 >= 5 && mod8 <= 7) {
                    extra_dots = 8;
                }
                mode3_dur = 172 + extra_dots;
                update_stat_line();
                // Pre-fill or prepare background fetcher
            }
            break;
        case 3: { // pixel transfer
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
                if (window_render) {
                    window_counter++;
                    window_render = false;
                }

                bus->ly++;

                if (bus->ly == 144) {
                    mode = 1; // enter vblank
                    bus->IF |= 1; // req vblank intr
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
                    window_counter = 0;
                    mode = 2;
                }
                update_stat_line();
            }
            break;
    }
}

void PPU::render_single_dot() {
    uint8_t bg_color_idx = 0;
    if (bus->lcdc & 0x01) {
        uint8_t wy = bus->read(0xFF4A, false);
        uint8_t wx = bus->read(0xFF4B, false);

        bool is_window = (bus->lcdc & 0x20) && (bus->ly >= wy) && ((lx + 7) >= wx);

        uint8_t scan_x, scan_y;
        uint16_t tilemap;

        if (is_window) {
            window_render = true;
            scan_x = lx + 7 - wx;
            scan_y = window_counter;
            tilemap = (bus->lcdc & 0x40) ? 0x9C00 : 0x9800;
        } else {
            scan_x = lx + bus->read(0xFF43, false);
            scan_y = bus->ly + bus->read(0xFF42, false);
            tilemap = (bus->lcdc & 0x08) ? 0x9C00 : 0x9800;
        }

        uint8_t tile_x = scan_x / 8;
        uint8_t tile_y = scan_y / 8;
        uint8_t pixel_x = scan_x % 8;
        uint8_t pixel_y = scan_y % 8;

        uint8_t tile_id = bus->vram[tilemap - 0x8000 + (tile_y * 32) + tile_x];

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
        bg_color_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);
    }

    // apply palette
    uint8_t bgp = bus->read(0xFF47, false);
    uint8_t final_color = (bgp >> (bg_color_idx * 2)) & 0x03;

    if (bus->lcdc & 0x02) {
        uint8_t sprite_height = (bus->lcdc & 0x04) ? 16 : 8;

        std::vector<oam_obj> oam_data;
        for (int i = 0; i < 40; i++) {
            oam_obj thing;
            thing.y_pos = bus->oam[i*4] - 16;
            thing.x_pos = bus->oam[(i*4)+1] - 8;
            thing.tile_id = bus->oam[(i*4)+2];
            thing.attr = bus->oam[(i*4)+3];
            if (bus->ly >= thing.y_pos && bus->ly < (thing.y_pos + sprite_height))
                oam_data.push_back(thing);
            if (oam_data.size() == 10) break;
        }

        std::sort(oam_data.begin(), oam_data.end(), oam_comp);
        for (oam_obj data : oam_data) {
            if (lx >= data.x_pos && lx < (data.x_pos + 8)) {
                if (sprite_height == 16) data.tile_id &= 0xFE;
                bool flip_x = data.attr & 0x20;
                bool flip_y = data.attr & 0x40;
                bool priority = data.attr & 0x80;
                uint16_t palette_reg = (data.attr & 0x10) ? 0xFF49 : 0xFF48;

                uint8_t py = bus->ly - data.y_pos;
                if (flip_y) py = (sprite_height - 1) - py;

                uint8_t px = lx - data.x_pos;
                if (flip_x) px = 7 - px;

                uint16_t sprite_addr = 0x8000 + (data.tile_id * 16) + (py * 2);
                uint8_t byte1 = bus->vram[sprite_addr - 0x8000];
                uint8_t byte2 = bus->vram[sprite_addr - 0x8000+1];

                uint8_t bit = 7 - px;
                uint8_t spr_color_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

                if (spr_color_idx != 0) {
                    if (!priority || bg_color_idx == 0) {
                        uint8_t obp = bus->read(palette_reg, false);
                        final_color = (obp >> (spr_color_idx * 2)) & 0x03;
                    }
                    break;
                }
            }
        }
    }
    auto huh = palettes.find(gb->gbconf.palette);
    framebuffer[bus->ly*160 + lx] = huh->second[final_color];
}

bool oam_comp(const oam_obj &a, const oam_obj &b) {
    if (a.x_pos != b.x_pos) {
        return a.x_pos < b.x_pos;
    }
    return a.oam_ind < b.oam_ind;
}

void PPU::update_stat_line() {
    uint8_t lyc = bus->lyc;
    bus->stat = (bus->stat & 0xFC) | (mode & 0x03);

    bool lyc_match = (bus->ly == lyc);
    if (lyc_match) {
        bus->stat |= (1 << 2);
    } else {
        bus->stat &= ~(1 << 2);
    }

    bool mode0_int = (bus->stat & (1 << 3)) && (mode == 0);
    bool mode1_int = (bus->stat & (1 << 4)) && (mode == 1);
    bool mode2_int = (bus->stat & (1 << 5)) && (mode == 2);
    bool lyc_int   = (bus->stat & (1 << 6)) && lyc_match;

    bool current_stat_line = mode0_int || mode1_int || mode2_int || lyc_int;

    if (current_stat_line && !stat_line) {
        bus->IF |= (1 << 1);
    }

    stat_line = current_stat_line;
}
