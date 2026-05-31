#pragma once

#include <vector>

namespace guandan {

class GameEngine;

struct AiDecision {
    bool pass = true;
    std::vector<int> cardIds;
};

class AiPlayer {
public:
    static AiDecision choose(const GameEngine& engine, int playerId);
};

} // namespace guandan

