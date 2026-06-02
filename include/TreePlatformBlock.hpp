#ifndef TREE_PLATFORM_BLOCK_HPP
#define TREE_PLATFORM_BLOCK_HPP

#include <memory>
#include <vector>

#include "Block.hpp"

class TreePlatformBlock : public Block {
public:
    TreePlatformBlock(glm::vec2 position, int segments);

    Type GetType() const override { return Type::TreePlatform; }
    bool IsSolid() const override { return true; }
    void Draw(const Camera& camera) override;

private:
    struct Segment {
        std::shared_ptr<Util::GameObject> obj;
        glm::vec2 offset;
    };

    std::vector<Segment> m_Segments;
};

#endif
