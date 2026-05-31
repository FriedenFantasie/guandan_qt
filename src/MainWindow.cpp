#include "MainWindow.h"

#include "AiPlayer.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QString hudStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow {
            background: #07090d;
        }
        QWidget#root {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #06080d, stop:0.58 #111822, stop:1 #08090d);
            color: #e9edf1;
            font-family: "Microsoft YaHei", "Segoe UI";
        }
        QWidget#topPanel,
        QWidget#sidePanel {
            background: rgba(8, 12, 18, 224);
            border: 1px solid rgba(242, 128, 38, 190);
            border-radius: 8px;
        }
        QLabel {
            color: #e9edf1;
        }
        QLabel#statusLabel {
            color: #f6dec0;
            background: rgba(2, 5, 9, 190);
            border: 1px solid rgba(125, 194, 255, 120);
            border-radius: 4px;
            padding: 5px 10px;
            font-weight: 700;
        }
        QLabel#logTitle {
            color: #ff9a3d;
            font-weight: 800;
            padding: 2px 4px 4px 4px;
            letter-spacing: 0px;
        }
        QPushButton {
            min-height: 30px;
            padding: 6px 13px;
            color: #f7ead7;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #202834, stop:1 #0a0e14);
            border: 1px solid rgba(255, 140, 48, 205);
            border-radius: 4px;
            font-weight: 800;
        }
        QPushButton:hover {
            color: #ffffff;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #34313a, stop:1 #15110d);
            border-color: #ffb060;
        }
        QPushButton:pressed {
            background: #2d1608;
            border-color: #ffd18c;
            padding-top: 7px;
            padding-bottom: 5px;
        }
        QPushButton:disabled {
            color: rgba(220, 224, 228, 82);
            background: #11161d;
            border-color: rgba(130, 140, 150, 70);
        }
        QTextEdit#logBox {
            color: #d8e9ff;
            background: rgba(2, 4, 8, 232);
            border: 1px solid rgba(125, 194, 255, 120);
            border-radius: 6px;
            padding: 7px;
            font-family: "Consolas", "Microsoft YaHei";
            selection-background-color: #a95518;
        }
        QTextEdit#logBox QScrollBar:vertical {
            width: 10px;
            background: #06090e;
            margin: 2px;
        }
        QTextEdit#logBox QScrollBar::handle:vertical {
            background: #c76a24;
            border-radius: 4px;
            min-height: 24px;
        }
        QTextEdit#logBox QScrollBar::add-line:vertical,
        QTextEdit#logBox QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QMessageBox {
            background: #050912;
            color: #d9f0ff;
        }
        QMessageBox QLabel {
            color: #d9f0ff;
            font-size: 13px;
            font-weight: 700;
            background: transparent;
        }
        QMessageBox QPushButton {
            min-width: 76px;
            min-height: 30px;
            color: #eaf8ff;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #123d63, stop:1 #071221);
            border: 1px solid #3fb6ff;
            border-radius: 4px;
            font-weight: 800;
        }
        QMessageBox QPushButton:hover {
            background: #15588c;
            border-color: #8ee0ff;
        }
        QMessageBox QPushButton:pressed {
            background: #061a30;
            border-color: #c7f2ff;
        }
    )");
}

} // namespace

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
    root->setObjectName(QStringLiteral("root"));
    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    QWidget* topPanel = new QWidget(root);
    topPanel->setObjectName(QStringLiteral("topPanel"));
    QHBoxLayout* topBar = new QHBoxLayout(topPanel);
    topBar->setContentsMargins(10, 8, 10, 8);
    topBar->setSpacing(8);
    QPushButton* humanVsAi = new QPushButton(QStringLiteral("1人对3电脑"), topPanel);
    QPushButton* localFour = new QPushButton(QStringLiteral("4人本地"), topPanel);
    nextDealButton_ = new QPushButton(QStringLiteral("下一副"), topPanel);
    statusLabel_ = new QLabel(topPanel);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    statusLabel_->setMinimumWidth(420);

    topBar->addWidget(humanVsAi);
    topBar->addWidget(localFour);
    topBar->addWidget(nextDealButton_);
    topBar->addStretch();
    topBar->addWidget(statusLabel_);
    rootLayout->addWidget(topPanel);

    QHBoxLayout* center = new QHBoxLayout();
    center->setSpacing(10);
    table_ = new TableWidget(root);
    table_->setEngine(&engine_);
    center->addWidget(table_, 1);

    QWidget* sidePanel = new QWidget(root);
    sidePanel->setObjectName(QStringLiteral("sidePanel"));
    sidePanel->setMinimumWidth(282);
    sidePanel->setMaximumWidth(348);
    QVBoxLayout* side = new QVBoxLayout(sidePanel);
    side->setContentsMargins(10, 10, 10, 10);
    side->setSpacing(9);
    playButton_ = new QPushButton(QStringLiteral("出牌"), sidePanel);
    passButton_ = new QPushButton(QStringLiteral("过牌"), sidePanel);
    hintButton_ = new QPushButton(QStringLiteral("提示"), sidePanel);
    logBox_ = new QTextEdit(sidePanel);
    logBox_->setObjectName(QStringLiteral("logBox"));
    logBox_->setReadOnly(true);
    logBox_->setMinimumWidth(260);
    logBox_->setMaximumWidth(330);
    QLabel* logTitle = new QLabel(QStringLiteral("牌局记录"), sidePanel);
    logTitle->setObjectName(QStringLiteral("logTitle"));

    side->addWidget(playButton_);
    side->addWidget(passButton_);
    side->addWidget(hintButton_);
    side->addWidget(logTitle);
    side->addWidget(logBox_, 1);
    center->addWidget(sidePanel);
    rootLayout->addLayout(center, 1);

    setCentralWidget(root);
    setStyleSheet(hudStyleSheet());
    resize(1280, 780);
    setWindowTitle(QStringLiteral("掼蛋 - Qt 四人桌"));
    setWindowIcon(QIcon(QStringLiteral(":/cards/icons/app_icon.ico")));

    connect(humanVsAi, &QPushButton::clicked, this, &MainWindow::newHumanVsAiGame);
    connect(localFour, &QPushButton::clicked, this, &MainWindow::newLocalGame);
    connect(nextDealButton_, &QPushButton::clicked, this, &MainWindow::nextDeal);
    connect(playButton_, &QPushButton::clicked, this, &MainWindow::playCards);
    connect(passButton_, &QPushButton::clicked, this, &MainWindow::passTurn);
    connect(hintButton_, &QPushButton::clicked, this, &MainWindow::hint);
    connect(table_, &TableWidget::selectionChanged, this, &MainWindow::refreshUi);
    connect(table_, &TableWidget::arrangeRequested, this, &MainWindow::sortHand);
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
