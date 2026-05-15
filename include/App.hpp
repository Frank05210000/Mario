#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export

#include "GameManager.hpp"

/*
 * App：應用程式進入點
 *
 * PTSD 框架會呼叫 Start → Update（循環）→ End。
 * App 只負責轉接給 GameManager，自己不做遊戲邏輯。
 *
 * OOP 關係：
 *   - App「持有」GameManager（組合，has-a）
 *   - App 不繼承任何人
 */
class App {
public:
    enum class State { START, UPDATE, END };

    State GetCurrentState() const { return m_CurrentState; }

    void Start();
    void Update();
    void End();

private:
    State       m_CurrentState = State::START;
    GameManager m_GameManager;  // 持有遊戲管理器
};

#endif
