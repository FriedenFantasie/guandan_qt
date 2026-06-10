#include "AiPlayer.h"
#include "Card.h"
#include "GameEngine.h"
#include "HandAnalyzer.h"

#include <array>
#include <cassert>
#include <iostream>
#include <set>

using namespace guandan;

namespace {

Card c(int id, Suit suit, Rank rank)
{
    return Card{ id, suit, rank, 0 };
}

std::set<int> idSet(const std::vector<int>& ids)
{
    return { ids.begin(), ids.end() };
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

void testSmartArrangeGroupsCombinations()
{
    const Rank level = Rank::Two;
    std::vector<Card> hand = {
        c(20, Suit::Diamonds, Rank::Ace),
        c(8, Suit::Clubs, Rank::Three),
        c(3, Suit::Spades, Rank::Eight),
        c(13, Suit::Spades, Rank::Jack),
        c(1, Suit::Spades, Rank::Six),
        c(15, Suit::Clubs, Rank::Jack),
        c(10, Suit::Diamonds, Rank::Four),
        c(18, Suit::Spades, Rank::King),
        c(5, Suit::Spades, Rank::Ten),
        c(7, Suit::Hearts, Rank::Three),
        c(14, Suit::Hearts, Rank::Jack),
        c(9, Suit::Clubs, Rank::Four),
        c(2, Suit::Spades, Rank::Seven),
        c(17, Suit::Diamonds, Rank::King),
        c(4, Suit::Spades, Rank::Nine),
        c(19, Suit::Joker, Rank::SmallJoker),
        c(11, Suit::Hearts, Rank::Four),
        c(6, Suit::Diamonds, Rank::Three)
    };

    const ArrangedHand arrangedHand = HandAnalyzer::arrangeHandWithGroups(hand, level);
    const std::vector<Card>& arranged = arrangedHand.cards;
    assert(arranged.size() == hand.size());
    assert(arrangedHand.groups.size() >= 3);

    std::set<int> originalIds;
    std::set<int> arrangedIds;
    for (const Card& card : hand) {
        originalIds.insert(card.id);
    }
    for (const Card& card : arranged) {
        arrangedIds.insert(card.id);
    }
    assert(originalIds == arrangedIds);

    std::vector<Card> firstGroup(arranged.begin(), arranged.begin() + 5);
    std::vector<Card> secondGroup(arranged.begin() + 5, arranged.begin() + 11);
    std::vector<Card> thirdGroup(arranged.begin() + 11, arranged.begin() + 16);

    assert(HandAnalyzer::analyze(firstGroup, level).type == HandType::StraightFlush);
    assert(HandAnalyzer::analyze(secondGroup, level).type == HandType::ConsecutiveTriples);
    assert(HandAnalyzer::analyze(thirdGroup, level).type == HandType::TripleWithPair);
    assert(arrangedHand.groups[0].analysis.type == HandType::StraightFlush);
    assert(arrangedHand.groups[1].analysis.type == HandType::ConsecutiveTriples);
    assert(arrangedHand.groups[2].analysis.type == HandType::TripleWithPair);
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

void testPlayPreservesCurrentHandOrder()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);
    engine.arrangeCurrentPlayerHand();

    const int playerId = engine.currentPlayer();
    const std::vector<Card> arranged = engine.player(playerId).hand;
    assert(!arranged.empty());

    const int playedId = arranged.front().id;
    std::vector<int> expectedIds;
    for (std::size_t i = 1; i < arranged.size(); ++i) {
        expectedIds.push_back(arranged[i].id);
    }

    std::string error;
    assert(engine.playSelectedCards(playerId, { playedId }, &error));

    const std::vector<Card>& remaining = engine.player(playerId).hand;
    assert(remaining.size() == expectedIds.size());
    for (std::size_t i = 0; i < remaining.size(); ++i) {
        assert(remaining[i].id == expectedIds[i]);
    }
}

void testAiLeadsUsefulCombinations()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);

    const int playerId = engine.currentPlayer();
    std::array<Player, 4>& players = const_cast<std::array<Player, 4>&>(engine.players());
    players[static_cast<std::size_t>(playerId)].hand = {
        c(300, Suit::Spades, Rank::Seven),
        c(301, Suit::Hearts, Rank::Seven),
        c(302, Suit::Clubs, Rank::Seven),
        c(303, Suit::Spades, Rank::Four),
        c(304, Suit::Hearts, Rank::Four),
        c(305, Suit::Diamonds, Rank::Ace),
        c(306, Suit::Clubs, Rank::King),
        c(307, Suit::Diamonds, Rank::Nine)
    };

    const AiDecision decision = AiPlayer::choose(engine, playerId);
    assert(!decision.pass);

    std::vector<Card> chosen;
    for (int id : decision.cardIds) {
        for (const Card& card : players[static_cast<std::size_t>(playerId)].hand) {
            if (card.id == id) {
                chosen.push_back(card);
            }
        }
    }

    const HandAnalysis analysis = HandAnalyzer::analyze(chosen, engine.currentLevel());
    assert(analysis.type == HandType::TripleWithPair);
}

void testAiPreservesStrongCombinationsWhenResponding()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);

    const int leader = engine.currentPlayer();
    const int responder = (leader + 1) % 4;
    std::array<Player, 4>& players = const_cast<std::array<Player, 4>&>(engine.players());
    players[static_cast<std::size_t>(leader)].hand = {
        c(400, Suit::Spades, Rank::Four),
        c(401, Suit::Hearts, Rank::Four),
        c(402, Suit::Clubs, Rank::Three)
    };
    players[static_cast<std::size_t>(responder)].hand = {
        c(410, Suit::Spades, Rank::Five),
        c(411, Suit::Hearts, Rank::Five),
        c(412, Suit::Spades, Rank::Six),
        c(413, Suit::Spades, Rank::Seven),
        c(414, Suit::Spades, Rank::Eight),
        c(415, Suit::Spades, Rank::Nine),
        c(416, Suit::Spades, Rank::King),
        c(417, Suit::Hearts, Rank::King)
    };

    std::string error;
    assert(engine.playSelectedCards(leader, { 400, 401 }, &error));
    assert(engine.currentPlayer() == responder);

    const AiDecision decision = AiPlayer::choose(engine, responder);
    assert(!decision.pass);
    assert(idSet(decision.cardIds) == std::set<int>({ 416, 417 }));
}

void testAiBombsOnlyUnderPressure()
{
    GameEngine engine;
    engine.startNewGame(GameMode::LocalFour);

    const int leader = engine.currentPlayer();
    const int responder = (leader + 1) % 4;
    std::array<Player, 4>& players = const_cast<std::array<Player, 4>&>(engine.players());
    players[static_cast<std::size_t>(leader)].hand = {
        c(500, Suit::Spades, Rank::Ace),
        c(501, Suit::Hearts, Rank::Ace),
        c(502, Suit::Clubs, Rank::Three),
        c(503, Suit::Diamonds, Rank::Four),
        c(504, Suit::Spades, Rank::Five),
        c(505, Suit::Hearts, Rank::Six),
        c(506, Suit::Clubs, Rank::Seven)
    };
    players[static_cast<std::size_t>(responder)].hand = {
        c(510, Suit::Spades, Rank::Eight),
        c(511, Suit::Hearts, Rank::Eight),
        c(512, Suit::Clubs, Rank::Eight),
        c(513, Suit::Diamonds, Rank::Eight),
        c(514, Suit::Spades, Rank::Three),
        c(515, Suit::Hearts, Rank::Five),
        c(516, Suit::Clubs, Rank::Seven),
        c(517, Suit::Diamonds, Rank::Nine)
    };

    std::string error;
    assert(engine.playSelectedCards(leader, { 500, 501 }, &error));
    assert(engine.currentPlayer() == responder);

    const AiDecision patientDecision = AiPlayer::choose(engine, responder);
    assert(patientDecision.pass);

    GameEngine urgentEngine;
    urgentEngine.startNewGame(GameMode::LocalFour);
    const int urgentLeader = urgentEngine.currentPlayer();
    const int urgentResponder = (urgentLeader + 1) % 4;
    std::array<Player, 4>& urgentPlayers = const_cast<std::array<Player, 4>&>(urgentEngine.players());
    urgentPlayers[static_cast<std::size_t>(urgentLeader)].hand = {
        c(520, Suit::Spades, Rank::Ace),
        c(521, Suit::Hearts, Rank::Ace),
        c(522, Suit::Clubs, Rank::Three)
    };
    urgentPlayers[static_cast<std::size_t>(urgentResponder)].hand = players[static_cast<std::size_t>(responder)].hand;

    assert(urgentEngine.playSelectedCards(urgentLeader, { 520, 521 }, &error));
    assert(urgentEngine.currentPlayer() == urgentResponder);

    const AiDecision urgentDecision = AiPlayer::choose(urgentEngine, urgentResponder);
    assert(!urgentDecision.pass);
    assert(idSet(urgentDecision.cardIds) == std::set<int>({ 510, 511, 512, 513 }));
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
    testSmartArrangeGroupsCombinations();
    testEngineDealAndAiLegality();
    testPlayPreservesCurrentHandOrder();
    testAiLeadsUsefulCombinations();
    testAiPreservesStrongCombinationsWhenResponding();
    testAiBombsOnlyUnderPressure();
    testFullDealCanFinish();
    std::cout << "All Guandan rule tests passed.\n";
    return 0;
}
