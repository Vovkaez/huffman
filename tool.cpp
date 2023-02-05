#include "argagg/argagg.hpp"
#include "huffman-lib/huffman.h"
#include <fstream>

int main(int argc, char const** argv) {
  try {
    argagg::parser argparser{{
        {"compress", {"--compress"}, "compress a file", 0},
        {"decompress", {"--decompress"}, "decompress a file", 0},
        {"input", {"--input"}, "specify input file", 1},
        {"output", {"--output"}, "specify output file", 1},
        {"help", {"-h", "--help"}, "shows this help message", 0},
    }};
    argagg::parser_results args = argparser.parse(argc, argv);
    if (args.count() > 0) {
      throw std::invalid_argument("no positional arguments expected, got " +
                                  std::to_string(args.count()));
    }
    if (args["help"]) {
      std::cout << argparser;
      return EXIT_SUCCESS;
    }
    if (!args["input"]) {
      std::cerr << "Input is not specified\n";
      return EXIT_FAILURE;
    }
    if (!args["output"]) {
      std::cerr << "Output is not specified\n";
      return EXIT_FAILURE;
    }
    if (!(static_cast<bool>(args["compress"]) ^
          static_cast<bool>(args["decompress"]))) {
      std::cerr << "Choose one mode [--compress/--decompress]\n";
      return EXIT_FAILURE;
    }
    std::string in_fname{args["input"].as<std::string>()};
    std::string out_fname{args["output"].as<std::string>()};
    std::ifstream inf;
    inf.exceptions(std::ifstream::badbit);
    inf.open(in_fname);
    if (!inf.is_open()) {
      std::cerr << "Failed to open " << in_fname << '\n';
      return EXIT_FAILURE;
    }
    std::ofstream outf;
    outf.exceptions(std::ofstream::badbit);
    outf.open(out_fname);
    if (!outf.is_open()) {
      std::cerr << "Failed to open " << out_fname << '\n';
      return EXIT_FAILURE;
    }
    if (args["compress"]) {
      huffman::encode(inf, outf);
    } else {
      huffman::decode(inf, outf);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
