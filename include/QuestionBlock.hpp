#ifndef QUESTION_BLOCK_HPP
#define QUESTION_BLOCK_HPP

#include <memory>
#include <string>

#include "Block.hpp"
#include "Util/Animation.hpp"

class QuestionBlock : public Block {
public:
    explicit QuestionBlock(glm::vec2 position, const std::string& theme = "ground");

    Type GetType() const override { return Type::Question; }
    bool IsSolid() const override { return true; }

    /* 每幀更新：推進彈跳計時器 */
    void Update(float deltaTime) override;

    BlockHitResult OnHit(Player* player) override;

private:
    /* 啟動彈跳動畫（與 BrickBlock 相同的 sin 曲線） */
    void StartBounce();

    std::string m_Theme = "ground";

    // 彈跳動畫
    float m_BaseY       = 0.0f;   // 靜止時的 Y
    float m_BounceTimer = 0.0f;   // 彈跳剩餘秒數；0 = 靜止

    // 閃爍動畫（閒置時）
    std::shared_ptr<Util::Animation> m_IdleAnim;
};

#endif
