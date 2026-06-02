#ifndef GAME_SESSION_HPP
#define GAME_SESSION_HPP

#include <array>
#include <string>

#include "Player.hpp"

enum class PlayerSlot {
    Mario,
    Luigi,
};

struct PlayerProgress {
    PlayerSlot slot = PlayerSlot::Mario;
    int lives = 3;
    int score = 0;
    int coins = 0;
    std::string levelName = "1-1";
    Player::Form form = Player::Form::SMALL;
};

class GameSession {
public:
    void ResetNewGame(int playerCount = 1);

    PlayerProgress& CurrentPlayer();
    const PlayerProgress& CurrentPlayer() const;

    int GetPlayerCount() const { return m_PlayerCount; }
    int GetCurrentPlayerIndex() const { return m_CurrentPlayerIndex; }
    std::string GetCurrentPlayerName() const;

    void AddScore(int amount);
    void AddCoin(int amount = 1);
    bool LoseLife();
    void SwitchToNextAlivePlayer();
    bool IsGameOver() const;

private:
    int m_PlayerCount = 1;
    int m_CurrentPlayerIndex = 0;
    std::array<PlayerProgress, 2> m_Players;
};

#endif
