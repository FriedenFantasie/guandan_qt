#include "TableWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSvgRenderer>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>

namespace {

constexpr int kCardWidth = 72;
constexpr int kCardHeight = 104;
constexpr int kActionAnimationMs = 560;
constexpr qreal kPi = 3.14159265358979323846;

QColor suitColor(guandan::Suit suit, guandan::Rank rank)
{
    if (rank == guandan::Rank::BigJoker) {
        return QColor(180, 30, 40);
    }
    if (rank == guandan::Rank::SmallJoker) {
        return QColor(30, 40, 60);
    }
    if (suit == guandan::Suit::Hearts || suit == guandan::Suit::Diamonds) {
        return QColor(190, 30, 45);
    }
    return QColor(22, 30, 42);
}

QString qCardRank(guandan::Rank rank)
{
    return QString::fromStdString(guandan::rankText(rank));
}

QString qCardSuit(guandan::Suit suit)
{
    switch (suit) {
    case guandan::Suit::Spades: return QStringLiteral("♠");
    case guandan::Suit::Hearts: return QStringLiteral("♥");
    case guandan::Suit::Clubs: return QStringLiteral("♣");
    case guandan::Suit::Diamonds: return QStringLiteral("♦");
    case guandan::Suit::Joker: return QStringLiteral("★");
    }
    return QStringLiteral("?");
}

QString qLevel(guandan::Rank rank)
{
    return QString::fromStdString(guandan::levelText(rank));
}

QString placeText(int place)
{
    switch (place) {
    case 1: return QStringLiteral("头游");
    case 2: return QStringLiteral("二游");
    case 3: return QStringLiteral("三游");
    case 4: return QStringLiteral("末游");
    default: return QStringLiteral("-");
    }
}

bool sameCardOrder(const std::vector<guandan::Card>& left, const std::vector<guandan::Card>& right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].id != right[i].id) {
            return false;
        }
    }
    return true;
}

bool shouldShowGroupHint(const guandan::ArrangedGroup& group)
{
    switch (group.analysis.type) {
    case guandan::HandType::Straight:
    case guandan::HandType::ConsecutivePairs:
    case guandan::HandType::ConsecutiveTriples:
    case guandan::HandType::TripleWithPair:
    case guandan::HandType::Bomb:
    case guandan::HandType::StraightFlush:
    case guandan::HandType::JokerBomb:
        return true;
    default:
        return false;
    }
}

QColor groupHintColor(int index)
{
    const std::array<QColor, 6> colors = {
        QColor(248, 210, 82, 226),
        QColor(108, 198, 255, 218),
        QColor(134, 226, 139, 218),
        QColor(255, 145, 112, 218),
        QColor(205, 164, 255, 218),
        QColor(255, 231, 136, 218)
    };
    return colors[static_cast<std::size_t>(index) % colors.size()];
}

void drawSvgMark(QPainter& painter, const QRectF& bounds, CardBackStyle style)
{
    const QString path = style == CardBackStyle::Ally
        ? QStringLiteral(":/cards/icons/rebel.svg")
        : QStringLiteral(":/cards/icons/empire.svg");
    QSvgRenderer renderer(path);
    if (renderer.isValid()) {
        renderer.render(&painter, bounds);
    }
}

void drawCardBack(QPainter& painter, const QRect& rect, CardBackStyle style)
{
    QColor top;
    QColor bottom;
    QColor line;
    QColor border;
    switch (style) {
    case CardBackStyle::Ally:
        top = QColor(247, 136, 38);
        bottom = QColor(205, 75, 20);
        line = QColor(255, 203, 120, 120);
        border = QColor(116, 48, 15);
        break;
    case CardBackStyle::Opponent:
        top = QColor(28, 29, 32);
        bottom = QColor(4, 5, 8);
        line = QColor(255, 255, 255, 58);
        border = QColor(190, 196, 204);
        break;
    case CardBackStyle::Neutral:
        top = QColor(54, 105, 183);
        bottom = QColor(30, 71, 145);
        line = QColor(230, 238, 255, 115);
        border = QColor(20, 37, 78);
        break;
    }

    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0.0, top);
    gradient.setColorAt(1.0, bottom);
    painter.fillPath(path, gradient);
    painter.setPen(QPen(border, 1));
    painter.drawPath(path);

    painter.save();
    painter.setClipPath(path);
    painter.setPen(QPen(line, style == CardBackStyle::Opponent ? 1 : 2));
    for (int y = 12; y < kCardHeight + 22; y += 14) {
        painter.drawLine(8, y, kCardWidth - 8, y + 18);
    }
    painter.setPen(QPen(QColor(255, 255, 255, style == CardBackStyle::Opponent ? 42 : 54), 1));
    painter.drawRoundedRect(rect.adjusted(7, 7, -7, -7), 6, 6);
    painter.restore();

    const QRectF markRect(rect.left() + 15, rect.top() + 22, rect.width() - 30, rect.height() - 44);
    if (style == CardBackStyle::Ally) {
        drawSvgMark(painter, markRect, style);
    } else if (style == CardBackStyle::Opponent) {
        drawSvgMark(painter, markRect, style);
    } else {
        painter.setPen(QPen(QColor(230, 238, 255), 2));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 18, QFont::Bold));
        painter.drawText(rect, Qt::AlignCenter, QStringLiteral("GD"));
    }
}

class CardPixmapCache {
public:
    QPixmap pixmap(const guandan::Card& card, bool faceUp, int levelValue, bool wild, CardBackStyle backStyle)
    {
        const QString key = faceUp
            ? QStringLiteral("%1-%2-%3-%4-%5-%6")
                  .arg(card.id)
                  .arg(static_cast<int>(card.suit))
                  .arg(static_cast<int>(card.rank))
                  .arg(faceUp)
                  .arg(levelValue + (wild ? 100 : 0))
                  .arg(static_cast<int>(backStyle))
            : QStringLiteral("back-%1").arg(static_cast<int>(backStyle));
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }

        QPixmap pixmap(kCardWidth * 2, kCardHeight * 2);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(2.0, 2.0);

        QRect rect(1, 1, kCardWidth - 2, kCardHeight - 2);
        QPainterPath path;
        path.addRoundedRect(rect, 8, 8);

        if (!faceUp) {
            drawCardBack(painter, rect, backStyle);
        } else {
            painter.fillPath(path, QColor(252, 250, 244));
            painter.setPen(QPen(QColor(28, 31, 36), 1));
            painter.drawPath(path);

            const QColor color = suitColor(card.suit, card.rank);
            painter.setPen(color);
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::Bold));
            painter.drawText(QRect(7, 5, kCardWidth - 14, 24), Qt::AlignLeft, qCardRank(card.rank));
            painter.setFont(QFont(QStringLiteral("Segoe UI Symbol"), 18));
            painter.drawText(QRect(7, 28, kCardWidth - 14, 24), Qt::AlignLeft, qCardSuit(card.suit));

            painter.setFont(QFont(QStringLiteral("Segoe UI Symbol"), 28, QFont::Bold));
            painter.drawText(QRect(0, 35, kCardWidth, 42), Qt::AlignCenter, qCardSuit(card.suit));

            if (wild) {
                painter.setPen(QPen(QColor(219, 126, 20), 2));
                painter.drawRoundedRect(QRect(5, 5, kCardWidth - 10, kCardHeight - 10), 7, 7);
                painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8, QFont::Bold));
                painter.drawText(QRect(0, kCardHeight - 21, kCardWidth, 18), Qt::AlignCenter, QStringLiteral("逢人配"));
            }
        }

        cache_[key] = pixmap;
        return pixmap;
    }

private:
    std::map<QString, QPixmap> cache_;
};

CardPixmapCache& cardCache()
{
    static CardPixmapCache cache;
    return cache;
}

} // namespace

TableWidget::TableWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAutoFillBackground(false);
    animationTimer_ = new QTimer(this);
    animationTimer_->setInterval(16);
    connect(animationTimer_, &QTimer::timeout, this, &TableWidget::updateActionAnimation);
}

void TableWidget::setEngine(guandan::GameEngine* engine)
{
    engine_ = engine;
    clearSelection();
    update();
}

void TableWidget::beginActionAnimation(int playerId)
{
    animatingPlayer_ = playerId;
    animationProgress_ = 0.0;
    animationClock_.restart();
    animationTimer_->start();
    update();
}

std::vector<int> TableWidget::selectedCardIds() const
{
    return { selectedIds_.begin(), selectedIds_.end() };
}

void TableWidget::setSelectedCardIds(const std::vector<int>& ids)
{
    selectedIds_.clear();
    for (int id : ids) {
        selectedIds_.insert(id);
    }
    emit selectionChanged();
    update();
}

void TableWidget::clearSelection()
{
    selectedIds_.clear();
    emit selectionChanged();
    update();
}

QSize TableWidget::minimumSizeHint() const
{
    return QSize(960, 680);
}

void TableWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(42, 112, 82));

    QRect table = rect().adjusted(22, 18, -22, -18);
    QPainterPath tablePath;
    tablePath.addRoundedRect(table, 18, 18);
    painter.fillPath(tablePath, QColor(36, 128, 86));
    painter.setPen(QPen(QColor(224, 235, 218, 110), 2));
    painter.drawPath(tablePath);

    if (!engine_) {
        painter.setPen(Qt::white);
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 20, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("掼蛋"));
        return;
    }

    clickableCards_.clear();
    arrangeButtonRect_ = {};

    const QRect bottom(width() / 2 - 390, height() - 145, 780, 126);
    const QRect bottomCards = bottom.adjusted(0, 0, -180, 0);
    const QRect top(width() / 2 - 300, 28, 600, 110);
    const QRect left(25, height() / 2 - 105, 230, 170);
    const QRect right(width() - 255, height() / 2 - 105, 230, 170);

    drawPlayerHand(painter, visualPlayerForSeat(2), top, false, false);
    drawPlayerHand(painter, visualPlayerForSeat(1), left, false, false);
    drawPlayerHand(painter, visualPlayerForSeat(3), right, false, false);
    drawPlayerHand(painter, visualPlayerForSeat(0), bottomCards, true, true);
    drawArrangeButton(painter, bottom);

    drawLastCards(painter, visualPlayerForSeat(2), QRect(width() / 2 - 210, 154, 420, 110));
    drawLastCards(painter, visualPlayerForSeat(1), QRect(width() / 2 - 360, height() / 2 - 70, 260, 110));
    drawLastCards(painter, visualPlayerForSeat(3), QRect(width() / 2 + 100, height() / 2 - 70, 260, 110));
    drawLastCards(painter, visualPlayerForSeat(0), QRect(width() / 2 - 230, height() / 2 + 78, 460, 110));

    painter.setPen(QColor(245, 248, 241));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
    painter.drawText(QRect(34, 26, 430, 26), Qt::AlignLeft, QString::fromStdString(engine_->tableStatus()));

    if (engine_->phase() == guandan::GamePhase::RoundOver) {
        drawSettlementOverlay(painter);
    }
}

void TableWidget::updateActionAnimation()
{
    if (animatingPlayer_ < 0) {
        animationTimer_->stop();
        return;
    }

    animationProgress_ = std::min<qreal>(1.0, static_cast<qreal>(animationClock_.elapsed()) / kActionAnimationMs);
    if (animationProgress_ >= 1.0) {
        animationTimer_->stop();
    }
    update();
}

void TableWidget::mousePressEvent(QMouseEvent* event)
{
    if (!engine_ || engine_->phase() != guandan::GamePhase::Playing) {
        return;
    }
    if (canUseArrangeButton() && arrangeButtonRect_.contains(event->pos())) {
        emit arrangeRequested();
        return;
    }
    for (auto it = clickableCards_.rbegin(); it != clickableCards_.rend(); ++it) {
        const int id = it->first;
        const QRect& rect = it->second;
        if (rect.contains(event->pos())) {
            if (selectedIds_.count(id)) {
                selectedIds_.erase(id);
            } else {
                selectedIds_.insert(id);
            }
            emit selectionChanged();
            update();
            return;
        }
    }
}

QRect TableWidget::cardRectForIndex(int index, int count, const QRect& area, bool selected) const
{
    const int overlapWidth = std::max(24, std::min(kCardWidth, area.width() / std::max(1, count)));
    const int fullWidth = overlapWidth * (count - 1) + kCardWidth;
    const int startX = area.center().x() - fullWidth / 2;
    const int y = area.top() + (area.height() - kCardHeight) / 2 - (selected ? 18 : 0);
    return QRect(startX + index * overlapWidth, y, kCardWidth, kCardHeight);
}

void TableWidget::drawPlayerHand(QPainter& painter, int playerId, const QRect& area, bool faceUp, bool interactive)
{
    const guandan::Player& player = engine_->player(playerId);
    const CardBackStyle backStyle = backStyleForPlayer(playerId);

    painter.setPen(QColor(245, 248, 241));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(area.adjusted(0, -24, 0, -area.height()), Qt::AlignCenter, playerLabel(playerId));

    if (player.finished) {
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14, QFont::Bold));
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("已出完"));
        return;
    }

    const int count = static_cast<int>(player.hand.size());
    std::vector<QRect> cardRects;
    cardRects.reserve(player.hand.size());
    for (int i = 0; i < count; ++i) {
        const guandan::Card& card = player.hand[i];
        const bool selected = selectedIds_.count(card.id) > 0 && interactive;
        const QRect cardRect = cardRectForIndex(i, count, area, selected);
        cardRects.push_back(cardRect);
        drawCard(painter, cardRect, card, faceUp, selected, backStyle);
        if (interactive && faceUp) {
            clickableCards_.push_back({ card.id, cardRect });
        }
    }

    if (interactive && faceUp) {
        drawHandGroupHints(painter, player.hand, cardRects);
    }

    if (!faceUp) {
        painter.setPen(QColor(245, 248, 241));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10));
        painter.drawText(area.adjusted(0, kCardHeight + 2, 0, 0), Qt::AlignCenter,
                         QStringLiteral("余牌 %1").arg(count));
    }
}

void TableWidget::drawHandGroupHints(QPainter& painter, const std::vector<guandan::Card>& hand, const std::vector<QRect>& cardRects)
{
    if (!engine_ || hand.empty() || hand.size() != cardRects.size()) {
        return;
    }

    const guandan::ArrangedHand arranged = guandan::HandAnalyzer::arrangeHandWithGroups(hand, engine_->currentLevel());
    if (!sameCardOrder(hand, arranged.cards)) {
        return;
    }

    int visibleGroupIndex = 0;
    for (const guandan::ArrangedGroup& group : arranged.groups) {
        if (!shouldShowGroupHint(group)) {
            continue;
        }
        if (group.startIndex < 0 || group.cardCount <= 0 ||
            group.startIndex + group.cardCount > static_cast<int>(cardRects.size())) {
            continue;
        }

        const QRect first = cardRects[static_cast<std::size_t>(group.startIndex)];
        const QRect last = cardRects[static_cast<std::size_t>(group.startIndex + group.cardCount - 1)];
        const int left = first.left() + 3;
        const int right = last.right() - 3;
        if (right <= left) {
            continue;
        }

        const QColor color = groupHintColor(visibleGroupIndex++);
        const QRect labelRect(left, std::max(first.bottom() - 17, first.top() + 68), right - left, 22);

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(labelRect, 7, 7);
        painter.fillPath(path, QColor(color.red(), color.green(), color.blue(), 216));
        painter.setPen(QPen(QColor(31, 37, 38, 218), 1));
        painter.drawPath(path);

        painter.setPen(QColor(23, 31, 34));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9, QFont::Bold));
        painter.drawText(labelRect.adjusted(5, 0, -5, 0),
                         Qt::AlignCenter,
                         QString::fromStdString(group.analysis.typeName()));

        painter.setPen(QPen(color.lighter(130), 3));
        painter.drawLine(QPoint(left + 3, first.bottom() + 4), QPoint(right - 3, first.bottom() + 4));
        painter.restore();
    }
}

void TableWidget::drawLastCards(QPainter& painter, int playerId, const QRect& area)
{
    const std::vector<guandan::Card>& cards = engine_->lastShownCards()[playerId];
    const guandan::PlayerAction& action = engine_->lastActions()[playerId];
    if (cards.empty()) {
        drawActionText(painter, playerId, area);
        return;
    }

    const bool animating = playerId == animatingPlayer_ && !action.pass;
    const qreal progress = animating ? animationProgress_ : 1.0;
    const QPoint offset = animating ? animationOffsetForPlayer(playerId) * (1.0 - progress) : QPoint();
    const int shake = action.bomb && animating
        ? static_cast<int>(std::sin(progress * kPi * 10.0) * (1.0 - progress) * 7.0)
        : 0;

    painter.save();
    painter.setOpacity(0.45 + 0.55 * progress);

    if (action.bomb) {
        drawBombEffect(painter, area, progress, offset);
    }

    painter.setPen(QColor(236, 240, 230));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    painter.drawText(area.adjusted(0, -20, 0, 0).translated(offset), Qt::AlignTop | Qt::AlignHCenter,
                     QString::fromStdString(action.text.empty() ? std::string("出牌") : action.text));

    const int count = static_cast<int>(cards.size());
    const int miniWidth = 46;
    const int miniHeight = 66;
    const int spacing = std::min(34, area.width() / std::max(1, count));
    const int fullWidth = spacing * (count - 1) + miniWidth;
    int x = area.center().x() - fullWidth / 2 + offset.x();
    for (int i = 0; i < count; ++i) {
        const guandan::Card& card = cards[i];
        const int jitter = shake == 0 ? 0 : (i % 2 == 0 ? shake : -shake);
        QRect rect(x + jitter, area.top() + 20 + offset.y(), miniWidth, miniHeight);
        drawCard(painter, rect, card, true, false);
        x += spacing;
    }
    painter.restore();
}

void TableWidget::drawActionText(QPainter& painter, int playerId, const QRect& area)
{
    const guandan::PlayerAction& action = engine_->lastActions()[playerId];
    if (action.text.empty()) {
        return;
    }

    const bool animating = playerId == animatingPlayer_;
    const qreal progress = animating ? animationProgress_ : 1.0;
    const QPoint offset = animating ? animationOffsetForPlayer(playerId) * (1.0 - progress) : QPoint();

    const QRect bubble = QRect(area.center().x() - 56 + offset.x(),
                               area.center().y() - 18 + offset.y(),
                               112,
                               36);

    painter.save();
    painter.setOpacity(0.40 + 0.60 * progress);

    QPainterPath path;
    path.addRoundedRect(bubble, 10, 10);
    const QColor fill = action.pass ? QColor(25, 35, 42, 205) : QColor(250, 246, 222, 220);
    painter.fillPath(path, fill);
    painter.setPen(QPen(action.pass ? QColor(226, 232, 220) : QColor(75, 52, 22), 1));
    painter.drawPath(path);

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
    painter.drawText(bubble, Qt::AlignCenter, QString::fromStdString(action.text));
    painter.restore();
}

void TableWidget::drawBombEffect(QPainter& painter, const QRect& area, qreal progress, const QPoint& offset)
{
    const qreal fade = std::max<qreal>(0.0, 1.0 - progress);
    if (fade <= 0.0) {
        return;
    }

    const QPointF center = area.center() + offset;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(fade);

    const qreal radius = 24.0 + 84.0 * progress;
    painter.setBrush(QColor(255, 210, 68, static_cast<int>(115 * fade)));
    painter.setPen(QPen(QColor(255, 246, 174, static_cast<int>(210 * fade)), 3));
    painter.drawEllipse(center, radius * 0.62, radius * 0.35);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 112, 48, static_cast<int>(230 * fade)), 5));
    painter.drawEllipse(center, radius, radius * 0.58);
    painter.setPen(QPen(QColor(255, 232, 96, static_cast<int>(190 * fade)), 2));
    painter.drawEllipse(center, radius * 1.25, radius * 0.72);

    painter.setPen(QPen(QColor(255, 236, 128, static_cast<int>(230 * fade)), 3));
    for (int i = 0; i < 14; ++i) {
        const qreal angle = (kPi * 2.0 * i / 14.0) + progress * 0.7;
        const qreal inner = 28.0 + 30.0 * progress;
        const qreal outer = 58.0 + 74.0 * progress;
        const QPointF start(center.x() + std::cos(angle) * inner,
                            center.y() + std::sin(angle) * inner * 0.58);
        const QPointF end(center.x() + std::cos(angle) * outer,
                          center.y() + std::sin(angle) * outer * 0.58);
        painter.drawLine(start, end);
    }

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 15, QFont::Bold));
    painter.setPen(QColor(82, 28, 9, static_cast<int>(235 * fade)));
    painter.drawText(QRectF(center.x() - 48, center.y() - 16, 96, 32), Qt::AlignCenter, QStringLiteral("炸弹"));
    painter.restore();
}

void TableWidget::drawSettlementOverlay(QPainter& painter)
{
    const std::vector<int>& order = engine_->finishOrder();
    if (order.size() != 4) {
        return;
    }

    const int winningTeam = engine_->lastRoundWinningTeam();
    const int upgradeAmount = engine_->lastRoundUpgradeAmount();
    const guandan::Rank nextLevel = winningTeam >= 0 ? engine_->teamLevels()[winningTeam] : engine_->currentLevel();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(8, 16, 22, 142));

    const int panelWidth = std::min(620, width() - 120);
    const int panelHeight = 430;
    const QRect panel(width() / 2 - panelWidth / 2,
                      height() / 2 - panelHeight / 2,
                      panelWidth,
                      panelHeight);

    QPainterPath shadow;
    shadow.addRoundedRect(panel.adjusted(0, 8, 0, 12), 16, 16);
    painter.fillPath(shadow, QColor(0, 0, 0, 76));

    QPainterPath panelPath;
    panelPath.addRoundedRect(panel, 16, 16);
    QLinearGradient gradient(panel.topLeft(), panel.bottomRight());
    gradient.setColorAt(0.0, QColor(252, 248, 232));
    gradient.setColorAt(1.0, QColor(224, 237, 222));
    painter.fillPath(panelPath, gradient);
    painter.setPen(QPen(QColor(50, 78, 58), 2));
    painter.drawPath(panelPath);

    painter.setPen(QColor(28, 40, 34));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 26, QFont::Bold));
    painter.drawText(QRect(panel.left(), panel.top() + 26, panel.width(), 46),
                     Qt::AlignCenter,
                     QStringLiteral("本副结算"));

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 15, QFont::Bold));
    painter.setPen(QColor(58, 91, 60));
    painter.drawText(QRect(panel.left() + 36, panel.top() + 86, panel.width() - 72, 34),
                     Qt::AlignCenter,
                     QStringLiteral("队伍%1获胜  升级%2级  下一副打%3")
                         .arg(winningTeam)
                         .arg(upgradeAmount)
                         .arg(qLevel(nextLevel)));

    const QRect ranking(panel.left() + 60, panel.top() + 145, panel.width() - 120, 190);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        const int playerId = order[i];
        const guandan::Player& player = engine_->player(playerId);
        const QRect row(ranking.left(), ranking.top() + i * 44, ranking.width(), 34);
        const QColor rowColor = i == 0 ? QColor(229, 194, 82, 210) : QColor(255, 255, 255, 150);

        QPainterPath rowPath;
        rowPath.addRoundedRect(row, 8, 8);
        painter.fillPath(rowPath, rowColor);

        painter.setPen(QColor(32, 43, 38));
        painter.drawText(row.adjusted(16, 0, -16, 0),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         QStringLiteral("%1  %2").arg(placeText(i + 1), QString::fromStdString(player.name)));
        painter.drawText(row.adjusted(16, 0, -16, 0),
                         Qt::AlignVCenter | Qt::AlignRight,
                         QStringLiteral("队伍%1").arg(playerId % 2));
    }

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
    painter.setPen(QColor(65, 78, 68));
    painter.drawText(QRect(panel.left() + 48, panel.bottom() - 62, panel.width() - 96, 28),
                     Qt::AlignCenter,
                     QStringLiteral("第%1副结束").arg(engine_->dealNumber()));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10));
    painter.drawText(QRect(panel.left() + 48, panel.bottom() - 36, panel.width() - 96, 22),
                     Qt::AlignCenter,
                     QStringLiteral("队伍0：%1    队伍1：%2")
                         .arg(qLevel(engine_->teamLevels()[0]), qLevel(engine_->teamLevels()[1])));

    painter.restore();
}

void TableWidget::drawArrangeButton(QPainter& painter, const QRect& playerArea)
{
    if (!canUseArrangeButton()) {
        arrangeButtonRect_ = {};
        return;
    }

    arrangeButtonRect_ = QRect(playerArea.right() - 126,
                               playerArea.bottom() - 36,
                               116,
                               32);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath shadow;
    shadow.addRoundedRect(arrangeButtonRect_.translated(0, 3), 9, 9);
    painter.fillPath(shadow, QColor(0, 0, 0, 70));

    QPainterPath button;
    button.addRoundedRect(arrangeButtonRect_, 9, 9);
    QLinearGradient gradient(arrangeButtonRect_.topLeft(), arrangeButtonRect_.bottomLeft());
    gradient.setColorAt(0.0, QColor(255, 243, 150));
    gradient.setColorAt(1.0, QColor(239, 176, 55));
    painter.fillPath(button, gradient);
    painter.setPen(QPen(QColor(87, 61, 19), 1));
    painter.drawPath(button);

    painter.setPen(QColor(43, 34, 18));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(arrangeButtonRect_, Qt::AlignCenter, QStringLiteral("一键理牌"));
    painter.restore();
}

void TableWidget::drawCard(QPainter& painter, const QRect& rect, const guandan::Card& card, bool faceUp, bool selected,
                           CardBackStyle backStyle)
{
    const bool wild = engine_ && guandan::isWildCard(card, engine_->currentLevel());
    QPixmap pixmap = cardCache().pixmap(card, faceUp, static_cast<int>(engine_->currentLevel()), wild, backStyle);
    painter.drawPixmap(rect, pixmap);
    if (selected) {
        painter.setPen(QPen(QColor(255, 220, 70), 3));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 8, 8);
    }
}

QString TableWidget::playerLabel(int playerId) const
{
    const guandan::Player& player = engine_->player(playerId);
    const QString team = playerId % 2 == 0 ? QStringLiteral("队伍0") : QStringLiteral("队伍1");
    return QString::fromStdString(player.name) + QStringLiteral(" / ") + team +
           QStringLiteral(" / 余牌 %1").arg(player.hand.size());
}

int TableWidget::visualPlayerForSeat(int seat) const
{
    if (!engine_) {
        return seat;
    }
    if (engine_->mode() == guandan::GameMode::LocalFour) {
        return (engine_->currentPlayer() + seat) % 4;
    }
    if (seat == 0) {
        return 0;
    }
    return seat;
}

CardBackStyle TableWidget::backStyleForPlayer(int playerId) const
{
    if (!engine_) {
        return CardBackStyle::Neutral;
    }

    const int viewpointPlayer = visualPlayerForSeat(0);
    if (playerId == viewpointPlayer) {
        return CardBackStyle::Neutral;
    }

    return guandan::sameTeam(playerId, viewpointPlayer)
        ? CardBackStyle::Ally
        : CardBackStyle::Opponent;
}

bool TableWidget::canUseArrangeButton() const
{
    return engine_ &&
           engine_->phase() == guandan::GamePhase::Playing &&
           engine_->isCurrentPlayerHuman();
}

QPoint TableWidget::animationOffsetForPlayer(int playerId) const
{
    int seat = playerId;
    if (engine_ && engine_->mode() == guandan::GameMode::LocalFour) {
        seat = (playerId - engine_->currentPlayer() + 4) % 4;
    }

    switch (seat) {
    case 0: return QPoint(0, 86);
    case 1: return QPoint(-120, 0);
    case 2: return QPoint(0, -90);
    case 3: return QPoint(120, 0);
    default: return QPoint();
    }
}
