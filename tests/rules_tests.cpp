#include "Card.h"
#include "GameEngine.h"
#include "HandAnalyzer.h"

#include <cassert>
#include <iostream>
#include <set>

using namespace guandan;

namespace {

Card c(int id, Suit suit, Rank rank)
{
    return Card{ id, suit, rank, 0 };
}

void testDeck()
{
    const std::vector<Card> deck = makeTwoDecks();
    assert(deck.size() == 108);
    std::set<int> ids;
    for (const Card& card : deck) {
        ids.insert(card.id);
    }
    assert(ids.size() == 108);
}

void testBasicHands()
{
    const Rank level = Rank::Two;
    auto straight = HandAnalyzer::analyze({
        c(1, Suit::Spades, Rank::Ten),
        c(2, Suit::Clubs, Rank::Jack),
        c(3, Suit::Hearts, Rank::Queen),
        c(4, Suit::Diamonds, Rank::King),
        c(5, Suit::Spades, Rank::Ace) }, level);
    assert(straight.type == HandType::Straight);
    assert(straight.mainValue == 14);

    auto lowStraight = HandAnalyzer::analyze({
        c(6, Suit::Spades, Rank::Ace),
        c(7, Suit::Clubs, Rank::Two),
        c(8, Suit::Hearts, Rank::Three),
        c(9, Suit::Diamonds, Rank::Four),
        c(10, Suit::Spades, Rank::Five) }, Rank::Three);
    assert(lowStraight.type == HandType::Straight);
    assert(lowStraight.mainValue == 5);

    auto flush = HandAnalyzer::analyze({
        c(11, Suit::Spades, Rank::Six),
        c(12, Suit::Spades, Rank::Seven),
        c(13, Suit::Spades, Rank::Eight),
        c(14, Suit::Spades, Rank::Nine),
        c(15, Suit::Spades, Rank::Ten) }, level);
    assert(flush.type == HandType::StraightFlush);

    auto pairs = HandAnalyzer::analyze({
        c(16, Suit::Spades, Rank::Four), c(17, Suit::Hearts, Rank::Four),
        c(18, Suit::Spades, Rank::Five), c(19, Suit::Hearts, Rank::Five),
        c(20, Suit::Spades, Rank::Six), c(21, Suit::Hearts, Rank::Six) }, level);
    assert(pairs.type == HandType::ConsecutivePairs);

    auto triples = HandAnalyzer::analyze({
        c(22, Suit::Spades, Rank::Seven), c(23, Suit::Hearts, Rank::Seven), c(24, Suit::Clubs, Rank::Seven),
        c(25, Suit::Spades, Rank::Eight), c(26, Suit::Hearts, Rank::Eight), c(27, Suit::Clubs, Rank::Eight) }, level);
    assert(triples.type == HandType::ConsecutiveTriples);
}

void testWildcards()
{
    const Rank level = Rank::Nine;
    auto bomb = HandAnalyzer::analyze({
        c(1, Suit::Hearts, Rank::Nine),
        c(2, Suit::Hearts, Rank::Nine),
        c(3, Suit::Spades, Rank::Ace),
        c(4, Suit::Clubs, Rank::Ace),
        c(5, Suit::Diamonds, Rank::Ace),
        c(6, Suit::Hearts, Rank::Ace) }, level);
    assert(bomb.type == HandType::Bomb);
    assert(bomb.bombSize == 6);

    auto straightFlush = HandAnalyzer::analyze({
        c(7, Suit::Spades, Rank::Ten),
        c(8, Suit::Spades, Rank::Jack),
        c(9, Suit::Hearts, Rank::Nine),
        c(10, Suit::Spades, Rank::King),
        c(11, Suit::Spades, Rank::Ace) }, level);
    assert(straightFlush.type == HandType::StraightFlush);
}

void testComparison()
{
    const Rank level = Rank::Two;
    auto pair4 = HandAnalyzer::analyze({ c(1, Suit::Spades, Rank::Four), c(2, Suit::Hearts, Rank::Four) }, level);
    auto pair5 = HandAnalyzer::analyze({ c(3, Suit::Spades, Rank::Five), c(4, Suit::Hearts, Rank::Five) }, level);
    assert(HandAnalyzer::canBeat(pair5, pair4));

    auto bomb4 = HandAnalyzer::analyze({
        c(5, Suit::Spades, Rank::Six), c(6, Suit::Hearts, Rank::Six),
        c(7, Suit::Clubs, Rank::Six), c(8, Suit::Diamonds, Rank::Six) }, level);
    assert(HandAnalyzer::canBeat(bomb4, pair5));

    auto straightFlush = HandAnalyzer::analyze({
        c(9, Suit::Spades, Rank::Seven), c(10, Suit::Spades, Rank::Eight),
        c(11, Suit::Spades, Rank::Nine), c(12, Suit::Spades, Rank::Ten),
        c(13, Suit::Spades, Rank::Jack) }, level);
    assert(HandAnalyzer::canBeat(straightFlush, bomb4));

    auto jokerBomb = HandAnalyzer::analyze({
        c(14, Suit::Joker, Rank::SmallJoker), c(15, Suit::Joker, Rank::SmallJoker),
        c(16, Suit::Joker, Rank::BigJoker), c(17, Suit::Joker, Rank::BigJoker) }, level);
    assert(HandAnalyzer::canBeat(jokerBomb, straightFlush));
}

void testEngineDealAndAiLegality()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);
    std::size_t total = 0;
    for (const Player& player : engine.players()) {
        assert(player.hand.size() == 27);
        total += player.hand.size();
    }
    assert(total == 108);

    auto hint = engine.hintForCurrentPlayer();
    assert(!hint.empty());

    GameEngine aiEngine;
    aiEngine.startNewGame(GameMode::HumanVsAi);
    assert(aiEngine.player(0).human);
    assert(aiEngine.phase() == GamePhase::Playing || aiEngine.phase() == GamePhase::RoundOver);
}

void testFullDealCanFinish()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);

    for (int step = 0; step < 1000 && engine.phase() == GamePhase::Playing; ++step) {
        const std::vector<int> hint = engine.hintForCurrentPlayer();
        std::string error;
        if (!hint.empty()) {
            assert(engine.playSelectedCards(engine.currentPlayer(), hint, &error));
        } else {
            assert(engine.pass(engine.currentPlayer(), &error));
        }
    }

    assert(engine.phase() == GamePhase::RoundOver);
    assert(engine.finishOrder().size() == 4);
    assert(engine.lastRoundWinningTeam() == engine.finishOrder().front() % 2);
    assert(engine.lastRoundUpgradeAmount() >= 1);
    assert(engine.lastRoundUpgradeAmount() <= 3);
}

} // namespace

int main()
{
    testDeck();
    testBasicHands();
    testWildcards();
    testComparison();
    testEngineDealAndAiLegality();
    testFullDealCanFinish();
    std::cout << "All Guandan rule tests passed.\n";
    return 0;
}
