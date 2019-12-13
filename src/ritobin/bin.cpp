#include "bin.hpp"
#include <fstream>
#include <stdexcept>

using namespace ritobin;

namespace {
    std::vector<char> read_file_data(std::string name) {
        std::ifstream file(name, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open input file: " + name);
        }
        std::vector<char> data;
        auto const start = file.tellg();
        file.seekg(static_cast<std::ifstream::off_type>(0), std::ifstream::end);
        auto const end = file.tellg();
        file.seekg(start);
        data.resize(static_cast<size_t>(end - start));
        file.read(data.data(), data.size());
        return data;
    }

    void write_file_data(std::string name, std::vector<char> const& data) {
        std::ofstream file(name, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open output file: " + name);
        }
        file.write(data.data(), data.size());
    }
}

void Bin::read_binary_file(std::string const& filename) {
    auto const data = read_file_data(filename);
    read_binary(data.data(), data.size());
}

void Bin::write_binary_file(std::string const& filename) const {
    std::vector<char> data = {};
    write_binary(data);
    write_file_data(filename, data);
}

void Bin::read_text_file(std::string const& filename) {
    auto const data = read_file_data(filename);
    read_text(data.data(), data.size());
}

void Bin::write_text_file(std::string const& filename, size_t ident_size) const {
    std::vector<char> data = {};
    write_text(data, ident_size);
    write_file_data(filename, data);
}