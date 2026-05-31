#include "MainWindow.h"

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
    sortButton_ = new QPushButton(QStringLiteral("整理"), root);
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
    engine_.startNewGame(guandan::GameMode::HumanVsAi);
    table_->clearSelection();
    refreshUi();
}

void MainWindow::newLocalGame()
{
    engine_.startNewGame(guandan::GameMode::LocalFour);
    table_->clearSelection();
    refreshUi();
}

void MainWindow::playCards()
{
    std::string error;
    if (!engine_.playSelectedCards(engine_.currentPlayer(), table_->selectedCardIds(), &error)) {
        showMessage(QString::fromStdString(error));
    }
    table_->clearSelection();
    refreshUi();
}

void MainWindow::passTurn()
{
    std::string error;
    if (!engine_.pass(engine_.currentPlayer(), &error)) {
        showMessage(QString::fromStdString(error));
    }
    table_->clearSelection();
    refreshUi();
}

void MainWindow::hint()
{
    table_->setSelectedCardIds(engine_.hintForCurrentPlayer());
    refreshUi();
}

void MainWindow::sortHand()
{
    engine_.sortCurrentPlayerHand();
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
}

void MainWindow::refreshUi()
{
    const bool humanTurn = engine_.isCurrentPlayerHuman();
    const bool roundOver = engine_.phase() == guandan::GamePhase::RoundOver;
    playButton_->setEnabled(humanTurn && !roundOver && !table_->selectedCardIds().empty());
    passButton_->setEnabled(humanTurn && !roundOver && engine_.canCurrentPlayerPass());
    hintButton_->setEnabled(humanTurn && !roundOver);
    sortButton_->setEnabled(humanTurn && !roundOver);
    nextDealButton_->setEnabled(true);

    statusLabel_->setText(QString::fromStdString(engine_.tableStatus()));
    syncLog();
    table_->update();
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

