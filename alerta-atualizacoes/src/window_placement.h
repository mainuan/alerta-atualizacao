#pragma once

#include <QMargins>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <Qt>

struct LayerShellSettings {
    Qt::Edges anchors;
    QMargins margins;
    int exclusiveZone;
    // The alert must not take the keyboard focus from what the user is doing
    // (it appears at login, possibly together with other startup windows).
    bool activateOnShow;
};

[[nodiscard]] QPoint bottomRightPosition(const QRect &availableGeometry,
                                         const QSize &windowSize,
                                         int margin = 24);
[[nodiscard]] LayerShellSettings notificationLayerShellSettings();
