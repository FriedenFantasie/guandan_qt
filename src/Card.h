#pragma once

#include <array>
#include <string>
#include <vector>

namespace guandan {

enum class Suit {
    Spades,
    Hearts,
    Clubs,
    Diamonds,
    Joker
};

enum class Rank {
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    Six = 6,
    Seven = 7,
    Eight = 8,
    Nine = 9,
    Ten = 10,
    Jack = 11,
    Queen = 12,
    King = 13,
    Ace = 14,
    SmallJoker = 16,
    BigJoker = 17
};

struct Card {
    int id = -1;
    Suit suit = Suit::Spades;
    Rank rank = Rank::Two;
    int deck = 0;
};

bool operator==(const Card& left, const Card& right);
bool operator!=(const Card& left, const Card& right);

std::vector<Card> makeTwoDecks();
std::array<Rank, 13> ordinaryRanks();

bool isJoker(const Card& card);
bool isWildCard(const Card& card, Rank level);
bool sameTeam(int playerA, int playerB);

int rankOrderValue(Rank rank, Rank level);
int naturalRankValue(Rank rank);
Rank nextLevel(Rank rank);

std::string rankText(Rank rank);
std::string suitText(Suit suit);
std::string cardText(const Card& card);
std::string levelText(Rank rank);

void sortHand(std::vector<Card>& cards, Rank level);

} // namespace guandan

