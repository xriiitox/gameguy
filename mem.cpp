#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std;

void Load_Rom(string filename, uint8_t mem[]) {
    filesystem::path p{filename};
    auto length = filesystem::file_size(p);
    if (length == 0) return;
    vector<byte> buffer(length);
    ifstream in(filename, ios_base::binary);
    in.read(reinterpret_cast<char*>(buffer.data()), length);
    in.close();

    int fakepc = 0x0;
    for (auto byte : buffer) {
        mem[fakepc++] = static_cast<uint8_t>(byte);

    }
}
