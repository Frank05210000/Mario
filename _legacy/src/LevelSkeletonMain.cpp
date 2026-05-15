#include <exception>
#include <iostream>
#include <string>

#include "BackgroundRenderer.h"
#include "CollisionSystem.h"
#include "IRenderer.h"
#include "LevelLoader.h"
#include "ObjectRenderer.h"

namespace {
class ConsoleRenderer final : public IRenderer {
public:
    void DrawImage(const std::string& imagePath,
                   float x,
                   float y,
                   float w,
                   float h) override {
        std::cout << "DrawImage: " << imagePath
                  << " at (" << x << ", " << y << ") size (" << w << ", " << h << ")\n";
    }

    void DrawSprite(const std::string& spriteName,
                    float x,
                    float y,
                    float w,
                    float h) override {
        std::cout << "DrawSprite: " << spriteName
                  << " at (" << x << ", " << y << ") size (" << w << ", " << h << ")\n";
    }
};
}

int main() {
    try {
        LevelLoader loader;
        const LevelData level = loader.LoadFromFile("data/1-1.json");

        ConsoleRenderer renderer;
        BackgroundRenderer backgroundRenderer;
        ObjectRenderer objectRenderer;

        backgroundRenderer.Render(level, renderer);
        objectRenderer.Render(level, renderer);

        const Rect playerBox {164.0F, 224.0F, 16.0F, 16.0F};
        const bool collided = CollisionSystem::CheckEntityCollision(playerBox, level);

        std::cout << "Player collision: " << (collided ? "true" : "false") << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "LevelSkeletonDemo failed: " << error.what() << '\n';
        return 1;
    }
}
