#include "Card.h"

#include <algorithm>

namespace guandan {

bool operator==(const Card& left, const Card& right)
{
    return left.id == right.id;
}

bool operator!=(const Card& left, const Card& right)
{
    return !(left == right);
}

std::array<Rank, 13> ordinaryRanks()
{
    return { Rank::Two, Rank::Three, Rank::Four, Rank::Five, Rank::Six, Rank::Seven,
             Rank::Eight, Rank::Nine, Rank::Ten, Rank::Jack, Rank::Queen, Rank::King,
             Rank::Ace };
}

std::vector<Card> makeTwoDecks()
{
    std::vector<Card> cards;
    cards.reserve(108);
    int id = 0;
    const std::array<Suit, 4> suits = { Suit::Spades, Suit::Hearts, Suit::Clubs, Suit::Diamonds };

    for (int deck = 0; deck < 2; ++deck) {
        for (Suit suit : suits) {
            for (Rank rank : ordinaryRanks()) {
                cards.push_back({ id++, suit, rank, deck });
            }
        }
        cards.push_back({ id++, Suit::Joker, Rank::SmallJoker, deck });
        cards.push_back({ id++, Suit::Joker, Rank::BigJoker, deck });
    }

    return cards;
}

bool isJoker(const Card& card)
{
    return card.rank == Rank::SmallJoker || card.rank == Rank::BigJoker;
}

bool isWildCard(const Card& card, Rank level)
{
    return card.suit == Suit::Hearts && card.rank == level;
}

bool sameTeam(int playerA, int playerB)
{
    return playerA % 2 == playerB % 2;
}

int naturalRankValue(Rank rank)
{
    return static_cast<int>(rank);
}

int rankOrderValue(Rank rank, Rank level)
{
    if (rank == Rank::BigJoker) {
        return 17;
    }
    if (rank == Rank::SmallJoker) {
        return 16;
    }
    if (rank == level) {
        return 15;
    }
    return static_cast<int>(rank);
}

Rank nextLevel(Rank rank)
{
    switch (rank) {
    case Rank::Two: return Rank::Three;
    case Rank::Three: return Rank::Four;
    case Rank::Four: return Rank::Five;
    case Rank::Five: return Rank::Six;
    case Rank::Six: return Rank::Seven;
    case Rank::Seven: return Rank::Eight;
    case Rank::Eight: return Rank::Nine;
    case Rank::Nine: return Rank::Ten;
    case Rank::Ten: return Rank::Jack;
    case Rank::Jack: return Rank::Queen;
    case Rank::Queen: return Rank::King;
    case Rank::King: return Rank::Ace;
    case Rank::Ace: return Rank::Ace;
    default: return Rank::Two;
    }
}

std::string rankText(Rank rank)
{
    switch (rank) {
    case Rank::Two: return "2";
    case Rank::Three: return "3";
    case Rank::Four: return "4";
    case Rank::Five: return "5";
    case Rank::Six: return "6";
    case Rank::Seven: return "7";
    case Rank::Eight: return "8";
    case Rank::Nine: return "9";
    case Rank::Ten: return "10";
    case Rank::Jack: return "J";
    case Rank::Queen: return "Q";
    case Rank::King: return "K";
    case Rank::Ace: return "A";
    case Rank::SmallJoker: return "SJ";
    case Rank::BigJoker: return "BJ";
    }
    return "?";
}

std::string suitText(Suit suit)
{
    switch (suit) {
    case Suit::Spades: return "S";
    case Suit::Hearts: return "H";
    case Suit::Clubs: return "C";
    case Suit::Diamonds: return "D";
    case Suit::Joker: return "J";
    }
    return "?";
}

std::string cardText(const Card& card)
{
    if (card.rank == Rank::SmallJoker) {
        return "Small Joker";
    }
    if (card.rank == Rank::BigJoker) {
        return "Big Joker";
    }
    return suitText(card.suit) + rankText(card.rank);
}

std::string levelText(Rank rank)
{
    return rankText(rank);
}

void sortHand(std::vector<Card>& cards, Rank level)
{
    std::sort(cards.begin(), cards.end(), [level](const Card& left, const Card& right) {
        const int lv = rankOrderValue(left.rank, level);
        const int rv = rankOrderValue(right.rank, level);
        if (lv != rv) {
            return lv < rv;
        }
        if (left.suit != right.suit) {
            return static_cast<int>(left.suit) < static_cast<int>(right.suit);
        }
        return left.id < right.id;
    });
}

} // namespace guandan

