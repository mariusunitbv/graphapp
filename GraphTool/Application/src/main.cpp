#include <pch.h>

import application;

int main() {
    try {
        Application::get().initialize();
        Application::get().run();
    } catch (const std::exception& ex) {
        Application::get().quit();

        std::cerr << "An error occurred:\n" << ex.what() << std::endl;
        std::cin.get();

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
