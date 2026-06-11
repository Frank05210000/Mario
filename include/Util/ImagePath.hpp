#ifndef UTIL_IMAGE_PATH_HPP
#define UTIL_IMAGE_PATH_HPP

#include <string>
#include <vector>

#include "AssetPath.hpp"

/*
 * ImagePath：Player 三套形態（Small / Super / Fire）的圖片路徑常數
 *
 * 全部使用 right 方向的素材，Draw() 裡用 scaleX 翻轉處理左向。
 * 資料夾結構：Resources/Asset/player/<形態>/right/<動作>/
 */

// ─── Small Mario（player/Mario） ─────────────────────────────────────
inline const std::string kSmallDir   = MakeAssetPath("player/Mario/right/");
inline const std::string kSmallIdle  = kSmallDir + "Walk1/Walk1.png"; // 站立用走路第 0 幀
inline const std::string kSmallJump  = kSmallDir + "Jump/Jump.png";
inline const std::string kSmallSkid  = kSmallDir + "Skid/Skid.png";
inline const std::vector<std::string> kSmallWalk = {
    kSmallDir + "Walk1/Walk1-1.png",
    kSmallDir + "Walk1/Walk1-2.png",
    kSmallDir + "Walk1/Walk1-3.png",
};
inline const std::vector<std::string> kSmallClimb = {
    kSmallDir + "Climb/Climb-1.png",
    kSmallDir + "Climb/Climb-2.png",
};

// ─── Super Mario（player/Super Mario） ───────────────────────────────
inline const std::string kSuperDir   = MakeAssetPath("player/Super Mario/right/");
inline const std::string kSuperIdle  = kSuperDir + "Walk1/Walk1.png";
inline const std::string kSuperJump  = kSuperDir + "Jump/Jump.png";
inline const std::string kSuperSkid  = kSuperDir + "Skid/Skid.png";
inline const std::vector<std::string> kSuperWalk = {
    kSuperDir + "Walk1/Walk1-1.png",
    kSuperDir + "Walk1/Walk1-2.png",
    kSuperDir + "Walk1/Walk1-3.png",
};
inline const std::vector<std::string> kSuperClimb = {
    kSuperDir + "Climb/Climb-1.png",
    kSuperDir + "Climb/Climb-2.png",
};

// ─── Fire Mario（player/Fiery Mario） ────────────────────────────────
inline const std::string kFireDir   = MakeAssetPath("player/Fiery Mario/right/");
inline const std::string kFireIdle  = kFireDir + "Walk1/Walk1.png";
inline const std::string kFireJump  = kFireDir + "Jump/Jump.png";
inline const std::string kFireSkid  = kFireDir + "Skid/Skid.png";
inline const std::string kFireShoot = kFireDir + "Shoot/Shoot.png";
inline const std::vector<std::string> kFireWalk = {
    kFireDir + "Walk1/Walk1-1.png",
    kFireDir + "Walk1/Walk1-2.png",
    kFireDir + "Walk1/Walk1-3.png",
};
inline const std::vector<std::string> kFireClimb = {
    kFireDir + "Climb/Climb-1.png",
    kFireDir + "Climb/Climb-2.png",
};

#endif
