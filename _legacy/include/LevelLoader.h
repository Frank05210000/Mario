#pragma once

#include <string>

#include "LevelTypes.h"

class LevelLoader {
public:
    LevelData LoadFromFile(const std::string& path) const;
    static ObjectType ParseObjectType(const std::string& type);
};
