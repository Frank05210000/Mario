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

    // ESC 不再直接關閉遊戲：交給 GameManager 的暫停選單處理。
    // 結束遊戲只剩兩種來源：視窗關閉鈕，或暫停選單選了「QUIT GAME」。
    const bool windowExitRequested = Util::Input::IfExit();
    const bool quitFromPauseMenu = m_GameManager.IsQuitRequested();
    if (windowExitRequested || quitFromPauseMenu) {
        LOG_INFO("App exit requested. windowExit={} pauseMenuQuit={}",
                 windowExitRequested,
                 quitFromPauseMenu);
        m_CurrentState = State::END;
    }
}

void App::End() {
    LOG_INFO("App end.");
}
