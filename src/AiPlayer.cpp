#include "AiPlayer.h"

#include "GameEngine.h"
#include "HandAnalyzer.h"

#include <algorithm>
#include <limits>
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

bool teammateNearlyOut(const GameEngine& engine, int playerId)
{
    for (const Player& player : engine.players()) {
        if (player.id == playerId || !sameTeam(player.id, playerId) || player.finished) {
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

int planValue(const HandAnalysis& analysis)
{
    switch (analysis.type) {
    case HandType::JokerBomb:
        return 9800;
    case HandType::StraightFlush:
        return 7600 + analysis.mainValue * 8;
    case HandType::Bomb:
        return 5200 + analysis.bombSize * 360 + analysis.mainValue * 6;
    case HandType::ConsecutiveTriples:
        return 3000 + analysis.sequenceLength * 260 + analysis.mainValue * 3;
    case HandType::TripleWithPair:
        return 2600 + analysis.mainValue * 4;
    case HandType::ConsecutivePairs:
        return 2150 + analysis.sequenceLength * 170 + analysis.mainValue * 3;
    case HandType::Straight:
        return 1900 + analysis.mainValue * 3;
    case HandType::Triple:
        return 980 + analysis.mainValue * 5;
    case HandType::Pair:
        return 430 + analysis.mainValue * 4;
    case HandType::Single:
        return 80 + analysis.mainValue;
    case HandType::Invalid:
        return 0;
    }
    return 0;
}

bool containsCardId(const std::vector<Card>& cards, int id)
{
    for (const Card& card : cards) {
        if (card.id == id) {
            return true;
        }
    }
    return false;
}

std::vector<Card> remainingAfterPlay(const std::vector<Card>& hand, const CandidatePlay& play)
{
    std::vector<Card> remaining;
    remaining.reserve(hand.size() - play.cards.size());
    for (const Card& card : hand) {
        if (!containsCardId(play.cards, card.id)) {
            remaining.push_back(card);
        }
    }
    return remaining;
}

int remainingPlanScore(const std::vector<Card>& remaining, Rank level)
{
    if (remaining.empty()) {
        return 100000;
    }

    const ArrangedHand arranged = HandAnalyzer::arrangeHandWithGroups(remaining, level);
    int score = 0;
    int groupedCards = 0;
    for (const ArrangedGroup& group : arranged.groups) {
        if (!group.analysis.valid()) {
            continue;
        }
        groupedCards += group.cardCount;
        score += planValue(group.analysis);
        score += group.cardCount * 34;
        if (group.analysis.type == HandType::Single) {
            score -= 95;
        }
    }

    const int looseCards = static_cast<int>(remaining.size()) - groupedCards;
    score -= static_cast<int>(arranged.groups.size()) * 55;
    score -= looseCards * 130;
    score -= static_cast<int>(remaining.size()) * 18;
    return score;
}

bool canSpendBombLike(
    const CandidatePlay& play,
    const Player& player,
    bool newTrick,
    bool urgent,
    bool teammateControls)
{
    if (!play.analysis.isBombLike()) {
        return true;
    }
    if (play.cards.size() == player.hand.size()) {
        return true;
    }
    if (teammateControls && player.hand.size() > 6) {
        return false;
    }
    if (newTrick) {
        return urgent || player.hand.size() <= 6;
    }
    return urgent || player.hand.size() <= 6;
}

int leadScore(const Player& player, const CandidatePlay& play, Rank level, bool urgent, bool partnerClose)
{
    const std::vector<Card> remaining = remainingAfterPlay(player.hand, play);
    int score = remainingPlanScore(remaining, level);
    score += leadPriority(play.analysis.type) * 9;
    score += static_cast<int>(play.cards.size()) * 300;
    score -= play.analysis.mainValue * 7;

    if (partnerClose) {
        score -= static_cast<int>(play.cards.size()) * 85;
        score -= play.analysis.mainValue * 6;
    }

    if (play.analysis.isBombLike()) {
        score -= 7800 + HandAnalyzer::bombPower(play.analysis);
        if (urgent || player.hand.size() <= 6) {
            score += 4300;
        }
    }
    if (remaining.size() <= 2) {
        score += 1500;
    }
    return score;
}

int responseScore(const Player& player, const CandidatePlay& play, Rank level, bool urgent)
{
    const std::vector<Card> remaining = remainingAfterPlay(player.hand, play);
    int score = remainingPlanScore(remaining, level);
    score += static_cast<int>(play.cards.size()) * 130;
    score -= play.analysis.mainValue * 18;

    if (play.analysis.isBombLike()) {
        score -= 9000 + HandAnalyzer::bombPower(play.analysis);
        if (urgent) {
            score += 6500;
        }
        if (player.hand.size() <= 6) {
            score += 2800;
        }
    }
    if (remaining.empty()) {
        score += 120000;
    } else if (remaining.size() <= 2) {
        score += 2200;
    } else if (remaining.size() <= 5) {
        score += 750;
    }
    return score;
}

bool betterByScore(const CandidatePlay& left, int leftScore, const CandidatePlay& right, int rightScore)
{
    if (leftScore != rightScore) {
        return leftScore > rightScore;
    }
    if (left.analysis.isBombLike() != right.analysis.isBombLike()) {
        return !left.analysis.isBombLike();
    }
    if (left.cards.size() != right.cards.size()) {
        return left.cards.size() > right.cards.size();
    }
    if (left.analysis.mainValue != right.analysis.mainValue) {
        return left.analysis.mainValue < right.analysis.mainValue;
    }
    return HandAnalyzer::bombPower(left.analysis) < HandAnalyzer::bombPower(right.analysis);
}

std::optional<CandidatePlay> chooseLeadFromArrangedHand(
    const Player& player,
    Rank level,
    bool urgent,
    bool partnerClose)
{
    const ArrangedHand arranged = HandAnalyzer::arrangeHandWithGroups(player.hand, level);
    std::optional<CandidatePlay> best;
    int bestScore = std::numeric_limits<int>::min();

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
        if (!canSpendBombLike(play, player, true, urgent, false)) {
            continue;
        }

        const int score = leadScore(player, play, level, urgent, partnerClose);
        if (!best || betterByScore(play, score, *best, bestScore)) {
            best = play;
            bestScore = score;
        }
    }

    return best;
}

std::optional<CandidatePlay> chooseBestResponse(
    const Player& player,
    Rank level,
    const std::vector<CandidatePlay>& plays,
    bool urgent,
    bool teammateControls)
{
    std::optional<CandidatePlay> best;
    int bestScore = std::numeric_limits<int>::min();

    for (const CandidatePlay& play : plays) {
        if (!canSpendBombLike(play, player, false, urgent, teammateControls)) {
            continue;
        }

        const int score = responseScore(player, play, level, urgent);
        if (!best || betterByScore(play, score, *best, bestScore)) {
            best = play;
            bestScore = score;
        }
    }

    return best;
}

} // namespace

AiDecision AiPlayer::choose(const GameEngine& engine, int playerId)
{
    const Player& player = engine.player(playerId);
    const PlayedTrick& last = engine.lastPlay();
    const bool newTrick = last.empty();
    const bool teammateControls = !newTrick && sameTeam(last.player, playerId);
    const bool urgent = opponentNearlyOut(engine, playerId);
    const bool partnerClose = teammateNearlyOut(engine, playerId);

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
        if (const std::optional<CandidatePlay> lead =
                chooseLeadFromArrangedHand(player, engine.currentLevel(), urgent, partnerClose)) {
            return { false, idsFromCards(lead->cards) };
        }
    }

    if (teammateControls && (!urgent || partnerClose)) {
        return { true, {} };
    }

    if (const std::optional<CandidatePlay> response =
            chooseBestResponse(player, engine.currentLevel(), plays, urgent, teammateControls)) {
        return { false, idsFromCards(response->cards) };
    }

    return { true, {} };
}

} // namespace guandan
