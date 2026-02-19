module;
#include <pch.h>

module utils;

GLuint TextureLoader::loadPNGFile(const char* filePath) {
    GLuint imageTexture{};

    std::vector<uint8_t> imageData;
    uint32_t width, height;

    const auto error = lodepng::decode(imageData, width, height, filePath);
    if (error) {
        GAPP_THROW(std::string("Failed to load texture: ") + lodepng_error_text(error));
    }

    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 imageData.data());

    return imageTexture;
}
