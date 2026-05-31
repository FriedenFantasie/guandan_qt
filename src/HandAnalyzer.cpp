#include "HandAnalyzer.h"

#include <algorithm>
#include <climits>
#include <map>
#include <set>
#include <sstream>
#include <unordered_set>

namespace guandan {
namespace {

bool isOrdinaryRank(Rank rank)
{
    return static_cast<int>(rank) >= 2 && static_cast<int>(rank) <= 14;
}

std::string cardsKey(const std::vector<Card>& cards)
{
    std::vector<int> ids;
    ids.reserve(cards.size());
    for (const Card& card : cards) {
        ids.push_back(card.id);
    }
    std::sort(ids.begin(), ids.end());

    std::ostringstream out;
    for (int id : ids) {
        out << id << '-';
    }
    return out.str();
}

bool allSameSuit(const std::vector<Card>& cards)
{
    if (cards.empty()) {
        return false;
    }
    const Suit suit = cards.front().suit;
    if (suit == Suit::Joker) {
        return false;
    }
    for (const Card& card : cards) {
        if (card.suit != suit || card.suit == Suit::Joker) {
            return false;
        }
    }
    return true;
}

std::optional<int> fiveCardSequenceHigh(const std::vector<Rank>& ranks)
{
    if (ranks.size() != 5) {
        return std::nullopt;
    }

    std::vector<int> values;
    values.reserve(5);
    for (Rank rank : ranks) {
        if (!isOrdinaryRank(rank)) {
            return std::nullopt;
        }
        values.push_back(static_cast<int>(rank));
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    if (values.size() != 5) {
        return std::nullopt;
    }

    if (values == std::vector<int>{ 2, 3, 4, 5, 14 }) {
        return 5;
    }
    if (values.back() - values.front() == 4) {
        return values.back();
    }
    return std::nullopt;
}

std::optional<int> consecutiveHigh(const std::vector<Rank>& ranks)
{
    if (ranks.empty()) {
        return std::nullopt;
    }

    std::vector<int> values;
    values.reserve(ranks.size());
    for (Rank rank : ranks) {
        if (!isOrdinaryRank(rank)) {
            return std::nullopt;
        }
        values.push_back(static_cast<int>(rank));
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    if (values.size() != ranks.size()) {
        return std::nullopt;
    }

    for (std::size_t i = 1; i < values.size(); ++i) {
        if (values[i] != values[i - 1] + 1) {
            return std::nullopt;
        }
    }
    return values.back();
}

int typePreference(HandType type)
{
    switch (type) {
    case HandType::Invalid: return 0;
    case HandType::Single: return 10;
    case HandType::Pair: return 20;
    case HandType::Triple: return 30;
    case HandType::Straight: return 40;
    case HandType::ConsecutivePairs: return 50;
    case HandType::ConsecutiveTriples: return 60;
    case HandType::TripleWithPair: return 70;
    case HandType::Bomb: return 200;
    case HandType::StraightFlush: return 240;
    case HandType::JokerBomb: return 400;
    }
    return 0;
}

int choiceStrength(const HandAnalysis& analysis)
{
    if (!analysis.valid()) {
        return 0;
    }
    if (analysis.isBombLike()) {
        return 100000 + HandAnalyzer::bombPower(analysis);
    }
    return typePreference(analysis.type) * 1000 + analysis.sequenceLength * 100 + analysis.mainValue;
}

bool analysisLessForChoice(const HandAnalysis& left, const HandAnalysis& right)
{
    return choiceStrength(left) < choiceStrength(right);
}

std::vector<Rank> ranksFromCounts(const std::map<Rank, int>& counts, int expectedCount)
{
    std::vector<Rank> ranks;
    for (const auto& [rank, count] : counts) {
        if (count != expectedCount) {
            return {};
        }
        ranks.push_back(rank);
    }
    return ranks;
}

std::vector<Card> wildCardsIn(const std::vector<Card>& hand, Rank level)
{
    std::vector<Card> wilds;
    for (const Card& card : hand) {
        if (isWildCard(card, level)) {
            wilds.push_back(card);
        }
    }
    return wilds;
}

bool idUsed(const std::vector<Card>& cards, int id)
{
    for (const Card& card : cards) {
        if (card.id == id) {
            return true;
        }
    }
    return false;
}

std::optional<std::vector<Card>> takeRank(
    const std::vector<Card>& hand,
    Rank rank,
    int count,
    Rank level,
    const std::vector<Card>& alreadyUsed = {})
{
    std::vector<Card> result;
    for (const Card& card : hand) {
        if (idUsed(alreadyUsed, card.id)) {
            continue;
        }
        if (!isWildCard(card, level) && card.rank == rank) {
            result.push_back(card);
            if (static_cast<int>(result.size()) == count) {
                return result;
            }
        }
    }

    if (!isOrdinaryRank(rank)) {
        return std::nullopt;
    }

    for (const Card& card : hand) {
        if (idUsed(alreadyUsed, card.id) || idUsed(result, card.id)) {
            continue;
        }
        if (isWildCard(card, level)) {
            result.push_back(card);
            if (static_cast<int>(result.size()) == count) {
                return result;
            }
        }
    }

    if (static_cast<int>(result.size()) == count) {
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<Card>> takeRankSuit(
    const std::vector<Card>& hand,
    Rank rank,
    Suit suit,
    Rank level,
    const std::vector<Card>& alreadyUsed = {})
{
    for (const Card& card : hand) {
        if (idUsed(alreadyUsed, card.id)) {
            continue;
        }
        if (!isWildCard(card, level) && card.rank == rank && card.suit == suit) {
            return std::vector<Card>{ card };
        }
    }

    if (!isOrdinaryRank(rank)) {
        return std::nullopt;
    }

    for (const Card& card : hand) {
        if (idUsed(alreadyUsed, card.id)) {
            continue;
        }
        if (isWildCard(card, level)) {
            return std::vector<Card>{ card };
        }
    }
    return std::nullopt;
}

std::vector<std::vector<Rank>> straightSequences()
{
    std::vector<std::vector<Rank>> sequences;
    sequences.push_back({ Rank::Ace, Rank::Two, Rank::Three, Rank::Four, Rank::Five });
    const auto ranks = ordinaryRanks();
    for (int start = 2; start <= 10; ++start) {
        std::vector<Rank> sequence;
        for (int value = start; value < start + 5; ++value) {
            sequence.push_back(static_cast<Rank>(value));
        }
        sequences.push_back(sequence);
    }
    return sequences;
}

std::vector<std::vector<Rank>> fixedLengthSequences(int length)
{
    std::vector<std::vector<Rank>> sequences;
    for (int start = 2; start <= 14 - length + 1; ++start) {
        std::vector<Rank> sequence;
        for (int value = start; value < start + length; ++value) {
            sequence.push_back(static_cast<Rank>(value));
        }
        sequences.push_back(sequence);
    }
    return sequences;
}

void addCandidate(
    std::vector<CandidatePlay>& plays,
    std::unordered_set<std::string>& seen,
    const std::vector<Card>& cards,
    Rank level,
    const std::optional<HandAnalysis>& previous)
{
    if (cards.empty()) {
        return;
    }
    const std::string key = cardsKey(cards);
    if (seen.find(key) != seen.end()) {
        return;
    }

    HandAnalysis analysis = HandAnalyzer::analyze(cards, level);
    if (!analysis.valid()) {
        return;
    }
    if (previous && !HandAnalyzer::canBeat(analysis, *previous)) {
        return;
    }

    seen.insert(key);
    plays.push_back({ cards, analysis });
}

bool playLessForAi(const CandidatePlay& left, const CandidatePlay& right)
{
    const bool lb = left.analysis.isBombLike();
    const bool rb = right.analysis.isBombLike();
    if (lb != rb) {
        return !lb;
    }
    if (left.cards.size() != right.cards.size()) {
        return left.cards.size() < right.cards.size();
    }
    if (left.analysis.mainValue != right.analysis.mainValue) {
        return left.analysis.mainValue < right.analysis.mainValue;
    }
    return HandAnalyzer::bombPower(left.analysis) < HandAnalyzer::bombPower(right.analysis);
}

int arrangePriority(HandType type)
{
    switch (type) {
    case HandType::JokerBomb: return 1000;
    case HandType::StraightFlush: return 950;
    case HandType::Bomb: return 900;
    case HandType::ConsecutiveTriples: return 820;
    case HandType::TripleWithPair: return 780;
    case HandType::ConsecutivePairs: return 720;
    case HandType::Straight: return 680;
    case HandType::Triple: return 520;
    case HandType::Pair: return 420;
    case HandType::Single: return 100;
    case HandType::Invalid: return 0;
    }
    return 0;
}

bool playLessForArrange(const CandidatePlay& left, const CandidatePlay& right)
{
    const int lp = arrangePriority(left.analysis.type);
    const int rp = arrangePriority(right.analysis.type);
    if (lp != rp) {
        return lp > rp;
    }
    if (left.analysis.sequenceLength != right.analysis.sequenceLength) {
        return left.analysis.sequenceLength > right.analysis.sequenceLength;
    }
    if (left.analysis.cardCount != right.analysis.cardCount) {
        return left.analysis.cardCount > right.analysis.cardCount;
    }
    if (left.analysis.mainValue != right.analysis.mainValue) {
        return left.analysis.mainValue < right.analysis.mainValue;
    }
    if (left.analysis.isBombLike() && right.analysis.isBombLike()) {
        return HandAnalyzer::bombPower(left.analysis) > HandAnalyzer::bombPower(right.analysis);
    }
    return cardsKey(left.cards) < cardsKey(right.cards);
}

bool usesAnyCard(const std::vector<Card>& cards, const std::unordered_set<int>& usedIds)
{
    for (const Card& card : cards) {
        if (usedIds.find(card.id) != usedIds.end()) {
            return true;
        }
    }
    return false;
}

std::vector<Card> displayOrderedCards(const CandidatePlay& play, Rank level)
{
    std::vector<Card> cards = play.cards;
    switch (play.analysis.type) {
    case HandType::Straight:
    case HandType::StraightFlush:
    case HandType::ConsecutivePairs:
    case HandType::ConsecutiveTriples:
    case HandType::TripleWithPair:
        return cards;
    default:
        sortHand(cards, level);
        return cards;
    }
}

} // namespace

bool HandAnalysis::isBombLike() const
{
    return type == HandType::Bomb || type == HandType::StraightFlush || type == HandType::JokerBomb;
}

std::string HandAnalysis::typeName() const
{
    switch (type) {
    case HandType::Invalid: return "无效牌型";
    case HandType::Single: return "单张";
    case HandType::Pair: return "对子";
    case HandType::Triple: return "三同张";
    case HandType::Straight: return "顺子";
    case HandType::ConsecutivePairs: return "三连对";
    case HandType::ConsecutiveTriples: return "二连三";
    case HandType::TripleWithPair: return "三带对";
    case HandType::Bomb: return "炸弹";
    case HandType::StraightFlush: return "同花顺";
    case HandType::JokerBomb: return "王炸";
    }
    return "未知";
}

HandAnalysis HandAnalyzer::analyze(const std::vector<Card>& cards, Rank level)
{
    if (cards.empty()) {
        return {};
    }

    std::vector<int> wildIndexes;
    for (std::size_t index = 0; index < cards.size(); ++index) {
        if (isWildCard(cards[index], level)) {
            wildIndexes.push_back(static_cast<int>(index));
        }
    }

    if (wildIndexes.empty()) {
        return analyzeWithoutWildcards(cards, level);
    }

    HandAnalysis best;
    std::vector<Card> substituted = cards;
    const std::array<Suit, 4> suits = { Suit::Spades, Suit::Hearts, Suit::Clubs, Suit::Diamonds };
    const auto ranks = ordinaryRanks();

    auto search = [&](auto&& self, int depth) -> void {
        if (depth == static_cast<int>(wildIndexes.size())) {
            HandAnalysis analysis = analyzeWithoutWildcards(substituted, level);
            if (analysis.valid()) {
                analysis.cards = cards;
            }
            if (analysisLessForChoice(best, analysis)) {
                best = analysis;
            }
            return;
        }

        const int index = wildIndexes[depth];
        for (Rank rank : ranks) {
            for (Suit suit : suits) {
                substituted[index].rank = rank;
                substituted[index].suit = suit;
                self(self, depth + 1);
            }
        }
    };

    search(search, 0);
    if (best.valid()) {
        best.cards = cards;
    }
    return best;
}

HandAnalysis HandAnalyzer::analyzeWithoutWildcards(const std::vector<Card>& cards, Rank level)
{
    HandAnalysis result;
    result.cards = cards;
    result.cardCount = static_cast<int>(cards.size());

    const int n = static_cast<int>(cards.size());
    std::map<Rank, int> counts;
    std::vector<Rank> ranks;
    int smallJokers = 0;
    int bigJokers = 0;
    for (const Card& card : cards) {
        counts[card.rank]++;
        ranks.push_back(card.rank);
        if (card.rank == Rank::SmallJoker) {
            smallJokers++;
        } else if (card.rank == Rank::BigJoker) {
            bigJokers++;
        }
    }

    if (n == 4 && smallJokers == 2 && bigJokers == 2) {
        result.type = HandType::JokerBomb;
        result.mainValue = 100;
        result.bombSize = 4;
        return result;
    }

    if (n == 1) {
        result.type = HandType::Single;
        result.mainValue = rankOrderValue(cards.front().rank, level);
        return result;
    }

    if (counts.size() == 1) {
        const Rank rank = counts.begin()->first;
        const int count = counts.begin()->second;
        if (count == 2) {
            result.type = HandType::Pair;
            result.mainValue = rankOrderValue(rank, level);
            return result;
        }
        if (count == 3) {
            result.type = HandType::Triple;
            result.mainValue = rankOrderValue(rank, level);
            return result;
        }
        if (count >= 4 && !isJoker(cards.front())) {
            result.type = HandType::Bomb;
            result.mainValue = rankOrderValue(rank, level);
            result.bombSize = count;
            return result;
        }
    }

    if (n == 5) {
        if (auto high = fiveCardSequenceHigh(ranks)) {
            result.type = allSameSuit(cards) ? HandType::StraightFlush : HandType::Straight;
            result.mainValue = *high;
            result.sequenceLength = 5;
            result.bombSize = result.type == HandType::StraightFlush ? 5 : 0;
            return result;
        }

        bool hasThree = false;
        bool hasPair = false;
        Rank tripleRank = Rank::Two;
        for (const auto& [rank, count] : counts) {
            if (count == 3) {
                hasThree = true;
                tripleRank = rank;
            } else if (count == 2) {
                hasPair = true;
            }
        }
        if (hasThree && hasPair) {
            result.type = HandType::TripleWithPair;
            result.mainValue = rankOrderValue(tripleRank, level);
            return result;
        }
    }

    if (n >= 6 && n % 2 == 0) {
        const std::vector<Rank> pairRanks = ranksFromCounts(counts, 2);
        if (pairRanks.size() >= 3) {
            if (auto high = consecutiveHigh(pairRanks)) {
                result.type = HandType::ConsecutivePairs;
                result.mainValue = *high;
                result.sequenceLength = static_cast<int>(pairRanks.size());
                return result;
            }
        }
    }

    if (n >= 6 && n % 3 == 0) {
        const std::vector<Rank> tripleRanks = ranksFromCounts(counts, 3);
        if (tripleRanks.size() >= 2) {
            if (auto high = consecutiveHigh(tripleRanks)) {
                result.type = HandType::ConsecutiveTriples;
                result.mainValue = *high;
                result.sequenceLength = static_cast<int>(tripleRanks.size());
                return result;
            }
        }
    }

    return result;
}

int HandAnalyzer::bombPower(const HandAnalysis& analysis)
{
    switch (analysis.type) {
    case HandType::JokerBomb:
        return 2000;
    case HandType::StraightFlush:
        return 450 + analysis.mainValue;
    case HandType::Bomb:
        if (analysis.bombSize <= 4) {
            return 400 + analysis.mainValue;
        }
        return 500 + (analysis.bombSize - 5) * 100 + analysis.mainValue;
    default:
        return 0;
    }
}

bool HandAnalyzer::canBeat(const HandAnalysis& candidate, const HandAnalysis& previous)
{
    if (!candidate.valid()) {
        return false;
    }
    if (!previous.valid()) {
        return true;
    }

    const bool cb = candidate.isBombLike();
    const bool pb = previous.isBombLike();
    if (cb || pb) {
        if (cb && !pb) {
            return true;
        }
        if (!cb && pb) {
            return false;
        }
        return bombPower(candidate) > bombPower(previous);
    }

    if (candidate.type != previous.type) {
        return false;
    }
    if (candidate.cardCount != previous.cardCount) {
        return false;
    }
    if (candidate.sequenceLength != previous.sequenceLength) {
        return false;
    }
    return candidate.mainValue > previous.mainValue;
}

std::vector<CandidatePlay> HandAnalyzer::generateCandidatePlays(
    const std::vector<Card>& hand,
    Rank level,
    const std::optional<HandAnalysis>& previous)
{
    std::vector<CandidatePlay> plays;
    std::unordered_set<std::string> seen;

    for (const Card& card : hand) {
        addCandidate(plays, seen, { card }, level, previous);
    }

    const auto ranks = ordinaryRanks();
    for (Rank rank : ranks) {
        for (int count : { 2, 3 }) {
            if (auto cards = takeRank(hand, rank, count, level)) {
                addCandidate(plays, seen, *cards, level, previous);
            }
        }
        for (int count = 4; count <= 10; ++count) {
            if (auto cards = takeRank(hand, rank, count, level)) {
                addCandidate(plays, seen, *cards, level, previous);
            }
        }
    }

    for (Rank rank : { Rank::SmallJoker, Rank::BigJoker }) {
        for (int count : { 2, 3 }) {
            if (auto cards = takeRank(hand, rank, count, level)) {
                addCandidate(plays, seen, *cards, level, previous);
            }
        }
    }

    std::vector<Card> jokers;
    for (const Card& card : hand) {
        if (isJoker(card)) {
            jokers.push_back(card);
        }
    }
    if (jokers.size() == 4) {
        addCandidate(plays, seen, jokers, level, previous);
    }

    for (Rank tripleRank : ranks) {
        auto triple = takeRank(hand, tripleRank, 3, level);
        if (!triple) {
            continue;
        }
        for (Rank pairRank : ranks) {
            if (pairRank == tripleRank) {
                continue;
            }
            auto pair = takeRank(hand, pairRank, 2, level, *triple);
            if (!pair) {
                continue;
            }
            std::vector<Card> cards = *triple;
            cards.insert(cards.end(), pair->begin(), pair->end());
            addCandidate(plays, seen, cards, level, previous);
        }
    }

    for (const std::vector<Rank>& sequence : straightSequences()) {
        std::vector<Card> cards;
        bool ok = true;
        for (Rank rank : sequence) {
            auto taken = takeRank(hand, rank, 1, level, cards);
            if (!taken) {
                ok = false;
                break;
            }
            cards.insert(cards.end(), taken->begin(), taken->end());
        }
        if (ok) {
            addCandidate(plays, seen, cards, level, previous);
        }
    }

    const std::array<Suit, 4> suits = { Suit::Spades, Suit::Hearts, Suit::Clubs, Suit::Diamonds };
    for (Suit suit : suits) {
        for (const std::vector<Rank>& sequence : straightSequences()) {
            std::vector<Card> cards;
            bool ok = true;
            for (Rank rank : sequence) {
                auto taken = takeRankSuit(hand, rank, suit, level, cards);
                if (!taken) {
                    ok = false;
                    break;
                }
                cards.insert(cards.end(), taken->begin(), taken->end());
            }
            if (ok) {
                addCandidate(plays, seen, cards, level, previous);
            }
        }
    }

    for (const std::vector<Rank>& sequence : fixedLengthSequences(3)) {
        std::vector<Card> cards;
        bool ok = true;
        for (Rank rank : sequence) {
            auto taken = takeRank(hand, rank, 2, level, cards);
            if (!taken) {
                ok = false;
                break;
            }
            cards.insert(cards.end(), taken->begin(), taken->end());
        }
        if (ok) {
            addCandidate(plays, seen, cards, level, previous);
        }
    }

    for (const std::vector<Rank>& sequence : fixedLengthSequences(2)) {
        std::vector<Card> cards;
        bool ok = true;
        for (Rank rank : sequence) {
            auto taken = takeRank(hand, rank, 3, level, cards);
            if (!taken) {
                ok = false;
                break;
            }
            cards.insert(cards.end(), taken->begin(), taken->end());
        }
        if (ok) {
            addCandidate(plays, seen, cards, level, previous);
        }
    }

    std::sort(plays.begin(), plays.end(), playLessForAi);
    return plays;
}

std::vector<Card> HandAnalyzer::arrangeHand(const std::vector<Card>& hand, Rank level)
{
    std::vector<CandidatePlay> plays = generateCandidatePlays(hand, level);
    std::sort(plays.begin(), plays.end(), playLessForArrange);

    std::vector<Card> arranged;
    arranged.reserve(hand.size());
    std::unordered_set<int> usedIds;

    for (const CandidatePlay& play : plays) {
        if (!play.analysis.valid() || usesAnyCard(play.cards, usedIds)) {
            continue;
        }

        std::vector<Card> cards = displayOrderedCards(play, level);
        for (const Card& card : cards) {
            arranged.push_back(card);
            usedIds.insert(card.id);
        }
    }

    std::vector<Card> leftovers;
    for (const Card& card : hand) {
        if (usedIds.find(card.id) == usedIds.end()) {
            leftovers.push_back(card);
        }
    }
    sortHand(leftovers, level);
    arranged.insert(arranged.end(), leftovers.begin(), leftovers.end());
    return arranged;
}

} // namespace guandan
