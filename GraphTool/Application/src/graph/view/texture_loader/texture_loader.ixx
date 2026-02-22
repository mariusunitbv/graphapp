module;
#include <pch.h>

export module texture_loader;

export class TextureLoader {
   public:
    static GLuint loadPNGFile(const char* filePath);
    static void unloadTexture(GLuint textureId);
};
