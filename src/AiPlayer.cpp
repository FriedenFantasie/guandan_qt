#include "AiPlayer.h"

#include "GameEngine.h"
#include "HandAnalyzer.h"

#include <algorithm>
#include <optional>

namespace guandan {
namespace {

std::vector<int> idsFromCards(const std::vector<Card>& cards)
{
    std::vector<int> ids;
    ids.reserve(cards.size());
    for (const Card& card : cards) {
        ids.push_back(card.id);
    }
    return ids;
}

bool opponentNearlyOut(const GameEngine& engine, int playerId)
{
    for (const Player& player : engine.players()) {
        if (player.id == playerId || sameTeam(player.id, playerId) || player.finished) {
            continue;
        }
        if (player.hand.size() <= 3) {
            return true;
        }
    }
    return false;
}

} // namespace

AiDecision AiPlayer::choose(const GameEngine& engine, int playerId)
{
    const Player& player = engine.player(playerId);
    const PlayedTrick& last = engine.lastPlay();
    const bool newTrick = last.empty();
    const bool teammateControls = !newTrick && sameTeam(last.player, playerId);
    const bool urgent = opponentNearlyOut(engine, playerId);

    const std::optional<HandAnalysis> previous =
        newTrick ? std::nullopt : std::optional<HandAnalysis>(last.analysis);
    const std::vector<CandidatePlay> plays =
        HandAnalyzer::generateCandidatePlays(player.hand, engine.currentLevel(), previous);

    if (plays.empty()) {
        return { true, {} };
    }

    for (const CandidatePlay& play : plays) {
        if (play.cards.size() == player.hand.size()) {
            return { false, idsFromCards(play.cards) };
        }
    }

    if (teammateControls && !urgent) {
        return { true, {} };
    }

    for (const CandidatePlay& play : plays) {
        if (!play.analysis.isBombLike()) {
            return { false, idsFromCards(play.cards) };
        }
    }

    if (newTrick || urgent || player.hand.size() <= 6) {
        return { false, idsFromCards(plays.front().cards) };
    }

    return { true, {} };
}

} // namespace guandan

