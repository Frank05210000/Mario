#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/vec2.hpp>

/*
 * Camera：鏡頭管理
 *
 * 負責追蹤玩家，計算鏡頭的水平偏移量（m_X）。
 * 同時作為全域的「世界座標 -> 螢幕座標」換算中心，確保所有物件套用 GAME_SCALE。
 *
 * OOP 關係：
 *   - 不繼承任何人（純計算工具，不是渲染物件）
 *   - GameManager「持有」Camera（組合，has-a）
 */
class Camera {
public:
    Camera() = default;

    /*
     * 每幀更新鏡頭位置
     *
     * @param targetX   追蹤目標的世界 X 座標（通常是玩家的 X）
     * @param levelWidth 地圖總寬度（世界像素），供邊界夾制使用
     */
    void Update(float targetX, float levelWidth);

    /*
     * 將遊戲邏輯世界座標轉換為 PTSD 螢幕繪製座標 (套用 GAME_SCALE)
     */
    glm::vec2 WorldToScreen(glm::vec2 worldPos) const;

    /*
     * 取得鏡頭左邊界的世界 X 座標
     */
    float GetX() const { return m_X; }

    /*
     * 取得鏡頭目前可視範圍的世界寬度（視窗寬 / GAME_SCALE）
     * 鏡頭右邊界 = GetX() + GetViewWorldWidth()
     */
    float GetViewWorldWidth() const;

    // 重設鏡頭到關卡起點（關卡開始或重新啟動時呼叫）
    void Reset() { m_X = 0.0f; }

private:
    float m_X = 0.0f;  // 鏡頭左邊界的世界座標
};

#endif
