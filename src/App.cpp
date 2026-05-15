#include "App.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");
    m_GameManager.Start();       // 初始化遊戲
    m_CurrentState = State::UPDATE;
}

void App::Update() {
    m_GameManager.Update();      // 每幀更新遊戲

    // 按 ESC 或關閉視窗時結束
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() {
    LOG_TRACE("End");
}
