#ifndef QUESTION_BLOCK_HPP
#define QUESTION_BLOCK_HPP

#include <memory>
#include "ThemeAssets.hpp"

#include "Block.hpp"
#include "Util/Animation.hpp"

class QuestionBlock : public Block {
public:
    explicit QuestionBlock(glm::vec2 position, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    Type GetType() const override { return Type::Question; }
    bool IsSolid() const override { return true; }

    /* 每幀更新：推進彈跳計時器 */
    void Update(float deltaTime) override;

    BlockHitResult OnHit(Player* player) override;

private:
    void StartBounce();

    ThemeAssets m_Assets;

    float m_BaseY       = 0.0f;
    float m_BounceTimer = 0.0f;

    std::shared_ptr<Util::Animation> m_IdleAnim;
};

#endif
