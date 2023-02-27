#include <cstdlib>
#include <argparse.hpp>
#include <ritobin/bin_io.hpp>
#include <ritobin/bin_unhash.hpp>
#include <optional>
#include <filesystem>

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
namespace fs = std::filesystem;

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
    bool recursive = {};
    bool log = {};

    std::string dir = {};
    std::string input_file = {};
    std::string output_file = {};
    std::string input_dir = {};
    std::string output_dir = {};
    std::string input_format = {};
    std::string output_format = {};
    std::shared_ptr<std::optional<BinUnhasher>> unhasher = {};

    Args(int argc, char** argv) {
        argparse::ArgumentParser program("ritobin");
        program.add_argument("-k", "--keep-hashed")
                .help("do not run unhasher")
                .default_value(false)
                .implicit_value(true);
        program.add_argument("-r", "--recursive")
                .help("run on directory")
                .default_value(false)
                .implicit_value(true);
        program.add_argument("-v", "--verbose")
                .help("log more")
                .default_value(false)
                .implicit_value(true);
        program.add_argument("input")
                .help("input file or directory")
                .required();
        program.add_argument("output")
                .help("output file or directory")
                .default_value(std::string(""));
        program.add_argument("-i", "--input-format")
                .default_value(std::string(""))
                .help("format of input file");
        program.add_argument("-o", "--output-format")
                .default_value(std::string(""))
                .help("format of output file");
        program.add_argument("-d", "--dir-hashes")
                .default_value((fs::path(argv[0]).parent_path() / "hashes").generic_string())
                .help("directory containing hashes");
        try {
            program.parse_args(argc, argv);
            dir =  program.get<std::string>("--dir-hashes");
            keep_hashed = program.get<bool>("--keep-hashed");
            recursive = program.get<bool>("--recursive");
            log = program.get<bool>("--verbose");
            input_format = program.get<std::string>("--input-format");
            output_format = program.get<std::string>("--output-format");
            if (recursive) {
                input_dir = program.get<std::string>("input");
                output_dir = program.get<std::string>("output");
            } else {
                input_file = program.get<std::string>("input");
                output_file = program.get<std::string>("output");
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
        unhasher = std::make_shared<std::optional<BinUnhasher>>(std::nullopt);
    }

    template<char M>
    FILE* open_file(std::string const& name) {
        char mode[] = { M, 'b', '\0'};
        if (log) {
            std::cerr << "Open file for " << mode << ": " << name << std::endl;
        }
        auto file = M == 'r' ? stdin : stdout;
        if (name == "-") {
            set_binary_mode(file);
        } else {
            if constexpr (M == 'w') {
                auto parent_dir = fs::path(name).parent_path();
                if (std::error_code ec = {}; (fs::create_directories(parent_dir, ec)), ec != std::error_code{}) {
                    throw std::runtime_error("Failed to create parent directory: " + ec.message());
                }
            }
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
        if (log) {
            std::cerr << "Reading..." << std::endl;
        }
        while (auto read = fread(buffer, 1, sizeof(buffer), file)) {
            data.insert(data.end(), buffer, buffer + read);
        }
        fclose(file);

        if (log) {
            std::cerr << "Parsing..." << std::endl;
        }
        auto format = get_format(input_format, std::string_view{data.data(), data.size()}, input_file);
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
            if (!*unhasher) {
                if (log) {
                    std::cerr << "Loading hashes..." << std::endl;
                }
                auto& uh = unhasher->emplace();
                if (dir.empty()) {
                    dir = ".";
                }
                uh.load_fnv1a_CDTB(dir + "/hashes.binentries.txt");
                uh.load_fnv1a_CDTB(dir + "/hashes.binhashes.txt");
                uh.load_fnv1a_CDTB(dir + "/hashes.bintypes.txt");
                uh.load_fnv1a_CDTB(dir + "/hashes.binfields.txt");
                uh.load_xxh64_CDTB(dir + "/hashes.game.txt");
                uh.load_xxh64_CDTB(dir + "/hashes.lcu.txt");
            }
            if (log) {
                std::cerr << "Unashing..." << std::endl;
            }
            (*unhasher)->unhash_bin(bin);
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
                output_file = fs::path(input_file).replace_extension(format->default_extension()).generic_string();
                if (recursive && !output_dir.empty()) {
                    output_file = (output_dir / fs::relative(output_file, input_dir)).generic_string();
                }
            }
        }

        if (log) {
            std::cerr << "Serializing..." << std::endl;
        }
        std::vector<char> data;
        auto error = format->write(bin, data);
        if (!error.empty()) {
            throw std::runtime_error(error);
        }

        auto file = open_file<'w'>(output_file);
        if (log) {
            std::cerr << "Writing data..." << std::endl;
        }
        fwrite(data.data(), 1, data.size(), file);
        fflush(file);
        fclose(file);
    }

    void run_once() {
        try {
            auto bin = Bin{};
            read(bin);
            write(bin);
        } catch (const std::runtime_error& err) {
            std::cerr << "In: " << input_file << std::endl;
            std::cerr << "Out: " << output_file << std::endl;
            std::cerr << "Error: " << err.what() << std::endl;
        }
    }

    void run() {
        if (!recursive) {
            return run_once();
        }

        if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
            throw std::runtime_error("Input directory doesn't exist!");
        }

        if (input_format.empty()) {
            throw std::runtime_error("Recursive run needs input format!");
        }

        auto const format = get_format(input_format, "", "");
        if (!format) {
            throw std::runtime_error("No format found for recursive run!");
        }

        auto const extension = format->default_extension();
        if (format->default_extension().empty()) {
            throw std::runtime_error("Format must have default extension!");
        }

        for (auto const& entry: fs::recursive_directory_iterator(input_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            auto const path = entry.path();
            if (path.extension() != extension) {
                continue;
            }
            this->input_file = path.generic_string();
            Args {*this}.run_once();
        }
    }
};



int main(int argc, char** argv) {
    try {
        auto args = Args(argc, argv);
        args.run();
        return 0;
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }
}
