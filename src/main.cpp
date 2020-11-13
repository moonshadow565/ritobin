#include <cstdlib>
#include "argparse.hpp"
#include "ritobin/bin_io.hpp"
#include "ritobin/bin_unhash.hpp"

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
static void set_binary_mode(FILE* file) {
    if (_setmode(_fileno(file), O_BINARY) == -1) {
        throw std::runtime_error("Can not change mode to binary!");
    }
}
#else
static void set_binary_mode(FILE*) {}
#endif

using ritobin::Bin;
using ritobin::BinUnhasher;
using ritobin::io::DynamicFormat;

static DynamicFormat const* parse_format(std::string const& name) {
    auto format = DynamicFormat::get(name);
    if (!format) {
        throw std::runtime_error("Format not found: " + name);
    }
    return format;
}

template<char M>
static FILE* parse_file(std::string const& name) {
    char mode[] = { M, 'b', '\0'};
    auto file = M == 'r' ? stdin : stdout;
    if (name == "-") {
        set_binary_mode(file);
    } else {
        file = fopen(name.c_str(), mode);
    }
    if (!file) {
        throw std::runtime_error("Failed to open file with " + (mode + (" mode: " + name)));
    }
    return file;
}

struct Args {
    bool keep_hashed = {};
    std::string dir = {};
    DynamicFormat const* input_format = {};
    FILE* input_file = {};
    DynamicFormat const* output_format = {};
    FILE* output_file = {};

    Args(int argc, char** argv) {
        argparse::ArgumentParser program("ritobin");
        program.add_argument("-k", "--keep-hashed")
                .help("do not run unhasher")
                .default_value(false)
                .implicit_value(true);
        program.add_argument("input_format")
                .help("format of input file")
                .required()
                .action(&parse_format);
        program.add_argument("input_file")
                .help("input file")
                .required()
                .action(&parse_file<'r'>);
        program.add_argument("output_format")
                .help("format of output file")
                .required()
                .action(&parse_format);
        program.add_argument("output_file")
                .help("output file")
                .required()
                .action(&parse_file<'w'>);
        try {
            program.parse_args(argc, argv);
            dir = argv[0];
            auto const slash = dir.find_last_of("/\\");
            dir =  slash == std::string::npos ? "." : dir.substr(0, slash);
            keep_hashed = program.get<bool>("--keep-hashed");
            input_format = program.get<DynamicFormat const*>("input_format");
            input_file = program.get<FILE*>("input_file");
            output_format = program.get<DynamicFormat const*>("output_format");
            output_file = program.get<FILE*>("output_file");
            if (output_format->output_allways_hashed()) {
                keep_hashed = true;
            }
        } catch (const std::runtime_error& err) {
            std::cerr << err.what() << std::endl;
            std::cerr << program << std::endl;
            std::cerr << "Formats:" << std::endl;
            for (auto format: ritobin::io::DynamicFormat::list()) {
                std::cerr << "\t- " << format->name() << std::endl;
            }
            exit(-1);
        }
    }

    void read(Bin& bin) {
        std::vector<char> data;
        char buffer[4096];
        while (auto read = fread(buffer, 1, sizeof(buffer), input_file)) {
            data.insert(data.end(), buffer, buffer + read);
        }
        auto error = input_format->read(bin, data);
        if (!error.empty()) {
            throw std::runtime_error(error);
        }
    }

    void unhash(Bin& bin) {
        if (!keep_hashed) {
            auto unhasher = BinUnhasher{};
            unhasher.load_fnv1a_CDTB(dir + "/hashes.binentries.txt");
            unhasher.load_fnv1a_CDTB(dir + "/hashes.binhashes.txt");
            unhasher.load_fnv1a_CDTB(dir + "/hashes.bintypes.txt");
            unhasher.load_fnv1a_CDTB(dir + "/hashes.binfields.txt");
            unhasher.load_xxh64_CDTB(dir + "/hashes.game.txt");
            unhasher.load_xxh64_CDTB(dir + "/hashes.lcu.txt");
            unhasher.unhash_bin(bin);
        }
    }

    void write(Bin const& bin) {
        std::vector<char> data;
        auto error = output_format->write(bin, data);
        if (!error.empty()) {
            throw std::runtime_error(error);
        }
        fwrite(data.data(), 1, data.size(), output_file);
        fflush(output_file);
    }
};

int main(int argc, char** argv) {
    try {
        auto args = Args(argc, argv);
        auto bin = Bin{};
        std::cerr << "Reading..." << std::endl;
        args.read(bin);
        std::cerr << "Unashing..." << std::endl;
        args.unhash(bin);
        std::cerr << "Writing..." << std::endl;
        args.write(bin);
        return 0;
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
}
