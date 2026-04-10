#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "Block.hpp"
#include "Entity.hpp"
#include "LevelTypes.h"

class Collision {
public:
    static Rect MakeEntityBounds(const Entity& entity, Vec2 size);
    static Rect MakeBlockBounds(const Block& block);
    static bool Intersects(const Rect& a, const Rect& b);
};

#endif
