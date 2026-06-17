#include "GameSession.hpp"

#include "Util/Logger.hpp"

#include <algorithm>

void GameSession::ResetNewGame(int playerCount) {
    m_PlayerCount = std::clamp(playerCount, 1, 2);
    m_CurrentPlayerIndex = 0;

    m_Players[0] = PlayerProgress{};
    m_Players[0].slot = PlayerSlot::Mario;

    m_Players[1] = PlayerProgress{};
    m_Players[1].slot = PlayerSlot::Luigi;

    LOG_INFO("GameSession reset: playerCount={}", m_PlayerCount);
}

PlayerProgress& GameSession::CurrentPlayer() {
    return m_Players[m_CurrentPlayerIndex];
}

const PlayerProgress& GameSession::CurrentPlayer() const {
    return m_Players[m_CurrentPlayerIndex];
}

PlayerProgress& GameSession::GetPlayerProgress(int index) {
    return m_Players[std::clamp(index, 0, 1)];
}

const PlayerProgress& GameSession::GetPlayerProgress(int index) const {
    return m_Players[std::clamp(index, 0, 1)];
}

std::string GameSession::GetCurrentPlayerName() const {
    return CurrentPlayer().slot == PlayerSlot::Mario ? "MARIO" : "LUIGI";
}

void GameSession::AddScore(int amount) {
    if (amount <= 0) return;
    CurrentPlayer().score += amount;
    LOG_INFO("Score added: amount={} total={}", amount, CurrentPlayer().score);
}

void GameSession::AddCoin(int amount) {
    if (amount <= 0) return;
    auto& player = CurrentPlayer();
    player.coins += amount;
    LOG_INFO("Coin added: amount={} coins={}", amount, player.coins);
    while (player.coins >= 100) {
        player.coins -= 100;
        AddLife();
    }
}

void GameSession::AddLife(int amount) {
    if (amount <= 0) return;
    CurrentPlayer().lives += amount;
    LOG_INFO("Life added: amount={} lives={}", amount, CurrentPlayer().lives);
}

bool GameSession::LoseLife() {
    auto& player = CurrentPlayer();
    if (player.lives > 0) {
        --player.lives;
    }
    LOG_INFO("Life lost: lives={}", player.lives);
    return player.lives > 0;
}

bool GameSession::SwitchToNextAlivePlayer() {
    if (IsGameOver()) return false;

    for (int step = 1; step <= m_PlayerCount; ++step) {
        const int candidate = (m_CurrentPlayerIndex + step) % m_PlayerCount;
        if (m_Players[candidate].lives > 0) {
            m_CurrentPlayerIndex = candidate;
            LOG_INFO("Switched player: index={} name={}", m_CurrentPlayerIndex, GetCurrentPlayerName());
            return true;
        }
    }

    return false;
}

bool GameSession::IsGameOver() const {
    for (int i = 0; i < m_PlayerCount; ++i) {
        if (m_Players[i].lives > 0) return false;
    }
    return true;
}
