#ifndef QUESTION_BLOCK_HPP
#define QUESTION_BLOCK_HPP

#include <string>

#include "Block.hpp"

class QuestionBlock : public Block {
public:
    explicit QuestionBlock(glm::vec2 position, const std::string& theme = "ground");

    Type GetType() const override { return Type::Question; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    std::string m_Theme = "ground";
};

#endif
