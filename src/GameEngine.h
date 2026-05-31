#pragma once

#include "Card.h"
#include "HandAnalyzer.h"

#include <array>
#include <string>
#include <vector>

namespace guandan {

enum class GameMode {
    HumanVsAi,
    LocalFour
};

enum class GamePhase {
    Playing,
    RoundOver
};

struct Player {
    int id = 0;
    std::string name;
    bool human = true;
    std::vector<Card> hand;
    bool finished = false;
    int finishPlace = 0;
};

struct PlayedTrick {
    int player = -1;
    std::vector<Card> cards;
    HandAnalysis analysis;

    bool empty() const { return player < 0 || !analysis.valid(); }
};

struct PlayerAction {
    int sequence = 0;
    bool pass = false;
    std::string text;
};

class GameEngine {
public:
    GameEngine();

    void startNewGame(GameMode mode);
    void startNextDeal();

    bool playSelectedCards(int playerId, const std::vector<int>& cardIds, std::string* error = nullptr);
    bool pass(int playerId, std::string* error = nullptr);
    std::vector<int> hintForCurrentPlayer() const;
    void sortCurrentPlayerHand();
    void advanceAiPlayers();

    const std::array<Player, 4>& players() const { return players_; }
    const Player& player(int id) const { return players_[id]; }
    int currentPlayer() const { return currentPlayer_; }
    int currentLevelTeam() const { return currentLevelTeam_; }
    Rank currentLevel() const { return currentLevel_; }
    const std::array<Rank, 2>& teamLevels() const { return teamLevels_; }
    GameMode mode() const { return mode_; }
    GamePhase phase() const { return phase_; }
    const PlayedTrick& lastPlay() const { return lastPlay_; }
    const std::array<std::vector<Card>, 4>& lastShownCards() const { return lastShownCards_; }
    const std::array<PlayerAction, 4>& lastActions() const { return lastActions_; }
    const std::vector<int>& finishOrder() const { return finishOrder_; }
    const std::vector<std::string>& log() const { return log_; }
    int dealNumber() const { return dealNumber_; }

    bool isCurrentPlayerHuman() const;
    bool canCurrentPlayerPass() const;
    bool isActivePlayer(int playerId) const;
    std::string tableStatus() const;

private:
    friend class AiPlayer;

    void dealCards();
    void applyTribute();
    void transferCard(int fromPlayer, int toPlayer, int cardId);
    int highestTributeCardId(int playerId) const;
    int lowestReturnCardId(int playerId) const;
    bool playerHasBothBigJokers(int playerId) const;
    int teamBigJokerCount(const std::vector<int>& players) const;

    void appendLog(const std::string& text);
    void afterSuccessfulPlay(int playerId);
    void afterPass(int playerId);
    void closeTrick();
    void finishPlayerIfNeeded(int playerId);
    void completeRoundIfNeeded();
    void upgradeWinningTeam();
    int nextActivePlayer(int fromPlayer) const;
    int activePlayersCount() const;
    int remainingActivePlayer() const;

    std::array<Player, 4> players_;
    std::array<Rank, 2> teamLevels_ = { Rank::Two, Rank::Two };
    std::array<std::vector<Card>, 4> lastShownCards_;
    std::array<PlayerAction, 4> lastActions_;
    std::vector<int> previousFinishOrder_;
    std::vector<int> finishOrder_;
    std::vector<std::string> log_;

    GameMode mode_ = GameMode::HumanVsAi;
    GamePhase phase_ = GamePhase::Playing;
    PlayedTrick lastPlay_;
    Rank currentLevel_ = Rank::Two;
    int currentLevelTeam_ = 0;
    int currentPlayer_ = 0;
    int starterPlayer_ = 0;
    int passCount_ = 0;
    int dealNumber_ = 0;
    int actionSequence_ = 0;
};

} // namespace guandan
