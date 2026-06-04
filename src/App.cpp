#include "App.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_INFO("App start.");
    m_GameManager.Start();       // 初始化遊戲
    m_CurrentState = State::UPDATE;
}

void App::Update() {
    m_GameManager.Update();      // 每幀更新遊戲

    // 按 ESC 或關閉視窗時結束
    const bool escapeReleased = Util::Input::IsKeyUp(Util::Keycode::ESCAPE);
    const bool windowExitRequested = Util::Input::IfExit();
    if (escapeReleased || windowExitRequested) {
        LOG_INFO("App exit requested. escape={} windowExit={}",
                 escapeReleased,
                 windowExitRequested);
        m_CurrentState = State::END;
    }
}

void App::End() {
    LOG_INFO("App end.");
}
