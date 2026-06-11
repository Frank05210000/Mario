#ifndef QUESTION_BLOCK_HPP
#define QUESTION_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

class QuestionBlock : public Block {
public:
    explicit QuestionBlock(glm::vec2 position, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    Type GetType() const override { return Type::Question; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    ThemeAssets m_Assets;
};

#endif
