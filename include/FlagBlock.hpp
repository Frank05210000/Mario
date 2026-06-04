#ifndef FLAG_BLOCK_HPP
#define FLAG_BLOCK_HPP

#include "Block.hpp"

/*
 * FlagBlock：終點旗杆
 *
 * JSON 只記錄旗杆底部的座標，
 * 程式自動根據 FLAG_POLE_TILES 往上推算偵測範圍。
 *
 * 計分邏輯：
 *   玩家碰到旗杆時，依接觸高度回傳不同分數（越高越多）。
 */
class FlagBlock : public Block {
public:
    explicit FlagBlock(glm::vec2 bottomPosition);

    Type GetType() const override { return Type::Flag; }
    bool IsSolid() const override { return false; }
    void Draw(const Camera& camera) override;

    /*
     * 根據玩家碰到旗杆的 Y 世界座標，計算對應得分。
     *
     * @param playerY  玩家腳底的世界 Y 座標
     * @return         對應分數（5000 / 2000 / 800 / 400 / 100）
     *
     * ratio = (旗杆底部 - 玩家Y) / 旗杆總高
     * ratio 越大代表碰得越高，分數越高。
     */
    int GetContactScore(float playerY) const;
};

#endif
