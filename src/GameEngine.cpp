#include "GameEngine.h"

#include "AiPlayer.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>

namespace guandan {
namespace {

std::string cardsText(const std::vector<Card>& cards)
{
    std::ostringstream out;
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) {
            out << ' ';
        }
        out << cardText(cards[i]);
    }
    return out.str();
}

int findCardIndexById(const std::vector<Card>& cards, int id)
{
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (cards[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int advanceAmountForOrder(const std::vector<int>& order)
{
    if (order.size() != 4) {
        return 1;
    }
    const int winnerTeam = order[0] % 2;
    if (order[1] % 2 == winnerTeam) {
        return 3;
    }
    if (order[2] % 2 == winnerTeam) {
        return 2;
    }
    return 1;
}

} // namespace

GameEngine::GameEngine()
{
    startNewGame(GameMode::HumanVsAi);
}

void GameEngine::startNewGame(GameMode mode)
{
    mode_ = mode;
    teamLevels_ = { Rank::Two, Rank::Two };
    currentLevelTeam_ = 0;
    currentLevel_ = Rank::Two;
    starterPlayer_ = 0;
    previousFinishOrder_.clear();
    finishOrder_.clear();
    log_.clear();
    dealNumber_ = 0;

    players_[0].name = "玩家一";
    players_[1].name = mode == GameMode::HumanVsAi ? "电脑一" : "玩家二";
    players_[2].name = mode == GameMode::HumanVsAi ? "电脑二" : "玩家三";
    players_[3].name = mode == GameMode::HumanVsAi ? "电脑三" : "玩家四";
    for (int i = 0; i < 4; ++i) {
        players_[i].id = i;
        players_[i].human = mode == GameMode::LocalFour || i == 0;
    }

    startNextDeal();
}

void GameEngine::startNextDeal()
{
    dealNumber_++;
    currentLevel_ = teamLevels_[currentLevelTeam_];
    phase_ = GamePhase::Playing;
    lastPlay_ = {};
    lastShownCards_ = {};
    lastActions_ = {};
    passCount_ = 0;
    actionSequence_ = 0;
    finishOrder_.clear();

    dealCards();
    applyTribute();
    currentPlayer_ = starterPlayer_;

    std::ostringstream out;
    out << "第 " << dealNumber_ << " 副开始，打 " << levelText(currentLevel_)
        << "，先手：" << players_[currentPlayer_].name;
    appendLog(out.str());
}

void GameEngine::dealCards()
{
    std::vector<Card> deck = makeTwoDecks();
    const auto seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);
    std::shuffle(deck.begin(), deck.end(), rng);

    for (Player& player : players_) {
        player.hand.clear();
        player.finished = false;
        player.finishPlace = 0;
    }

    for (std::size_t i = 0; i < deck.size(); ++i) {
        players_[i % 4].hand.push_back(deck[i]);
    }
    for (Player& player : players_) {
        sortHand(player.hand, currentLevel_);
    }
}

bool GameEngine::playSelectedCards(int playerId, const std::vector<int>& cardIds, std::string* error)
{
    if (phase_ != GamePhase::Playing) {
        if (error) {
            *error = "本副已经结束，请开始下一副。";
        }
        return false;
    }
    if (playerId != currentPlayer_) {
        if (error) {
            *error = "还没轮到这个玩家。";
        }
        return false;
    }
    if (cardIds.empty()) {
        if (error) {
            *error = "请先选择要出的牌。";
        }
        return false;
    }

    Player& player = players_[playerId];
    std::vector<Card> selected;
    for (int id : cardIds) {
        const int index = findCardIndexById(player.hand, id);
        if (index < 0) {
            if (error) {
                *error = "选择的牌不在当前手牌里。";
            }
            return false;
        }
        selected.push_back(player.hand[index]);
    }

    HandAnalysis analysis = HandAnalyzer::analyze(selected, currentLevel_);
    if (!analysis.valid()) {
        if (error) {
            *error = "这组牌不是合法牌型。";
        }
        return false;
    }
    if (!HandAnalyzer::canBeat(analysis, lastPlay_.analysis)) {
        if (error) {
            *error = "这组牌压不过当前桌面牌。";
        }
        return false;
    }

    for (const Card& card : selected) {
        const int index = findCardIndexById(player.hand, card.id);
        if (index >= 0) {
            player.hand.erase(player.hand.begin() + index);
        }
    }
    sortHand(player.hand, currentLevel_);

    lastPlay_ = { playerId, selected, analysis };
    lastShownCards_[playerId] = selected;
    lastActions_[playerId] = { ++actionSequence_, false, analysis.typeName(), analysis.isBombLike() };
    passCount_ = 0;

    std::ostringstream out;
    out << player.name << " 出 " << analysis.typeName() << "：" << cardsText(selected);
    appendLog(out.str());

    afterSuccessfulPlay(playerId);
    return true;
}

bool GameEngine::pass(int playerId, std::string* error)
{
    if (phase_ != GamePhase::Playing) {
        if (error) {
            *error = "本副已经结束，请开始下一副。";
        }
        return false;
    }
    if (playerId != currentPlayer_) {
        if (error) {
            *error = "还没轮到这个玩家。";
        }
        return false;
    }
    if (!canCurrentPlayerPass()) {
        if (error) {
            *error = "新一轮必须出牌，不能过牌。";
        }
        return false;
    }

    lastShownCards_[playerId].clear();
    lastActions_[playerId] = { ++actionSequence_, true, "过牌", false };
    appendLog(players_[playerId].name + " 过牌。");
    afterPass(playerId);
    return true;
}

std::vector<int> GameEngine::hintForCurrentPlayer() const
{
    const Player& player = players_[currentPlayer_];
    const std::optional<HandAnalysis> previous =
        lastPlay_.empty() ? std::nullopt : std::optional<HandAnalysis>(lastPlay_.analysis);
    const std::vector<CandidatePlay> plays =
        HandAnalyzer::generateCandidatePlays(player.hand, currentLevel_, previous);
    if (plays.empty()) {
        return {};
    }

    for (const CandidatePlay& play : plays) {
        if (!play.analysis.isBombLike()) {
            std::vector<int> ids;
            for (const Card& card : play.cards) {
                ids.push_back(card.id);
            }
            return ids;
        }
    }

    std::vector<int> ids;
    for (const Card& card : plays.front().cards) {
        ids.push_back(card.id);
    }
    return ids;
}

void GameEngine::sortCurrentPlayerHand()
{
    sortHand(players_[currentPlayer_].hand, currentLevel_);
}

void GameEngine::advanceAiPlayers()
{
    int safety = 0;
    while (phase_ == GamePhase::Playing && mode_ == GameMode::HumanVsAi && !players_[currentPlayer_].human) {
        if (++safety > 80) {
            appendLog("AI 连续行动次数过多，已暂停。");
            break;
        }

        const AiDecision decision = AiPlayer::choose(*this, currentPlayer_);
        std::string error;
        if (decision.pass) {
            if (!pass(currentPlayer_, &error)) {
                appendLog(players_[currentPlayer_].name + " 无法过牌：" + error);
                break;
            }
        } else {
            if (!playSelectedCards(currentPlayer_, decision.cardIds, &error)) {
                appendLog(players_[currentPlayer_].name + " 出牌失败，改为过牌：" + error);
                if (!canCurrentPlayerPass() || !pass(currentPlayer_, nullptr)) {
                    break;
                }
            }
        }
    }
}

bool GameEngine::isCurrentPlayerHuman() const
{
    return players_[currentPlayer_].human;
}

bool GameEngine::canCurrentPlayerPass() const
{
    return !lastPlay_.empty() && lastPlay_.player != currentPlayer_;
}

bool GameEngine::isActivePlayer(int playerId) const
{
    return !players_[playerId].finished && !players_[playerId].hand.empty();
}

std::string GameEngine::tableStatus() const
{
    std::ostringstream out;
    out << "队伍0打 " << levelText(teamLevels_[0]) << "，队伍1打 " << levelText(teamLevels_[1])
        << "；本副级牌 " << levelText(currentLevel_) << "；当前：" << players_[currentPlayer_].name;
    if (!lastPlay_.empty()) {
        out << "；桌面：" << players_[lastPlay_.player].name << " 的 " << lastPlay_.analysis.typeName();
    }
    if (phase_ == GamePhase::RoundOver) {
        out << "；本副结束";
    }
    return out.str();
}

int GameEngine::lastRoundWinningTeam() const
{
    if (finishOrder_.empty()) {
        return -1;
    }
    return finishOrder_.front() % 2;
}

int GameEngine::lastRoundUpgradeAmount() const
{
    if (phase_ != GamePhase::RoundOver || finishOrder_.size() != 4) {
        return 0;
    }
    return advanceAmountForOrder(finishOrder_);
}

void GameEngine::applyTribute()
{
    if (previousFinishOrder_.size() != 4) {
        return;
    }

    const int first = previousFinishOrder_[0];
    const int second = previousFinishOrder_[1];
    const int third = previousFinishOrder_[2];
    const int fourth = previousFinishOrder_[3];

    if (sameTeam(first, second)) {
        std::vector<int> donors = { third, fourth };
        if (teamBigJokerCount(donors) >= 2) {
            appendLog("双贡被抗贡：下游方合计持有两张大王。");
            return;
        }
        std::vector<int> receivers = { first, second };
        for (int i = 0; i < 2; ++i) {
            const int gift = highestTributeCardId(donors[i]);
            if (gift >= 0) {
                transferCard(donors[i], receivers[i], gift);
            }
        }
        for (int i = 0; i < 2; ++i) {
            const int back = lowestReturnCardId(receivers[i]);
            if (back >= 0) {
                transferCard(receivers[i], donors[i], back);
            }
        }
        appendLog("执行双贡和还贡。");
        return;
    }

    if (playerHasBothBigJokers(fourth)) {
        appendLog("末游持有两张大王，抗贡成功。");
        return;
    }

    const int gift = highestTributeCardId(fourth);
    if (gift >= 0) {
        transferCard(fourth, first, gift);
    }
    const int back = lowestReturnCardId(first);
    if (back >= 0) {
        transferCard(first, fourth, back);
    }
    appendLog("执行单贡和还贡。");
}

void GameEngine::transferCard(int fromPlayer, int toPlayer, int cardId)
{
    Player& from = players_[fromPlayer];
    Player& to = players_[toPlayer];
    const int index = findCardIndexById(from.hand, cardId);
    if (index < 0) {
        return;
    }
    Card card = from.hand[index];
    from.hand.erase(from.hand.begin() + index);
    to.hand.push_back(card);
    sortHand(from.hand, currentLevel_);
    sortHand(to.hand, currentLevel_);
}

int GameEngine::highestTributeCardId(int playerId) const
{
    const Player& player = players_[playerId];
    if (player.hand.empty()) {
        return -1;
    }
    return std::max_element(player.hand.begin(), player.hand.end(), [this](const Card& left, const Card& right) {
        const int lv = rankOrderValue(left.rank, currentLevel_);
        const int rv = rankOrderValue(right.rank, currentLevel_);
        if (lv != rv) {
            return lv < rv;
        }
        return left.id < right.id;
    })->id;
}

int GameEngine::lowestReturnCardId(int playerId) const
{
    const Player& player = players_[playerId];
    if (player.hand.empty()) {
        return -1;
    }

    auto nonJoker = std::min_element(player.hand.begin(), player.hand.end(), [this](const Card& left, const Card& right) {
        const bool lj = isJoker(left);
        const bool rj = isJoker(right);
        if (lj != rj) {
            return !lj;
        }
        const int lv = rankOrderValue(left.rank, currentLevel_);
        const int rv = rankOrderValue(right.rank, currentLevel_);
        if (lv != rv) {
            return lv < rv;
        }
        return left.id < right.id;
    });
    return nonJoker->id;
}

bool GameEngine::playerHasBothBigJokers(int playerId) const
{
    int count = 0;
    for (const Card& card : players_[playerId].hand) {
        if (card.rank == Rank::BigJoker) {
            count++;
        }
    }
    return count >= 2;
}

int GameEngine::teamBigJokerCount(const std::vector<int>& players) const
{
    int count = 0;
    for (int playerId : players) {
        for (const Card& card : players_[playerId].hand) {
            if (card.rank == Rank::BigJoker) {
                count++;
            }
        }
    }
    return count;
}

void GameEngine::appendLog(const std::string& text)
{
    log_.push_back(text);
    if (log_.size() > 80) {
        log_.erase(log_.begin());
    }
}

void GameEngine::afterSuccessfulPlay(int playerId)
{
    finishPlayerIfNeeded(playerId);
    completeRoundIfNeeded();
    if (phase_ == GamePhase::RoundOver) {
        return;
    }
    currentPlayer_ = nextActivePlayer(playerId);
}

void GameEngine::afterPass(int playerId)
{
    passCount_++;
    const int activeOtherCount = activePlayersCount() - (isActivePlayer(lastPlay_.player) ? 1 : 0);
    if (passCount_ >= std::max(1, activeOtherCount)) {
        closeTrick();
        return;
    }
    currentPlayer_ = nextActivePlayer(playerId);
}

void GameEngine::closeTrick()
{
    const int winner = lastPlay_.player;
    lastPlay_ = {};
    passCount_ = 0;

    if (winner < 0) {
        currentPlayer_ = nextActivePlayer(currentPlayer_);
        return;
    }

    if (!isActivePlayer(winner)) {
        const int partner = (winner + 2) % 4;
        if (isActivePlayer(partner)) {
            currentPlayer_ = partner;
            appendLog(players_[partner].name + " 接风。");
            return;
        }
    }

    currentPlayer_ = isActivePlayer(winner) ? winner : nextActivePlayer(winner);
}

void GameEngine::finishPlayerIfNeeded(int playerId)
{
    Player& player = players_[playerId];
    if (!player.finished && player.hand.empty()) {
        player.finished = true;
        player.finishPlace = static_cast<int>(finishOrder_.size()) + 1;
        finishOrder_.push_back(playerId);
        appendLog(player.name + " 第 " + std::to_string(player.finishPlace) + " 个出完。");
    }
}

void GameEngine::completeRoundIfNeeded()
{
    if (activePlayersCount() > 1) {
        return;
    }

    const int last = remainingActivePlayer();
    if (last >= 0 && !players_[last].finished) {
        players_[last].finished = true;
        players_[last].finishPlace = static_cast<int>(finishOrder_.size()) + 1;
        finishOrder_.push_back(last);
    }

    phase_ = GamePhase::RoundOver;
    previousFinishOrder_ = finishOrder_;
    upgradeWinningTeam();
}

void GameEngine::upgradeWinningTeam()
{
    if (finishOrder_.empty()) {
        return;
    }

    const int winningTeam = finishOrder_[0] % 2;
    const int amount = advanceAmountForOrder(finishOrder_);
    for (int i = 0; i < amount; ++i) {
        teamLevels_[winningTeam] = nextLevel(teamLevels_[winningTeam]);
    }
    currentLevelTeam_ = winningTeam;
    starterPlayer_ = finishOrder_[0];

    std::ostringstream out;
    out << "本副结束，队伍" << winningTeam << "升级 " << amount << " 级，下一副打 "
        << levelText(teamLevels_[winningTeam]) << "。";
    appendLog(out.str());
}

int GameEngine::nextActivePlayer(int fromPlayer) const
{
    for (int step = 1; step <= 4; ++step) {
        const int candidate = (fromPlayer + step) % 4;
        if (isActivePlayer(candidate)) {
            return candidate;
        }
    }
    return fromPlayer;
}

int GameEngine::activePlayersCount() const
{
    int count = 0;
    for (const Player& player : players_) {
        if (!player.finished && !player.hand.empty()) {
            count++;
        }
    }
    return count;
}

int GameEngine::remainingActivePlayer() const
{
    for (const Player& player : players_) {
        if (!player.finished && !player.hand.empty()) {
            return player.id;
        }
    }
    return -1;
}

} // namespace guandan
