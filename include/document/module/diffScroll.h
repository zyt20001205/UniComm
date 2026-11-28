#ifndef UNICOMM_DIFFSCROLL_H
#define UNICOMM_DIFFSCROLL_H

#include <algorithm>
#include <array>
#include <iterator>

#include "scintillaWidget.h"

namespace DiffScroll {
    using Anchors = QList<std::array<int, 2>>;

    inline void sync(ScintillaWidget *editor, ScintillaWidget *target, const Anchors &anchors, const bool reverse) {
        if (anchors.isEmpty()) return;
        const int src = reverse ? 1 : 0;
        const int dst = 1 - src;
        const auto reference = editor->firstVisibleLine() + editor->linesOnScreen() / 3;
        const int line = qBound(0, static_cast<int>(editor->docLineFromVisible(reference)), editor->lineCountGet() - 1);
        const auto next = std::ranges::lower_bound(anchors, line, {}, [src](const auto &anchor) { return anchor[src]; });
        int targetLine;
        if (next != anchors.cend() && (*next)[src] == line) {
            targetLine = (*next)[dst];
        } else {
            const auto &start = *std::prev(next);
            targetLine = start[dst] + (line - start[src]);
            if (next != anchors.cend()) targetLine = qMin(targetLine, (*next)[dst]);
        }
        const auto displayLine = target->visibleFromDocLine(qMin(targetLine, target->lineCountGet() - 1));
        target->setFirstVisibleLine(qMax<sptr_t>(0, displayLine - target->linesOnScreen() / 3));
    }
}

#endif // UNICOMM_DIFFSCROLL_H
