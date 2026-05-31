#pragma once

#include "GameEngine.h"

#include <QElapsedTimer>
#include <QWidget>

#include <set>
#include <utility>
#include <vector>

enum class CardBackStyle {
    Neutral,
    Ally,
    Opponent
};

class TableWidget : public QWidget {
    Q_OBJECT

public:
    explicit TableWidget(QWidget* parent = nullptr);

    void setEngine(guandan::GameEngine* engine);
    void beginActionAnimation(int playerId);
    std::vector<int> selectedCardIds() const;
    void setSelectedCardIds(const std::vector<int>& ids);
    void clearSelection();

signals:
    void selectionChanged();
    void arrangeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize minimumSizeHint() const override;

private slots:
    void updateActionAnimation();

private:
    QRect cardRectForIndex(int index, int count, const QRect& area, bool selected) const;
    void drawPlayerHand(QPainter& painter, int playerId, const QRect& area, bool faceUp, bool interactive);
    void drawHandGroupHints(QPainter& painter, const std::vector<guandan::Card>& hand, const std::vector<QRect>& cardRects);
    void drawLastCards(QPainter& painter, int playerId, const QRect& area);
    void drawActionText(QPainter& painter, int playerId, const QRect& area);
    void drawBombEffect(QPainter& painter, const QRect& area, qreal progress, const QPoint& offset);
    void drawSettlementOverlay(QPainter& painter);
    void drawArrangeButton(QPainter& painter, const QRect& playerArea);
    void drawCard(QPainter& painter, const QRect& rect, const guandan::Card& card, bool faceUp, bool selected,
                  CardBackStyle backStyle = CardBackStyle::Neutral);
    bool canUseArrangeButton() const;
    QString playerLabel(int playerId) const;
    int visualPlayerForSeat(int seat) const;
    CardBackStyle backStyleForPlayer(int playerId) const;
    QPoint animationOffsetForPlayer(int playerId) const;

    guandan::GameEngine* engine_ = nullptr;
    std::set<int> selectedIds_;
    std::vector<std::pair<int, QRect>> clickableCards_;
    QRect arrangeButtonRect_;
    QElapsedTimer animationClock_;
    QTimer* animationTimer_ = nullptr;
    int animatingPlayer_ = -1;
    qreal animationProgress_ = 1.0;
};
