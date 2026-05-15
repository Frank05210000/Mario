#pragma once

#include <vector>

#include "LevelTypes.h"

class CollisionSystem {
public:
    static bool Intersects(const Rect& a, const Rect& b);
    static std::vector<const LevelObject*> GetSolidObjects(const LevelData& level);
    static bool CheckEntityCollision(const Rect& entityBox, const LevelData& level);
};
