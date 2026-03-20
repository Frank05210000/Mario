#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "Block.hpp"
#include "Entity.hpp"

struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

class Collision {
public:
    static Rect MakeEntityBounds(const Entity& entity, Vec2 size);
    static Rect MakeBlockBounds(const Block& block);
    static bool Intersects(const Rect& a, const Rect& b);
};

#endif
