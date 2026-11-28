#ifndef UNICOMM_DIFFOVERLAY_H
#define UNICOMM_DIFFOVERLAY_H

#include <optional>
#include <QQuickWidget>
#include <QTimer>

#include "types/diffTypes.h"

class ScintillaWidget;

class DiffOverlay final : public QQuickWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList regions READ regionsGet NOTIFY regionsChanged)

public:
    struct Range {
        int start;
        int count;
    };

    struct Block {
        QList<std::optional<Range>> ranges{};
        DiffKind::Value kind{DiffKind::Modified};
    };

    DiffOverlay(const QList<ScintillaWidget *> &editors, QWidget *parent);

    void blocksSet(QList<Block> blocks);

    [[nodiscard]] QVariantList regionsGet() const;

signals:
    void regionsChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QList<ScintillaWidget *> m_editors;
    QList<Block> m_blocks{};
    QTimer m_refresh{};
};

#endif // UNICOMM_DIFFOVERLAY_H
