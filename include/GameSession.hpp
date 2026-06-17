#ifndef GAME_SESSION_HPP
#define GAME_SESSION_HPP

#include <array>
#include <optional>
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
    std::optional<glm::vec2> checkpoint;
};

class GameSession {
public:
    void ResetNewGame(int playerCount = 1);

    PlayerProgress& CurrentPlayer();
    const PlayerProgress& CurrentPlayer() const;
    PlayerProgress& GetPlayerProgress(int index);
    const PlayerProgress& GetPlayerProgress(int index) const;

    int GetPlayerCount() const { return m_PlayerCount; }
    int GetCurrentPlayerIndex() const { return m_CurrentPlayerIndex; }
    PlayerSlot GetCurrentPlayerSlot() const { return CurrentPlayer().slot; }
    std::string GetCurrentPlayerName() const;

    void AddScore(int amount);
    void AddCoin(int amount = 1);
    void AddLife(int amount = 1);
    bool LoseLife();
    bool SwitchToNextAlivePlayer();
    bool IsGameOver() const;

private:
    int m_PlayerCount = 1;
    int m_CurrentPlayerIndex = 0;
    std::array<PlayerProgress, 2> m_Players;
};

#endif
