#include "thumbnaildelegate.h"

#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QTextOption>
#include <QFontMetrics>

ThumbnailDelegate::ThumbnailDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ThumbnailDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    painter->save();

    // Folder items use the default delegate rendering unchanged
    if (index.data(Qt::UserRole).toBool()) {
        painter->restore();
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Draw selection / hover background
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    QStyleOptionViewItem bgOption = option;
    bgOption.showDecorationSelected = true;
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &bgOption, painter, option.widget);

    const QRect cellRect = option.rect;

    // Fixed icon zone: top IconZoneHeight pixels of the cell
    const QRect iconZone(cellRect.left(), cellRect.top(),
                         cellRect.width(), IconZoneHeight);

    // Fixed text zone: everything below the icon zone
    const QRect textZone(cellRect.left(), cellRect.top() + IconZoneHeight,
                         cellRect.width(), cellRect.height() - IconZoneHeight);

    // --- Draw icon centered within the icon zone ---
    const QVariant decorData = index.data(Qt::DecorationRole);
    QPixmap pixmap;
    if (decorData.userType() == QMetaType::QPixmap) {
        pixmap = qvariant_cast<QPixmap>(decorData);
    } else if (decorData.userType() == QMetaType::QIcon) {
        pixmap = qvariant_cast<QIcon>(decorData).pixmap(option.decorationSize);
    }
    if (!pixmap.isNull()) {
        // Center the pixmap in the icon zone
        const int x = iconZone.left() + (iconZone.width()  - pixmap.width())  / 2;
        const int y = iconZone.top()  + (iconZone.height() - pixmap.height()) / 2;
        painter->drawPixmap(x, y, pixmap);
    }

    // --- Draw text top-aligned within the text zone ---
    const QString text = index.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        QColor textColor = option.palette.color(
            (option.state & QStyle::State_Selected) ? QPalette::HighlightedText
                                                    : QPalette::Text);
        painter->setPen(textColor);
        painter->setFont(option.font);

        QTextOption textOpt;
        textOpt.setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        textOpt.setWrapMode(QTextOption::WordWrap);

        // Elide text that would overflow the text zone
        const QFontMetrics fm(option.font);
        const QString elidedText = fm.elidedText(
            text, option.textElideMode, textZone.width() * 2);

        painter->drawText(QRectF(textZone), elidedText, textOpt);
    }

    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    // Return the grid size set on the view (148×168)
    return option.rect.size().isValid() ? option.rect.size()
                                        : QStyledItemDelegate::sizeHint(option, index);
}
