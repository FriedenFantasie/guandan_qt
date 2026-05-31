#include "MainWindow.h"

#include "AiPlayer.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    makeUi();
    engine_.startNewGame(guandan::GameMode::HumanVsAi);
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::makeUi()
{
    QWidget* root = new QWidget(this);
    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    QHBoxLayout* topBar = new QHBoxLayout();
    QPushButton* humanVsAi = new QPushButton(QStringLiteral("1人对3电脑"), root);
    QPushButton* localFour = new QPushButton(QStringLiteral("4人本地"), root);
    nextDealButton_ = new QPushButton(QStringLiteral("下一副"), root);
    statusLabel_ = new QLabel(root);
    statusLabel_->setMinimumWidth(420);

    topBar->addWidget(humanVsAi);
    topBar->addWidget(localFour);
    topBar->addWidget(nextDealButton_);
    topBar->addStretch();
    topBar->addWidget(statusLabel_);
    rootLayout->addLayout(topBar);

    QHBoxLayout* center = new QHBoxLayout();
    table_ = new TableWidget(root);
    table_->setEngine(&engine_);
    center->addWidget(table_, 1);

    QVBoxLayout* side = new QVBoxLayout();
    playButton_ = new QPushButton(QStringLiteral("出牌"), root);
    passButton_ = new QPushButton(QStringLiteral("过牌"), root);
    hintButton_ = new QPushButton(QStringLiteral("提示"), root);
    sortButton_ = new QPushButton(QStringLiteral("一键理牌"), root);
    logBox_ = new QTextEdit(root);
    logBox_->setReadOnly(true);
    logBox_->setMinimumWidth(260);
    logBox_->setMaximumWidth(330);

    side->addWidget(playButton_);
    side->addWidget(passButton_);
    side->addWidget(hintButton_);
    side->addWidget(sortButton_);
    side->addWidget(new QLabel(QStringLiteral("牌局记录"), root));
    side->addWidget(logBox_, 1);
    center->addLayout(side);
    rootLayout->addLayout(center, 1);

    setCentralWidget(root);
    resize(1280, 780);
    setWindowTitle(QStringLiteral("掼蛋 - Qt 四人桌"));

    connect(humanVsAi, &QPushButton::clicked, this, &MainWindow::newHumanVsAiGame);
    connect(localFour, &QPushButton::clicked, this, &MainWindow::newLocalGame);
    connect(nextDealButton_, &QPushButton::clicked, this, &MainWindow::nextDeal);
    connect(playButton_, &QPushButton::clicked, this, &MainWindow::playCards);
    connect(passButton_, &QPushButton::clicked, this, &MainWindow::passTurn);
    connect(hintButton_, &QPushButton::clicked, this, &MainWindow::hint);
    connect(sortButton_, &QPushButton::clicked, this, &MainWindow::sortHand);
    connect(table_, &TableWidget::selectionChanged, this, &MainWindow::refreshUi);
}

void MainWindow::newHumanVsAiGame()
{
    aiTurnPending_ = false;
    engine_.startNewGame(guandan::GameMode::HumanVsAi);
    table_->clearSelection();
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::newLocalGame()
{
    aiTurnPending_ = false;
    engine_.startNewGame(guandan::GameMode::LocalFour);
    table_->clearSelection();
    refreshUi();
}

void MainWindow::playCards()
{
    const int actingPlayer = engine_.currentPlayer();
    std::string error;
    if (!engine_.playSelectedCards(actingPlayer, table_->selectedCardIds(), &error)) {
        showMessage(QString::fromStdString(error));
    } else {
        table_->beginActionAnimation(actingPlayer);
    }
    table_->clearSelection();
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::passTurn()
{
    const int actingPlayer = engine_.currentPlayer();
    std::string error;
    if (!engine_.pass(actingPlayer, &error)) {
        showMessage(QString::fromStdString(error));
    } else {
        table_->beginActionAnimation(actingPlayer);
    }
    table_->clearSelection();
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::hint()
{
    table_->setSelectedCardIds(engine_.hintForCurrentPlayer());
    refreshUi();
}

void MainWindow::sortHand()
{
    engine_.arrangeCurrentPlayerHand();
    table_->clearSelection();
    refreshUi();
}

void MainWindow::nextDeal()
{
    if (engine_.phase() != guandan::GamePhase::RoundOver) {
        const auto answer = QMessageBox::question(
            this,
            QStringLiteral("提前开始下一副"),
            QStringLiteral("当前这一副还没有结束，确定重新发下一副吗？"));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }
    engine_.startNextDeal();
    table_->clearSelection();
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::refreshUi()
{
    const bool humanTurn = engine_.isCurrentPlayerHuman();
    const bool roundOver = engine_.phase() == guandan::GamePhase::RoundOver;
    const bool acceptingInput = humanTurn && !roundOver && !aiTurnPending_;
    playButton_->setEnabled(acceptingInput && !table_->selectedCardIds().empty());
    passButton_->setEnabled(acceptingInput && engine_.canCurrentPlayerPass());
    hintButton_->setEnabled(acceptingInput);
    sortButton_->setEnabled(acceptingInput);
    nextDealButton_->setEnabled(true);

    statusLabel_->setText(QString::fromStdString(engine_.tableStatus()));
    syncLog();
    table_->update();
}

void MainWindow::runAiTurn()
{
    aiTurnPending_ = false;
    if (engine_.mode() != guandan::GameMode::HumanVsAi ||
        engine_.phase() != guandan::GamePhase::Playing ||
        engine_.isCurrentPlayerHuman()) {
        refreshUi();
        return;
    }

    const int actingPlayer = engine_.currentPlayer();
    const guandan::AiDecision decision = guandan::AiPlayer::choose(engine_, actingPlayer);
    std::string error;
    bool ok = false;
    if (decision.pass) {
        ok = engine_.pass(actingPlayer, &error);
    } else {
        ok = engine_.playSelectedCards(actingPlayer, decision.cardIds, &error);
        if (!ok && engine_.canCurrentPlayerPass()) {
            ok = engine_.pass(actingPlayer, &error);
        }
    }

    if (!ok) {
        // A failed AI move should be visible but non-blocking during a casual game.
        table_->clearSelection();
        refreshUi();
        return;
    }

    table_->beginActionAnimation(actingPlayer);
    table_->clearSelection();
    refreshUi();
    scheduleAiTurn();
}

void MainWindow::showMessage(const QString& message)
{
    QMessageBox::information(this, QStringLiteral("提示"), message);
}

void MainWindow::syncLog()
{
    QString text;
    for (const std::string& line : engine_.log()) {
        text += QString::fromStdString(line) + QLatin1Char('\n');
    }
    logBox_->setPlainText(text);
    logBox_->moveCursor(QTextCursor::End);
}

void MainWindow::scheduleAiTurn()
{
    if (aiTurnPending_ ||
        engine_.mode() != guandan::GameMode::HumanVsAi ||
        engine_.phase() != guandan::GamePhase::Playing ||
        engine_.isCurrentPlayerHuman()) {
        return;
    }

    aiTurnPending_ = true;
    refreshUi();
    QTimer::singleShot(760, this, &MainWindow::runAiTurn);
}
