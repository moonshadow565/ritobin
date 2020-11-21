#include <cstdlib>
#include <portable-file-dialogs.h>
#include <ritobin/bin_io.hpp>
#include <ritobin/bin_unhash.hpp>

using ritobin::Bin;
using ritobin::BinUnhasher;
using ritobin::io::DynamicFormat;

struct App {
    std::string dir;
    std::string error;
    std::optional<BinUnhasher> unhasher{};
    std::string input_filename = {};
    std::string output_filename = {};
    DynamicFormat const* input_format = {};
    DynamicFormat const* output_format = {};
    std::vector<char> data = {};

    void set_dir_from_apppath(std::string const& app) {
        auto const slash = app.find_last_of("/\\");
        dir =  slash == std::string::npos ? "." : app.substr(0, slash);
    }

    BinUnhasher& get_unhasher() {
        if (!unhasher) {
            unhasher = BinUnhasher{};
            unhasher->load_fnv1a_CDTB(dir + "/hashes/hashes.binentries.txt");
            unhasher->load_fnv1a_CDTB(dir + "/hashes/hashes.binhashes.txt");
            unhasher->load_fnv1a_CDTB(dir + "/hashes/hashes.bintypes.txt");
            unhasher->load_fnv1a_CDTB(dir + "/hashes/hashes.binfields.txt");
            unhasher->load_xxh64_CDTB(dir + "/hashes/hashes.game.txt");
            unhasher->load_xxh64_CDTB(dir + "/hashes/hashes.lcu.txt");
        }
        return *unhasher;
    }

    bool read_file() {
        data.clear();
        printf("Reading file: %s\n", input_filename.c_str());
        auto file = fopen(input_filename.c_str(), "rb");
        if (!file) {
            return false;
        }
        auto start = ftell(file);
        fseek(file, 0, SEEK_END);
        auto end = ftell(file);
        fseek(file, start, SEEK_SET);
        data.resize((size_t)(end - start));
        fread(data.data(), 1, data.size(), file);
        fclose(file);
        return true;
    }

    bool write_file() {
        printf("Writing file: %s\n", output_filename.c_str());
        auto file = fopen(output_filename.c_str(), "wb");
        if (!file) {
            return false;
        }
        fwrite(data.data(), 1, data.size(), file);
        fflush(file);
        fclose(file);
        return true;
    }

    bool get_input_filename() {
        auto results = pfd::open_file("ritobin input file", "",
                                      {
                                          "bin files", "*.bin",
                                          "text files", "*.txt *.py",
                                          "json files", "*.json",
                                          "All Files", "*",
                                      }).result();
        if (results.empty()) {
            input_filename = "";
            return false;
        }
        input_filename = std::move(results.front());
        return true;
    }

    bool get_output_filename() {
        DynamicFormat const* guessed_output_format = DynamicFormat::get(input_format->oposite_name());
        std::string guessed_output_name = {};
        if (guessed_output_format) {
            std::string extension = std::string(guessed_output_format->default_extension());
            if (!extension.empty()) {
                auto const dot = input_filename.find_last_of(".");
                guessed_output_name = input_filename.substr(0, dot) + extension;
            }
        }
        printf("Guessed name: %s\n", guessed_output_name.c_str());
        auto results = pfd::save_file("ritobin output file", guessed_output_name,
                                      {
                                          "All Files", "*",
                                          "bin files", "*.bin",
                                          "text files", "*.txt *.py",
                                          "json files", "*.json",
                                      }, pfd::opt::force_overwrite).result();
        if (results.empty()) {
            output_filename = "";
            return false;
        }
        output_filename = std::move(results);
        return true;
    }

    bool run_once() {
        Bin bin = {};
        if (!get_input_filename()) {
            fprintf(stderr, "Failed to open file!\n");
            return false;
        }
        if (!read_file()) {
            fprintf(stderr, "Failed to read file!\n");
            return false;
        }
        input_format = DynamicFormat::guess(data, input_filename);
        if (!input_format) {
            fprintf(stderr, "Input file has unknown format!\n");
            return false;
        }
        error = input_format->read(bin, data);
        if (!error.empty()) {
            fprintf(stderr, "Failed to process file:%s\n", error.c_str());
            return false;
        }
        if (!get_output_filename()) {
            fprintf(stderr, "No output file selected!\n");
            return false;
        }
        data.clear();
        output_format = DynamicFormat::guess(data, output_filename);
        if (!output_format) {
            fprintf(stderr, "Output file has unknown format!\n");
            return false;
        }
        if (!output_format->output_allways_hashed()) {
            auto& unhasher = get_unhasher();
            unhasher.unhash_bin(bin);
        }
        error = output_format->write(bin, data);
        if (!error.empty()) {
            fprintf(stderr, "Failed to generate file:%s\n", error.c_str());
            return false;
        }
        if (!write_file()) {
            fprintf(stderr, "Failed to write file!\n");
            return false;
        }
        fprintf(stderr, "Ok!\n");
        return true;
    }
};

int main(int, char** argv) {
    App app = {};
    app.set_dir_from_apppath(argv[0]);
    bool result = app.run_once();
    [[maybe_unused]] int c = getc(stdin);
    fprintf(stderr, "Press enter to exit or close this window...\n");
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
