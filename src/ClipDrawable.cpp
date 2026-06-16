#include "ClipDrawable.hpp"

#include <algorithm>
#include <cmath>

#include <GL/glew.h>

#include "Core/Context.hpp"

void ClipDrawable::Draw(const Core::Matrices& data) {
    if (!m_Inner) return;

    if (!m_Enabled) {
        m_Inner->Draw(data);
        return;
    }

    const auto ctx = Core::Context::GetInstance();
    const int W = static_cast<int>(ctx->GetWindowWidth());
    const int H = static_cast<int>(ctx->GetWindowHeight());

    const int line = static_cast<int>(std::floor(m_LinePx));
    int x = 0, y = 0, w = W, h = H;

    switch (m_Mode) {
        case Mode::KeepAbove: { const int L = std::clamp(line, 0, H); y = L; h = H - L; } break;
        case Mode::KeepBelow: { const int L = std::clamp(line, 0, H); y = 0; h = L;     } break;
        case Mode::KeepLeft:  { const int L = std::clamp(line, 0, W); x = 0; w = L;     } break;
        case Mode::KeepRight: { const int L = std::clamp(line, 0, W); x = L; w = W - L; } break;
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
    m_Inner->Draw(data);
    glDisable(GL_SCISSOR_TEST);
}
