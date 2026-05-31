#pragma once

#include "GameEngine.h"
#include "TableWidget.h"

#include <array>
#include <QMainWindow>

class QLabel;
class QPushButton;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void newHumanVsAiGame();
    void newLocalGame();
    void playCards();
    void passTurn();
    void hint();
    void toggleArrangeHand();
    void nextDeal();
    void refreshUi();
    void runAiTurn();

private:
    void makeUi();
    int visibleBottomPlayer() const;
    void resetArrangeModes();
    void showMessage(const QString& message);
    void syncLog();
    void scheduleAiTurn();

    guandan::GameEngine engine_;
    TableWidget* table_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTextEdit* logBox_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* passButton_ = nullptr;
    QPushButton* hintButton_ = nullptr;
    QPushButton* nextDealButton_ = nullptr;
    std::array<bool, 4> arrangeModes_ = { false, false, false, false };
    bool aiTurnPending_ = false;
};
