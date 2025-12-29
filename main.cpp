#include "search_engine.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  std::string configDir = ".";

  if (argc > 1) {
    configDir = argv[1];
  }

  try {
    SearchEngine engine(configDir);

    if (!engine.initialize()) {
      std::cerr << "Failed to initialize search engine.\n";
      return 1;
    }

    engine.run();

  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
