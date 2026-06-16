#ifndef CLIP_DRAWABLE_HPP
#define CLIP_DRAWABLE_HPP

#include <memory>
#include <utility>

#include "Core/Drawable.hpp"

/*
 * ClipDrawable：包裝任一 Drawable，在繪製時用 glScissor 只保留某條裁切線的「一側」。
 *
 * 用途：物件「進入/冒出」管狀構造（問號磚、水管）時，把不該露出的部分裁掉——
 *   - 道具自問號磚冒出：只露出方塊頂線以上（KeepAbove）。
 *   - 食人花自水管升出：只露出管口以上（KeepAbove）。
 *   - 馬力歐進/出水管：依開口方向裁切（上→KeepAbove、左→KeepLeft…）。
 *
 * 因為本作水管/方塊都只是背景圖（z=-1），上層的動態物件無法被它遮住，
 * 改用裁切就能在不新增前景貼圖、也不改 PTSD 引擎的前提下做出遮擋效果。
 *
 * 裁切線以 framebuffer 像素表示（原點左下、Y 向上、X 向右）。
 */
class ClipDrawable : public Core::Drawable {
public:
    enum class Mode { KeepAbove, KeepBelow, KeepLeft, KeepRight };

    explicit ClipDrawable(std::shared_ptr<Core::Drawable> inner)
        : m_Inner(std::move(inner)) {}

    // 替換被包裝的內層可繪物（給每幀切換動畫的物件用，例如玩家）。
    void SetInner(const std::shared_ptr<Core::Drawable>& inner) { m_Inner = inner; }

    glm::vec2 GetSize() const override {
        return m_Inner ? m_Inner->GetSize() : glm::vec2{0.0f, 0.0f};
    }

    void Draw(const Core::Matrices& data) override;

    void SetEnabled(bool enabled) { m_Enabled = enabled; }

    // 保留裁切線的某一側（px 為 framebuffer 像素）。
    void SetClipKeepAbovePx(float px) { m_Mode = Mode::KeepAbove; m_LinePx = px; }
    void SetClipKeepBelowPx(float px) { m_Mode = Mode::KeepBelow; m_LinePx = px; }
    void SetClipKeepLeftPx(float px)  { m_Mode = Mode::KeepLeft;  m_LinePx = px; }
    void SetClipKeepRightPx(float px) { m_Mode = Mode::KeepRight; m_LinePx = px; }

private:
    std::shared_ptr<Core::Drawable> m_Inner;
    Mode  m_Mode    = Mode::KeepAbove;
    float m_LinePx  = 0.0f;
    bool  m_Enabled = false;
};

#endif
