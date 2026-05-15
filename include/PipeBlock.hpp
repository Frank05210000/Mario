#ifndef PIPE_BLOCK_HPP
#define PIPE_BLOCK_HPP

#include <vector>
#include <string>

#include "Block.hpp"

class PipeBlock : public Block {
public:
    explicit PipeBlock(glm::vec2 position,
                       glm::vec2 size,
                       const std::string& opening = "up",
                       bool enterable = false,
                       const std::string& targetLevel = "",
                       const std::string& exitToLevel = "",
                       glm::vec2 targetSpawn = {0.0f, 0.0f},
                       bool hasTargetSpawn = false,
                       const std::string& theme = "ground");

    Type GetType() const override { return Type::Pipe; }
    bool IsSolid() const override { return true; }
    bool IsEnterable() const { return m_Enterable; }
    std::string GetOpening() const { return m_Opening; }
    std::string GetTargetLevel() const { return m_TargetLevel; }
    std::string GetExitToLevel() const { return m_ExitToLevel; }
    glm::vec2 GetTargetSpawn() const { return m_TargetSpawn; }
    bool HasTargetSpawn() const { return m_HasTargetSpawn; }
    void Draw(const Camera& camera) override;

private:
    struct Segment {
        std::shared_ptr<Util::GameObject> obj;
        glm::vec2 offset;
        glm::vec2 displaySize;
        glm::vec2 scale = {1.0f, 1.0f};
        float rotation = 0.0f;
    };
    std::vector<Segment> m_Segments;
    bool m_Enterable = false;
    std::string m_Opening = "up";
    std::string m_TargetLevel;
    std::string m_ExitToLevel;
    glm::vec2 m_TargetSpawn = {0.0f, 0.0f};
    bool m_HasTargetSpawn = false;
    std::string m_Theme = "ground";
};

#endif
