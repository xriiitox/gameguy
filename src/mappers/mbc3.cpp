#include "mbc3.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <chrono>

MBC3::MBC3(std::string filename, bool timer, bool ram, bool battery) {
    std::filesystem::path p{filename};
    auto length = std::filesystem::file_size(p);
    if (length == 0) return;
    std::vector<std::byte> buffer(length);
    std::ifstream in(filename, std::ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

    Mapper::rom_size = length;
    Mapper::ram = ram;
    Mapper::battery = battery;
    Mapper::bank_count = length / 0x4000;
    Mapper::rtc = timer;

    Mapper::ram_bank_count = 0;
    switch ((uint8_t)buffer[0x0149]) {
        case 0x0:
            Mapper::ram = false;
            Mapper::ram_bank_count = 0;
            break;
        case 0x2:
            Mapper::ram_bank_count = 1;
            break;
        case 0x3:
            Mapper::ram_bank_count = 4;
            break;
            /*
        case 0x4:
            Mapper::ram_bank_count = 16;
            break;
            */
        case 0x5:
            Mapper::ram_bank_count = 8;
            break;
    }

    if (Mapper::ram) {
        ram_banks.resize(Mapper::ram_bank_count * 0x2000);
    }

    int rtc_amt = Mapper::rtc ? 48 : 0;

    std::string ext = p.extension().string();
    int extnum;
    if (ext == "gb") {
        extnum = 3;
    } else {
        extnum = 4; // gbc filename
    }
    std::string savename = filename.substr(0, filename.size()-extnum)+".sav";
    if (std::filesystem::exists(savename) && Mapper::battery && Mapper::ram) {
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
        std::filesystem::path savepath{savename};
        auto savelength = std::filesystem::file_size(savepath);
        std::ifstream save(savename, std::ios_base::binary);
        std::vector<std::byte> rambuf(savelength);
        save.read(reinterpret_cast<char*>(rambuf.data()), savelength-rtc_amt);
        ram_banks.resize(savelength);
        std::memcpy(ram_banks.data(), rambuf.data(), savelength-rtc_amt);
        if (Mapper::rtc) {
            std::memcpy(rtc_regs.data(), rambuf.data()+savelength, 20);
            std::memcpy(latched_regs.data(), rambuf.data()+savelength+20, 20);
            long now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            uint64_t old_seconds = ((long)rambuf[savelength-1] << 24) +
                ((long)rambuf[savelength-2] << 16) +
                ((long)rambuf[savelength-3] << 8) +
                (long)rambuf[savelength-4];
            long seconds_diff = now_seconds - old_seconds;
            for (long i = 0; i < seconds_diff; i++) {
                tick_rtc(); // update rtc regs to time diff
            }
        }
        save.close();


    } else if (Mapper::battery && Mapper::ram) {
        // create file
        std::ofstream file;
        file.open(savename, std::ios::binary | std::ios::trunc);
        std::vector<char> emptyRam(ram_bank_count*0x2000+rtc_amt, 0xFF);
        file.write(emptyRam.data(), ram_bank_count*0x2000+rtc_amt);
        file.close();
        mmap = mio::make_mmap_sink(savename, 0, mio::map_entire_file, error);
    }

    // load rom
    rom_banks.resize(length);
    std::memcpy(rom_banks.data(), buffer.data(), length);
}

MBC3::~MBC3() {
    if (Mapper::battery && Mapper::ram) {
        for (int i = 0; i < ram_banks.size(); i++) {
            mmap[i] = ram_banks[i];
        }
        if (Mapper::rtc) {
            for (int i = 0; i < 5; i++) {
                mmap[ram_banks.size()+i] = rtc_regs[i];
            }
            for (int i = 0; i < 5; i++) {
                mmap[ram_banks.size()+5+i] = latched_regs[i];
            }
            long now_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            for (int i = 0; i < 8; i++) {
                // write bytes of current time to save file and reload later
                mmap[ram_banks.size()+10+i] = (now_seconds & (0xFF << i)) >> i;
            }
        }
        mmap.sync(error);
    }
}

void MBC3::tick_rtc() {
    rtc_regs[0]++;
    if (rtc_regs[0] == 60) { // second overflow
        rtc_regs[1]++;
        rtc_regs[0] = 0;
    }
    if (rtc_regs[1] == 60) { // minute overflow
        rtc_regs[2]++;
        rtc_regs[1] = 0;
    }
    if (rtc_regs[2] == 24) { // hour overflow
        rtc_regs[3]++;
        rtc_regs[2] = 0;
        if (rtc_regs[3] == 0) { // lower byte of day counter overflow
            if ((rtc_regs[4] & 1) != 0) { // day counter msb overflow
                rtc_regs[4] &= ~1;
                rtc_regs[4] |= (1 << 7);
            } else rtc_regs[4] |= 1;
        }
    }
}

uint8_t MBC3::read(uint16_t addr) {
    if (addr >= 0 && addr <= 0x3FFF) {
        return rom_banks[addr];
    }
    if (addr >= 0x4000 && addr <= 0x7FFF) {
        if (rom_select == 0) rom_select = 1;
        return rom_helper(rom_select & 0x7F, addr - 0x4000);
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_timer_en == 0x0A) {
            if (ram_rtc_sel >= 0 && ram_rtc_sel <= 7) {
                return ram_banks[(ram_rtc_sel % Mapper::ram_bank_count)*0x2000 + addr - 0xA000];
            } else if (ram_rtc_sel >= 8 && ram_rtc_sel <= 0x0C) {
                return latched_regs[ram_rtc_sel - 8];
            }
            return 0xFF;
        }
    }
    return 0xFF;
}

uint8_t MBC3::rom_helper(size_t bank, uint16_t offset) {
    size_t real_addr = (bank * 0x4000) + offset;
    return rom_banks[real_addr % Mapper::rom_size];
}

void MBC3::write(uint16_t addr, uint8_t val) {
    if (addr >= 0 && addr <= 0x1FFF) {
        ram_timer_en = val;
        return;
    }
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        rom_select = val & 0x7F;
        return;
    }
    if (addr >= 0x4000 && addr <= 0x5FFF) {
        ram_rtc_sel = val;
        return;
    }
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (latch == 0 && val == 1) {
            for (int i = 0; i < rtc_regs.size(); i++) {
                latched_regs[i] = rtc_regs[i];
            }
        }
        latch = val;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_timer_en == 0x0A) {
            if (ram_rtc_sel >= 0 && ram_rtc_sel <= 7) {
                ram_banks[(ram_rtc_sel % Mapper::ram_bank_count)*0x2000 + addr - 0xA000] = val;
                return;
            }
            if (ram_rtc_sel >= 8 && ram_rtc_sel <= 0xC) {
                rtc_regs[ram_rtc_sel - 8] = val;
                return;
            }
        }
    }
}
