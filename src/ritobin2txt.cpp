#include <iostream>
#include <fstream>
#include <cstdio>
#include "ritobin/bin.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        puts("ritobin2txt <input bin file> <output txt file>");
        return -1;
    }

    std::vector<char> indata;
    if (std::ifstream file(argv[1], std::ios::binary); file) {
        auto const start = file.tellg();
        file.seekg(static_cast<std::ifstream::off_type>(0), std::ifstream::end);
        auto const end = file.tellg();
        file.seekg(start);
        indata.resize(static_cast<size_t>(end - start));
        file.read(indata.data(), indata.size());
    } else {
        puts("Input file not found!");
        return -1;
    }

    std::string error = {};
    ritobin::NodeList nodes = {};
    if(!ritobin::binary_read({ indata.data(), indata.size() }, nodes, error)) {
        puts("Failed to read:");
        puts(error.c_str());
        return -1;
    }

    ritobin::LookupTable lookup = {};
    ritobin::lookup_load_CDTB("hashes.binentries.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.binhashes.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.bintypes.txt", lookup);
    ritobin::lookup_load_CDTB("hashes.binfields.txt", lookup);
    ritobin::lookup_unhash(lookup, nodes);

    std::vector<char> outdata = {};
    if (!ritobin::text_write(nodes, outdata, error)) {
        puts("Failed to write:");
        puts(error.c_str());
        return -1;
    }

    /*
    if (std::string name(argv[2]); name == "-") {
        fwrite(outdata.data(), 1, outdata.size(), stdout);
        return 0;
    }
    */    
    if (std::ofstream file(argv[2], std::ios::binary); file) {
        file.write(outdata.data(), outdata.size());
    } else {
        puts("Output file not found!");
        return -1;
    }
}