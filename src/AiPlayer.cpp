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
        if (player.hand.size() <= 4) {
            return true;
        }
    }
    return false;
}

int leadPriority(HandType type)
{
    switch (type) {
    case HandType::ConsecutiveTriples: return 900;
    case HandType::TripleWithPair: return 860;
    case HandType::ConsecutivePairs: return 790;
    case HandType::Straight: return 740;
    case HandType::Triple: return 620;
    case HandType::Pair: return 480;
    case HandType::Single: return 100;
    case HandType::StraightFlush: return 60;
    case HandType::Bomb: return 50;
    case HandType::JokerBomb: return 40;
    case HandType::Invalid: return 0;
    }
    return 0;
}

bool betterLead(const CandidatePlay& left, const CandidatePlay& right)
{
    const int lp = leadPriority(left.analysis.type);
    const int rp = leadPriority(right.analysis.type);
    if (lp != rp) {
        return lp > rp;
    }
    if (left.analysis.cardCount != right.analysis.cardCount) {
        return left.analysis.cardCount > right.analysis.cardCount;
    }
    if (left.analysis.sequenceLength != right.analysis.sequenceLength) {
        return left.analysis.sequenceLength > right.analysis.sequenceLength;
    }
    if (left.analysis.mainValue != right.analysis.mainValue) {
        return left.analysis.mainValue < right.analysis.mainValue;
    }
    return HandAnalyzer::bombPower(left.analysis) < HandAnalyzer::bombPower(right.analysis);
}

std::optional<CandidatePlay> chooseLeadFromArrangedHand(const Player& player, Rank level, bool urgent)
{
    const ArrangedHand arranged = HandAnalyzer::arrangeHandWithGroups(player.hand, level);
    const bool allowBombLike = urgent || player.hand.size() <= 6;
    std::optional<CandidatePlay> best;
    std::optional<CandidatePlay> bestSingle;

    for (const ArrangedGroup& group : arranged.groups) {
        if (group.cardCount <= 0 ||
            group.startIndex < 0 ||
            group.startIndex + group.cardCount > static_cast<int>(arranged.cards.size())) {
            continue;
        }

        CandidatePlay play;
        play.cards.assign(arranged.cards.begin() + group.startIndex,
                          arranged.cards.begin() + group.startIndex + group.cardCount);
        play.analysis = group.analysis;
        if (!play.analysis.valid()) {
            continue;
        }
        if (play.analysis.isBombLike() && !allowBombLike) {
            continue;
        }
        if (play.analysis.type == HandType::Single) {
            if (!bestSingle || betterLead(play, *bestSingle)) {
                bestSingle = play;
            }
            continue;
        }
        if (!best || betterLead(play, *best)) {
            best = play;
        }
    }

    if (best) {
        return best;
    }
    return bestSingle;
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

    if (newTrick) {
        if (const std::optional<CandidatePlay> lead = chooseLeadFromArrangedHand(player, engine.currentLevel(), urgent)) {
            return { false, idsFromCards(lead->cards) };
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
