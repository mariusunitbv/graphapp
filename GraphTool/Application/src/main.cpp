#include <pch.h>

import application;

import graph_common;

int main() {
    try {
        Application::get().initialize();

        common::FileSystem::get().createFolder(Constants::userDataPath);

#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg([](void*) { Application::get().run(); }, nullptr, 0, true);
#else
        Application::get().run();
#endif
    } catch (const std::exception& ex) {
        Application::get().quit();

        std::cerr << "An error occurred:\n" << ex.what() << std::endl;
        std::cin.get();

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
