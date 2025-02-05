/**
 *  (C) Copyright 2025 Elysia. All Rights Reserved.
 *  Description：defines
 *  Author：Elysia
 *  Date: 25-2-5
 *  Modify Record:
 */
#ifndef LINGMO_DOCK_DEFINES_H
#define LINGMO_DOCK_DEFINES_H

#include <QObject>

namespace Lingmo::DockIcons {

/**
 * These are the standard sizes for icons.
 */
enum StdSizes {
    /// small icons for menu entries
    SizeSmall = 16,
    /// slightly larger small icons for toolbars, panels, etc
    SizeSmallMedium = 22,
    /// medium sized icons for the desktop
    SizeMedium = 32,
    /// large sized icons for the panel
    SizeLarge = 48,
    /// huge sized icons for iconviews
    SizeHuge = 64,
    /// enormous sized icons for iconviews
    SizeEnormous = 128,
};
}

#endif // LINGMO_DOCK_DEFINES_H
