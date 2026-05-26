#pragma once

#include <QStyledItemDelegate>

class ThumbnailDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static constexpr int IconZoneHeight = 128;

    explicit ThumbnailDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};
