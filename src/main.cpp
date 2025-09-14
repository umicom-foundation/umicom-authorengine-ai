#include <iostream>
#include <string>
#include "ueng/version.hpp"

static void usage() {
    std::cout << "ueng " << UENG_VERSION << "\n"
              << "Usage: ueng <init|build|serve|publish> [options]\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 0; }
    std::string cmd = argv[1];

    if (cmd == "init") {
        std::cout << "[init] scaffold manifest/book.yaml (TODO)\n";
    } else if (cmd == "build") {
        std::cout << "[build] run pipeline: ingestor->outliner->scribe->editor->renderer (TODO)\n";
    } else if (cmd == "serve") {
        std::cout << "[serve] preview static site (TODO)\n";
    } else if (cmd == "publish") {
        std::cout << "[publish] create/approve repo and push artefacts (TODO)\n";
    } else if (cmd == "-v" || cmd == "--version") {
        std::cout << UENG_VERSION << "\n";
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        usage();
        return 1;
    }
    return 0;
}
