#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include "ritobin/bin.hpp"

namespace fs = std::filesystem;
int main(int argc, char** argv) {
    if (argc < 2) {
        puts("ritobin2txt <input bin file> <output txt file>");
        return -1;
    }
    fs::path infile = argv[1];
    fs::path outfile = argc > 2 ? argv[2] : "";
    if (argc < 3) {
        outfile = infile;
        outfile.replace_extension("bin");
    }

    try {
        ritobin::Bin bin = {};
        bin.read_text_file(infile.string());
        bin.write_binary_file(outfile.string());
        return 0;
    }  catch (std::runtime_error const& err) {
        fputs("Error: ", stderr);
        fputs(err.what(), stderr);
        if (argc < 3) {
            fputs("Press enter to continue...", stderr);
            int c = getc(stdin);
        }
        return -1;
    }
}
