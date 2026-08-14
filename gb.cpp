#include "gb.h"
#include "mem.h"

GameBoy::GameBoy(GBConfig gbconf) {
    Load_Rom(gbconf.filename, bus);
}

void GameBoy::cycle() {

}
