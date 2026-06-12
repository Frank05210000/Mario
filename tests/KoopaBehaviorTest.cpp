#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "GameConstants.hpp"
#include "Koopa.hpp"

TEST(KoopaBehaviorTest, WalkingStompEntersShell) {
    EXPECT_EQ(Koopa::OutcomeForStomp(Koopa::State::Walking),
              Enemy::StompOutcome::EnteredShell);
    EXPECT_EQ(Koopa::StateAfterStomp(Koopa::State::Walking),
              Koopa::State::ShellIdle);
}

TEST(KoopaBehaviorTest, SlidingStompStopsShell) {
    EXPECT_EQ(Koopa::OutcomeForStomp(Koopa::State::ShellSliding),
              Enemy::StompOutcome::StoppedShell);
    EXPECT_EQ(Koopa::StateAfterStomp(Koopa::State::ShellSliding),
              Koopa::State::ShellIdle);
}

TEST(KoopaBehaviorTest, RepeatedStompResetsStationaryShell) {
    EXPECT_EQ(Koopa::OutcomeForStomp(Koopa::State::ShellReviving),
              Enemy::StompOutcome::StoppedShell);
    EXPECT_EQ(Koopa::StateAfterStomp(Koopa::State::ShellReviving),
              Koopa::State::ShellIdle);
}

TEST(KoopaBehaviorTest, DefeatedKoopaIgnoresStomp) {
    EXPECT_EQ(Koopa::OutcomeForStomp(Koopa::State::Defeated),
              Enemy::StompOutcome::NoEffect);
    EXPECT_EQ(Koopa::StateAfterStomp(Koopa::State::Defeated),
              Koopa::State::Defeated);
}

TEST(KoopaBehaviorTest, ReviveWarningPrecedesWalking) {
    EXPECT_EQ(Koopa::StationaryShellStateForTimer(0.0f),
              Koopa::State::ShellIdle);
    EXPECT_EQ(
        Koopa::StationaryShellStateForTimer(
            KOOPA_REVIVE_DELAY - KOOPA_REVIVE_WARNING),
        Koopa::State::ShellReviving);
    EXPECT_EQ(Koopa::StationaryShellStateForTimer(KOOPA_REVIVE_DELAY),
              Koopa::State::Walking);
}

TEST(KoopaBehaviorTest, ShellSpeedUsesWorldCoordinates) {
    EXPECT_FLOAT_EQ(KOOPA_SHELL_SPEED, 180.0f);
}

TEST(KoopaBehaviorTest, RuntimeAssetsExist) {
    const std::string root = std::string(RESOURCE_DIR) + "/Asset/enemy/Koopa/";
    const char* paths[] = {
        "ground/normal/shell/shell_revive.png",
        "ground/red/walk/walk-1.png",
        "ground/red_reverse/walk/walk-1.png",
        "ground/red/fly/fly-1.png",
        "ground/red/shell/shell_revive.png",
        "underground/normal/shell/shell_revive.png",
        "underground/normal/shell/shell_flip.png",
        "underground/red/walk/walk-1.png",
    };

    for (const char* path : paths) {
        EXPECT_TRUE(std::filesystem::exists(root + path)) << path;
    }
}
