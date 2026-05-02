#pragma once

#include <source_location>
#include <stdexcept>

#define GAPP_VERSION "pre1.5.2"

#define GAPP_THROW(message)                                                       \
    throw std::runtime_error(                                                     \
        std::format("{}:{}\n{}\n{}", std::source_location::current().file_name(), \
                    std::source_location::current().line(),                       \
                    std::source_location::current().function_name(), message))

inline uint32_t xorshift32() noexcept {
    static uint32_t state = 0xDEADBEEF;

    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return state = x;
}

class Constants {
   public:
    static constexpr char userDataPath[] = "userdata/";
    static constexpr char uiSettingsFile[] = "userdata/ui_settings.json";
    static constexpr char imguiIniFile[] = "userdata/imgui.ini";

    static constexpr char assetsFolder[] = "assets/";
    static constexpr char unitbvLogoPath[] = "assets/unitbv.png";
    static constexpr char defaultFontPath[] = "assets/CozetteVector.otf";
    static constexpr char appIconPath[] = "assets/icon.png";
};
