#include "GameSession.hpp"

#include <algorithm>

void GameSession::ResetNewGame(int playerCount) {
    m_PlayerCount = std::clamp(playerCount, 1, 2);
    m_CurrentPlayerIndex = 0;

    m_Players[0] = PlayerProgress{};
    m_Players[0].slot = PlayerSlot::Mario;

    m_Players[1] = PlayerProgress{};
    m_Players[1].slot = PlayerSlot::Luigi;
}

PlayerProgress& GameSession::CurrentPlayer() {
    return m_Players[m_CurrentPlayerIndex];
}

const PlayerProgress& GameSession::CurrentPlayer() const {
    return m_Players[m_CurrentPlayerIndex];
}

std::string GameSession::GetCurrentPlayerName() const {
    return CurrentPlayer().slot == PlayerSlot::Mario ? "MARIO" : "LUIGI";
}

void GameSession::AddScore(int amount) {
    if (amount <= 0) return;
    CurrentPlayer().score += amount;
}

void GameSession::AddCoin(int amount) {
    if (amount <= 0) return;
    auto& player = CurrentPlayer();
    player.coins += amount;
    while (player.coins >= 100) {
        player.coins -= 100;
        AddLife();
    }
}

void GameSession::AddLife(int amount) {
    if (amount <= 0) return;
    CurrentPlayer().lives += amount;
}

bool GameSession::LoseLife() {
    auto& player = CurrentPlayer();
    if (player.lives > 0) {
        --player.lives;
    }
    return player.lives > 0;
}

void GameSession::SwitchToNextAlivePlayer() {
    if (IsGameOver()) return;

    for (int step = 1; step <= m_PlayerCount; ++step) {
        const int candidate = (m_CurrentPlayerIndex + step) % m_PlayerCount;
        if (m_Players[candidate].lives > 0) {
            m_CurrentPlayerIndex = candidate;
            return;
        }
    }
}

bool GameSession::IsGameOver() const {
    for (int i = 0; i < m_PlayerCount; ++i) {
        if (m_Players[i].lives > 0) return false;
    }
    return true;
}
