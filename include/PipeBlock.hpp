#ifndef PIPE_BLOCK_HPP
#define PIPE_BLOCK_HPP

#include <optional>
#include <string>

#include "Block.hpp"

class Player;

class PipeBlock : public Block {
public:
    explicit PipeBlock(glm::vec2 position,
                       glm::vec2 size,
                       const std::string& opening = "up");

    Type GetType() const override { return Type::Pipe; }
    bool IsSolid() const override { return true; }
    virtual bool IsEnterable() const { return false; }
    std::string GetOpening() const { return m_Opening; }
    virtual bool CanEnter(const Player& player,
                          bool pressingUp,
                          bool pressingDown,
                          bool pressingLeft,
                          bool pressingRight) const;
    virtual std::string GetDestinationLevel() const;
    virtual std::optional<glm::vec2> GetDestinationSpawn() const;

protected:
    std::string m_Opening = "up";
};

class EnterablePipeBlock : public PipeBlock {
public:
    explicit EnterablePipeBlock(glm::vec2 position,
                                glm::vec2 size,
                                const std::string& opening = "up",
                                const std::string& targetLevel = "",
                                const std::string& exitToLevel = "",
                                glm::vec2 targetSpawn = {0.0f, 0.0f},
                                bool hasTargetSpawn = false);

    bool IsEnterable() const override { return true; }
    bool CanEnter(const Player& player,
                  bool pressingUp,
                  bool pressingDown,
                  bool pressingLeft,
                  bool pressingRight) const override;
    std::string GetDestinationLevel() const override;
    std::optional<glm::vec2> GetDestinationSpawn() const override;

private:
    std::string m_TargetLevel;
    std::string m_ExitToLevel;
    glm::vec2 m_TargetSpawn = {0.0f, 0.0f};
    bool m_HasTargetSpawn = false;
};

class PipeCollisionBlock : public PipeBlock {
public:
    explicit PipeCollisionBlock(glm::vec2 position, glm::vec2 size);
};

#endif
