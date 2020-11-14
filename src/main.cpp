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

static DynamicFormat const* get_format(std::string const& name, std::string_view data, std::string const& file_name) {
    if (!name.empty()) {
        auto format = DynamicFormat::get(name);
        if (!format) {
            throw std::runtime_error("Format not found: " + name);
        }
        return format;
    } else {
        auto format = DynamicFormat::guess(data, file_name);
        if (!format) {
            throw std::runtime_error("Failed to guess format for file: " + file_name);
        }
        return format;
    }
}

struct Args {
    bool keep_hashed = {};
    std::string dir = {};
    std::string input_file = {};
    std::string output_file = {};
    std::string input_format = {};
    std::string output_format = {};

    Args(int argc, char** argv) {
        argparse::ArgumentParser program("ritobin");
        program.add_argument("-k", "--keep-hashed")
                .help("do not run unhasher")
                .default_value(false)
                .implicit_value(true);
        program.add_argument("input_file")
                .help("input file")
                .required();
        program.add_argument("output_file")
                .help("output file")
                .default_value(std::string(""));
        program.add_argument("-i", "--input-format")
                .default_value(std::string(""))
                .help("format of input file");
        program.add_argument("-o", "--output-format")
                .default_value(std::string(""))
                .help("format of output file");
        try {
            program.parse_args(argc, argv);
            dir = argv[0];
            auto const slash = dir.find_last_of("/\\");
            dir =  slash == std::string::npos ? "." : dir.substr(0, slash);
            keep_hashed = program.get<bool>("--keep-hashed");
            input_file = program.get<std::string>("input_file");
            output_file = program.get<std::string>("output_file");
            input_format = program.get<std::string>("--input-format");
            output_format = program.get<std::string>("--output-format");
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

    template<char M>
    FILE* open_file(std::string const& name) {
        char mode[] = { M, 'b', '\0'};
        std::cerr << "Open file for " << mode << ": " << name << std::endl;
        auto file = M == 'r' ? stdin : stdout;
        if (name == "-") {
            set_binary_mode(file);
        } else {
            file = fopen(name.c_str(), mode);
        }
        if (!file) {
            throw std::runtime_error("Failed to open file with!");
        }
        return file;
    }

    void read(Bin& bin) {
        auto file = open_file<'r'>(input_file);

        std::vector<char> data;
        char buffer[4096];
        std::cerr << "Reading..." << std::endl;
        while (auto read = fread(buffer, 1, sizeof(buffer), file)) {
            data.insert(data.end(), buffer, buffer + read);
        }
        fclose(file);

        std::cerr << "Parsing..." << std::endl;
        auto format = get_format(input_format, {data.begin(), data.end()}, input_file);
        auto error = format->read(bin, data);
        if (!error.empty()) {
            throw std::runtime_error(error);
        }
        if (output_file.empty() && output_format.empty()) {
            output_format = format->oposite_name();
        }
    }

    void unhash(Bin& bin) {
        if (!keep_hashed) {
            std::cerr << "Unashing..." << std::endl;
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

    void write(Bin& bin) {
        auto format = get_format(output_format, "", output_file);
        if (!keep_hashed && !format->output_allways_hashed()) {
            unhash(bin);
        }
        if (output_file.empty()) {
            if (input_file == "-") {
                output_file = "-";
            } else {
                output_file = input_file + std::string(format->default_extension());
            }
        }
        auto file = open_file<'w'>(output_file);

        std::cerr << "Serializing..." << std::endl;
        std::vector<char> data;
        auto error = format->write(bin, data);
        if (!error.empty()) {
            fclose(file);
            throw std::runtime_error(error);
        }

        std::cerr << "Writing data..." << std::endl;
        fwrite(data.data(), 1, data.size(), file);
        fflush(file);
        fclose(file);
    }
};

int main(int argc, char** argv) {
    try {
        auto args = Args(argc, argv);
        auto bin = Bin{};
        args.read(bin);
        args.write(bin);
        return 0;
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
}
