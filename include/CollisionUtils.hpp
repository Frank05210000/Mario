#ifndef COLLISION_UTILS_HPP
#define COLLISION_UTILS_HPP

#include <glm/vec2.hpp>

namespace CollisionUtils {

    /*
     * 判斷兩個 AABB (Axis-Aligned Bounding Box) 矩形是否重疊
     * posA, posB: 矩形的左上角座標
     * sizeA, sizeB: 矩形的寬高
     */
    inline bool CheckAABB(const glm::vec2& posA, const glm::vec2& sizeA, 
                          const glm::vec2& posB, const glm::vec2& sizeB) {
        bool overlapX = posA.x < posB.x + sizeB.x && posA.x + sizeA.x > posB.x;
        bool overlapY = posA.y < posB.y + sizeB.y && posA.y + sizeA.y > posB.y;
        return overlapX && overlapY;
    }

} // namespace CollisionUtils

#endif // COLLISION_UTILS_HPP
