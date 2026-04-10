#pragma once

#include <string>

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void DrawImage(const std::string& imagePath,
                           float x,
                           float y,
                           float w,
                           float h) = 0;

    virtual void DrawSprite(const std::string& spriteName,
                            float x,
                            float y,
                            float w,
                            float h) = 0;
};
