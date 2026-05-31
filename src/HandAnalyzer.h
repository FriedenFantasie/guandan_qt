#pragma once

#include "Card.h"

#include <optional>
#include <string>
#include <vector>

namespace guandan {

enum class HandType {
    Invalid,
    Single,
    Pair,
    Triple,
    Straight,
    ConsecutivePairs,
    ConsecutiveTriples,
    TripleWithPair,
    Bomb,
    StraightFlush,
    JokerBomb
};

struct HandAnalysis {
    HandType type = HandType::Invalid;
    int mainValue = 0;
    int cardCount = 0;
    int sequenceLength = 0;
    int bombSize = 0;
    std::vector<Card> cards;

    bool valid() const { return type != HandType::Invalid; }
    bool isBombLike() const;
    std::string typeName() const;
};

struct CandidatePlay {
    std::vector<Card> cards;
    HandAnalysis analysis;
};

class HandAnalyzer {
public:
    static HandAnalysis analyze(const std::vector<Card>& cards, Rank level);
    static bool canBeat(const HandAnalysis& candidate, const HandAnalysis& previous);
    static int bombPower(const HandAnalysis& analysis);

    static std::vector<CandidatePlay> generateCandidatePlays(
        const std::vector<Card>& hand,
        Rank level,
        const std::optional<HandAnalysis>& previous = std::nullopt);
    static std::vector<Card> arrangeHand(const std::vector<Card>& hand, Rank level);

private:
    static HandAnalysis analyzeWithoutWildcards(const std::vector<Card>& cards, Rank level);
};

} // namespace guandan
