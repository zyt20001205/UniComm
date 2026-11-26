#ifndef UNICOMM_DIFFOVERLAY_H
#define UNICOMM_DIFFOVERLAY_H

#include <array>
#include <optional>
#include <QQuickWidget>
#include <QTimer>

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
        std::array<std::optional<Range>, 3> ranges{};
        QString color{};
    };

    DiffOverlay(ScintillaWidget *ours, ScintillaWidget *base, ScintillaWidget *theirs, QWidget *parent);

    void blocksSet(QList<Block> blocks);

    [[nodiscard]] QVariantList regionsGet() const;

signals:
    void regionsChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::array<ScintillaWidget *, 3> m_editors;
    QList<Block> m_blocks{};
    QTimer m_refresh{};
};

#endif // UNICOMM_DIFFOVERLAY_H
