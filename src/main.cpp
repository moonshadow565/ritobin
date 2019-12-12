#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <array>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <variant>
#include <iostream>
#include "ritobin/bin.hpp"


std::vector<char> ReadData(std::string name) noexcept {
    std::vector<char> data;
    std::ifstream file(name, std::ios::binary);
    auto const start = file.tellg();
    file.seekg(static_cast<std::ifstream::off_type>(0), std::ifstream::end);
    auto const end = file.tellg();
    file.seekg(start);
    data.resize(static_cast<size_t>(end - start));
    file.read(data.data(), data.size());
    return data;
}

void test_read_binary() noexcept {
    auto data = ReadData("skin8.bin");
    std::string error = {};
    ritobin::NodeList nodes = {};
    ritobin::binary_read({ data.data(), data.size() }, nodes, error);


    ritobin::LookupTable lookup = {};
    ritobin::lookup_load_CDTB("hashes.binentries.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.binhashes.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.bintypes.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.binfields.txt", lookup);
    ritobin::lookup_unhash(lookup, nodes);

    std::vector<char> result = {};
    ritobin::text_write(nodes, result, error);

    std::cout.write(result.data(), result.size());
}

void test_read_text() noexcept {
    auto data = ReadData("skin8-org.py");

    std::string error = {};
    ritobin::NodeList nodes = {};
    ritobin::text_read({ data.data(), data.size() }, nodes, error);

    std::vector<char> result = {};
    ritobin::binary_write(nodes, result, error);

    std::ofstream outfile("skin8-new.bin", std::ios::binary);
    outfile.write(result.data(), result.size());
}

int main() {
    // test_read_text();
    test_read_text();

    return 0;
}
