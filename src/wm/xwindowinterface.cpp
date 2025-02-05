/*
 * Copyright (C) 2021 LingmoOS Team.
 *
 * Author:     rekols <revenmartin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xwindowinterface.h"
#include "global/defines.h"
#include "utils.h"
#include "wm/windowinfowrap.h"

#include <QDebug>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtGui/private/qtx11extras_p.h>

#include <KWindowEffects>
#include <KWindowInfo>
#include <KWindowSystem>

// X11
#include <KX11Extras>
#include <NETWM>
#include <xcb/shape.h>
#include <xcb/xcb.h>

XWindowInterface::XWindowInterface(QObject* parent)
    : AbstractWindowInterface(parent)
{
    m_currentDesktop = QString::number(KX11Extras::self()->currentDesktop());

    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &AbstractWindowInterface::activeWindowChanged);
    connect(KX11Extras::self(), &KX11Extras::windowRemoved, this, &AbstractWindowInterface::windowRemoved);

    connect(KX11Extras::self(), &KX11Extras::windowAdded, this, &XWindowInterface::windowAddedProxy);

    connect(KX11Extras::self(), &KX11Extras::currentDesktopChanged, this, [&](int desktop) {
        m_currentDesktop = QString::number(desktop);
        emit currentDesktopChanged();
    });

    // connect(KX11Extras::self(), static_cast<void (KX11Extras::*)(WId, NET::Properties, NET::Properties2)>(&KX11Extras::windowChanged), this, &XWindowInterface::windowChangedProxy);

    for (auto wid : KX11Extras::self()->windows()) {
        windowAddedProxy(wid);
    }
    connect(KX11Extras::self(), &KX11Extras::windowAdded, this, &XWindowInterface::onWindowadded);
    connect(KX11Extras::self(), &KX11Extras::windowRemoved, this, &XWindowInterface::windowRemoved);
    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &XWindowInterface::activeChanged);
}

void XWindowInterface::setViewStruts(QWindow& view, DockSettings::Direction direction, const QRect& rect, bool compositing)
{
    NETExtendedStrut strut;

    const auto screen = view.screen();

    bool isRound = DockSettings::self()->style() == DockSettings::Round;
    const int edgeMargins = compositing && isRound ? DockSettings::self()->edgeMargins() : 0;

    switch (direction) {
    case DockSettings::Left: {
        const int leftOffset = { screen->geometry().left() };
        strut.left_width = rect.width() + leftOffset + edgeMargins;
        strut.left_start = rect.y();
        strut.left_end = rect.y() + rect.height() - 1;
        break;
    }
    case DockSettings::Bottom: {
        strut.bottom_width = rect.height() + edgeMargins;
        strut.bottom_start = rect.x();
        strut.bottom_end = rect.x() + rect.width();
        break;
    }
    case DockSettings::Right: {
        // const int rightOffset = {wholeScreen.right() - currentScreen.right()};
        strut.right_width = rect.width() + edgeMargins;
        strut.right_start = rect.y();
        strut.right_end = rect.y() + rect.height() - 1;
        break;
    }
    default:
        break;
    }

    KX11Extras::setExtendedStrut(view.winId(),
        strut.left_width, strut.left_start, strut.left_end,
        strut.right_width, strut.right_start, strut.right_end,
        strut.top_width, strut.top_start, strut.top_end,
        strut.bottom_width, strut.bottom_start, strut.bottom_end);
}

void XWindowInterface::removeViewStruts(QWindow& view)
{
    KX11Extras::setStrut(view.winId(), 0, 0, 0, 0);
}

WindowId XWindowInterface::activeWindow()
{
    return KX11Extras::self()->activeWindow();
}

void XWindowInterface::skipTaskBar(const QDialog& dialog)
{
    KX11Extras::setState(dialog.winId(), NET::SkipTaskbar);
}

void XWindowInterface::slideWindow(QWindow& view, AbstractWindowInterface::Slide location)
{
    auto slideLocation = KWindowEffects::NoEdge;

    switch (location) {
    case Slide::Top:
        slideLocation = KWindowEffects::TopEdge;
        break;

    case Slide::Bottom:
        slideLocation = KWindowEffects::BottomEdge;
        break;

    case Slide::Left:
        slideLocation = KWindowEffects::LeftEdge;
        break;

    case Slide::Right:
        slideLocation = KWindowEffects::RightEdge;
        break;

    default:
        break;
    }

    KWindowEffects::slideWindow(&view, slideLocation, -1);
}

void XWindowInterface::enableBlurBehind(QWindow& view)
{
    KWindowEffects::enableBlurBehind(&view);
}

void XWindowInterface::enableBlurBehind(QWindow& view, bool enable, const QRegion& region)
{
    KWindowEffects::enableBlurBehind(&view, enable, region);
}

void XWindowInterface::requestActivate(WindowId wid)
{
    KX11Extras::activateWindow(wid.toInt());
}

void XWindowInterface::forceActiveWindow(WindowId win)
{
    KX11Extras::forceActiveWindow(win.toUInt());
}

void XWindowInterface::requestClose(WindowId wid)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!wInfo.isValid()) {
        return;
    }

    NETRootInfo ri(QX11Info::connection(), NET::CloseWindow);
    ri.closeWindowRequest(wInfo.wid().toUInt());
}

void XWindowInterface::closeWindow(WId id)
{
    // FIXME: Why there is no such thing in KWindowSystem??
    NETRootInfo(QX11Info::connection(), NET::CloseWindow).closeWindowRequest(id);
}

void XWindowInterface::requestMoveWindow(WindowId wid, QPoint from)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!wInfo.isValid() || !inCurrentDesktopActivity(wInfo)) {
        return;
    }

    int borderX = wInfo.geometry().width() > 120 ? 60 : 10;
    int borderY { 10 };

    //! find min/max values for x,y based on active window geometry
    int minX = wInfo.geometry().x() + borderX;
    int maxX = wInfo.geometry().x() + wInfo.geometry().width() - borderX;
    int minY = wInfo.geometry().y() + borderY;
    int maxY = wInfo.geometry().y() + wInfo.geometry().height() - borderY;

    //! set the point from which this window will be moved,
    //! make sure that it is in window boundaries
    int validX = qBound(minX, from.x(), maxX);
    int validY = qBound(minY, from.y(), maxY);

    NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(wInfo.wid().toUInt(), validX, validY, NET::Move);
}

void XWindowInterface::requestToggleIsOnAllDesktops(WindowId wid)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!wInfo.isValid()) {
        return;
    }

    if (KX11Extras::numberOfDesktops() <= 1) {
        return;
    }

    if (wInfo.isOnAllDesktops()) {
        KX11Extras::setOnDesktop(wid.toUInt(), KX11Extras::currentDesktop());
        KX11Extras::forceActiveWindow(wid.toUInt());
    } else {
        KX11Extras::setOnAllDesktops(wid.toUInt(), true);
    }
}

void XWindowInterface::requestToggleKeepAbove(WindowId wid)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!wInfo.isValid()) {
        return;
    }

    NETWinInfo ni(QX11Info::connection(), wid.toUInt(), QX11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (wInfo.isKeepAbove()) {
        ni.setState(NET::States(), NET::KeepAbove);
    } else {
        ni.setState(NET::KeepAbove, NET::KeepAbove);
    }
}

void XWindowInterface::requestToggleMinimized(WindowId wid)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!wInfo.isValid() || !inCurrentDesktopActivity(wInfo)) {
        return;
    }

    if (wInfo.isMinimized()) {
        bool onCurrent = wInfo.isOnDesktop(m_currentDesktop);

        KX11Extras::unminimizeWindow(wid.toUInt());

        if (onCurrent) {
            KX11Extras::forceActiveWindow(wid.toUInt());
        }
    } else {
        KX11Extras::minimizeWindow(wid.toUInt());
    }
}

void XWindowInterface::minimizeWindow(WindowId win)
{
    KX11Extras::minimizeWindow(win.toUInt());
}

void XWindowInterface::requestToggleMaximized(WindowId wid)
{
    WindowInfoWrap wInfo = requestInfo(wid);

    if (!windowCanBeMaximized(wid) || !inCurrentDesktopActivity(wInfo)) {
        return;
    }

    bool restore = wInfo.isMaxHoriz() && wInfo.isMaxVert();

    if (wInfo.isMinimized()) {
        KX11Extras::unminimizeWindow(wid.toUInt());
    }

    NETWinInfo ni(QX11Info::connection(), wid.toInt(), QX11Info::appRootWindow(), NET::WMState, NET::Properties2());

    if (restore) {
        ni.setState(NET::States(), NET::Max);
    } else {
        ni.setState(NET::Max, NET::Max);
    }
}

void XWindowInterface::setKeepAbove(WindowId wid, bool active)
{
    if (wid.toUInt() <= 0) {
        return;
    }

    if (active) {
        KX11Extras::setState(wid.toUInt(), NET::KeepAbove);
        KX11Extras::clearState(wid.toUInt(), NET::KeepBelow);
    } else {
        KX11Extras::clearState(wid.toUInt(), NET::KeepAbove);
    }
}

void XWindowInterface::setKeepBelow(WindowId wid, bool active)
{
    if (wid.toUInt() <= 0) {
        return;
    }

    if (active) {
        KX11Extras::setState(wid.toUInt(), NET::KeepBelow);
        KX11Extras::clearState(wid.toUInt(), NET::KeepAbove);
    } else {
        KX11Extras::clearState(wid.toUInt(), NET::KeepBelow);
    }
}

bool XWindowInterface::windowCanBeDragged(WindowId wid)
{
    WindowInfoWrap winfo = requestInfo(wid);
    return (winfo.isValid()
        && !winfo.isMinimized()
        && winfo.isMovable()
        && inCurrentDesktopActivity(winfo));
}

bool XWindowInterface::windowCanBeMaximized(WindowId wid)
{
    WindowInfoWrap winfo = requestInfo(wid);
    return (winfo.isValid()
        && !winfo.isMinimized()
        && winfo.isMaximizable()
        && inCurrentDesktopActivity(winfo));
}

QIcon XWindowInterface::iconFor(WindowId wid)
{
    QIcon icon;
    using namespace Lingmo::DockIcons;
    icon.addPixmap(KX11Extras::icon(wid.value<WId>(), StdSizes::SizeSmall, StdSizes::SizeSmall, false));
    icon.addPixmap(KX11Extras::icon(wid.value<WId>(), StdSizes::SizeSmallMedium, StdSizes::SizeSmallMedium, false));
    icon.addPixmap(KX11Extras::icon(wid.value<WId>(), StdSizes::SizeMedium, StdSizes::SizeMedium, false));
    icon.addPixmap(KX11Extras::icon(wid.value<WId>(), StdSizes::SizeLarge, StdSizes::SizeLarge, false));

    return icon;
}

WindowId XWindowInterface::winIdFor(QString appId, QRect geometry)
{
    Q_UNUSED(appId);
    Q_UNUSED(geometry);
    return activeWindow();
}

WindowId XWindowInterface::winIdFor(QString appId, QString title)
{
    Q_UNUSED(appId);
    Q_UNUSED(title);
    return activeWindow();
}

void XWindowInterface::switchToNextVirtualDesktop()
{
    int desktops = KX11Extras::numberOfDesktops();

    if (desktops <= 1) {
        return;
    }

    int curPos = KX11Extras::currentDesktop();
    int nextPos = curPos + 1;

    if (curPos == desktops) {
        if (isVirtualDesktopNavigationWrappingAround()) {
            nextPos = 1;
        } else {
            return;
        }
    }

    KX11Extras::setCurrentDesktop(nextPos);
}

void XWindowInterface::switchToPreviousVirtualDesktop()
{
    int desktops = KX11Extras::numberOfDesktops();
    if (desktops <= 1) {
        return;
    }

    int curPos = KX11Extras::currentDesktop();
    int nextPos = curPos - 1;

    if (curPos == 1) {
        if (isVirtualDesktopNavigationWrappingAround()) {
            nextPos = desktops;
        } else {
            return;
        }
    }

    KX11Extras::setCurrentDesktop(nextPos);
}

void XWindowInterface::setFrameExtents(QWindow* view, const QMargins& margins)
{
    if (!view) {
        return;
    }

    NETWinInfo ni(QX11Info::connection(),
        view->winId(),
        QX11Info::appRootWindow(),
        static_cast<NET::Property>(0),
        NET::WM2GTKFrameExtents);

    if (margins.isNull()) {
        //! delete property
        xcb_connection_t* c = QX11Info::connection();
        const QByteArray atomName = QByteArrayLiteral("_GTK_FRAME_EXTENTS");
        xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(c, false, atomName.length(), atomName.constData());
        QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atom(xcb_intern_atom_reply(c, atomCookie, nullptr));

        if (!atom) {
            return;
        }

        // qDebug() << "   deleting gtk frame extents atom..";

        xcb_delete_property(c, view->winId(), atom->atom);
    } else {
        NETStrut struts;
        struts.left = margins.left();
        struts.top = margins.top();
        struts.right = margins.right();
        struts.bottom = margins.bottom();

        ni.setGtkFrameExtents(struts);
    }

    /*NETWinInfo ni2(QX11Info::connection(), view->winId(), QX11Info::appRootWindow(), 0, NET::WM2GTKFrameExtents);
      NETStrut applied = ni2.gtkFrameExtents();
      QMargins amargins(applied.left, applied.top, applied.right, applied.bottom);
      qDebug() << "     window gtk frame extents applied :: " << amargins;*/
}

void XWindowInterface::setInputMask(QWindow* window, const QRect& rect)
{
    if (!window || !window->isVisible()) {
        return;
    }

    xcb_connection_t* c = QX11Info::connection();

    if (!m_shapeExtensionChecked) {
        checkShapeExtension();
    }

    if (!m_shapeAvailable) {
        return;
    }

    if (!rect.isEmpty()) {
        xcb_rectangle_t xcbrect;
        xcbrect.x = qMax(SHRT_MIN, rect.x());
        xcbrect.y = qMax(SHRT_MIN, rect.y());
        xcbrect.width = qMin((int)USHRT_MAX, rect.width());
        xcbrect.height = qMin((int)USHRT_MAX, rect.height());

        // set input shape, so that it doesn't accept any input events
        xcb_shape_rectangles(c, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT,
            XCB_CLIP_ORDERING_UNSORTED, window->winId(), 0, 0, 1, &xcbrect);
    } else {
        // delete the shape
        xcb_shape_mask(c, XCB_SHAPE_SO_INTERSECT, XCB_SHAPE_SK_INPUT,
            window->winId(), 0, 0, XCB_PIXMAP_NONE);
    }
}

bool XWindowInterface::isAcceptableWindow(WindowId wid)
{
    const KWindowInfo info(wid.toUInt(), NET::WMGeometry | NET::WMState, NET::WM2WindowClass);

    const auto winClass = QString(info.windowClassName());

    //! ignored windows do not trackd
    if (hasBlockedTracking(wid)) {
        return false;
    }

    //! whitelisted/approved windows
    if (isWhitelistedWindow(wid)) {
        return true;
    }

    //! Window Checks
    bool hasSkipTaskbar = info.hasState(NET::SkipTaskbar);
    bool hasSkipPager = info.hasState(NET::SkipPager);
    bool isSkipped = hasSkipTaskbar && hasSkipPager;

    //    if (isSkipped
    //        && ((winClass == QLatin1String("yakuake")
    //            || (winClass == QLatin1String("krunner"))))) {
    //        registerWhitelistedWindow(wid);
    //    } else if (winClass == QLatin1String("plasmashell")) {
    //        if (isSkipped && isSidepanel(info.geometry())) {
    //            registerWhitelistedWindow(wid);
    //            return true;
    //        } else if (isPlasmaPanel(info.geometry()) || isFullScreenWindow(info.geometry())) {
    //            registerPlasmaIgnoredWindow(wid);
    //            return false;
    //        }
    //    } else if ((winClass == QLatin1String("latte-dock"))
    //        || (winClass == QLatin1String("ksmserver"))) {
    //        if (isFullScreenWindow(info.geometry())) {
    //            registerIgnoredWindow(wid);
    //            return false;
    //        }
    //    }

    return !isSkipped;
}

bool XWindowInterface::isValidWindow(WindowId wid)
{
    return isAcceptableWindow(wid);
}

QRect XWindowInterface::visibleGeometry(const WindowId& wid, const QRect& frameGeometry) const
{
    NETWinInfo ni(QX11Info::connection(),
        wid.toUInt(),
        QX11Info::appRootWindow(),
        static_cast<NET::Property>(0),
        NET::WM2GTKFrameExtents);
    NETStrut struts = ni.gtkFrameExtents();
    QMargins margins(struts.left, struts.top, struts.right, struts.bottom);
    QRect visibleGeometry = frameGeometry;

    if (!margins.isNull()) {
        visibleGeometry -= margins;
    }

    return visibleGeometry;
}

void XWindowInterface::checkShapeExtension()
{
    if (!m_shapeExtensionChecked) {
        xcb_connection_t* c = QX11Info::connection();
        xcb_prefetch_extension_data(c, &xcb_shape_id);
        const xcb_query_extension_reply_t* extension = xcb_get_extension_data(c, &xcb_shape_id);
        if (extension->present) {
            // query version
            auto cookie = xcb_shape_query_version(c);
            QScopedPointer<xcb_shape_query_version_reply_t, QScopedPointerPodDeleter> version(xcb_shape_query_version_reply(c, cookie, nullptr));
            if (!version.isNull()) {
                m_shapeAvailable = (version->major_version * 0x10 + version->minor_version) >= 0x11;
            }
        }
        m_shapeExtensionChecked = true;
    }
}

void XWindowInterface::windowAddedProxy(WId wid)
{
    if (!isAcceptableWindow(wid)) {
        return;
    }

    emit windowAdded(wid);
    considerWindowChanged(wid);
}
void XWindowInterface::onWindowadded(WId wid)
{
    this->windowAddedProxy(wid);
}

///////////////////////////////////////

WindowInfoWrap XWindowInterface::requestInfo(WindowId wid)
{
    const KWindowInfo winfo { wid.value<WId>(), NET::WMFrameExtents | NET::WMWindowType | NET::WMGeometry | NET::WMDesktop | NET::WMState | NET::WMName | NET::WMVisibleName,
        NET::WM2WindowClass
            | NET::WM2Activities
            | NET::WM2AllowedActions
            | NET::WM2TransientFor };

    WindowInfoWrap winfoWrap;

    const auto winClass = QString(winfo.windowClassName());

    //! used to track Plasma DesktopView windows because during startup can not be identified properly
    bool plasmaBlockedWindow = (winClass == QLatin1String("plasmashell") && !isAcceptableWindow(wid));

    if (!winfo.valid() || plasmaBlockedWindow) {
        winfoWrap.setIsValid(false);
    } else if (isValidWindow(wid)) {
        winfoWrap.setIsValid(true);
        winfoWrap.setWid(wid);
        winfoWrap.setParentId(winfo.transientFor());
        winfoWrap.setIsActive(KX11Extras::activeWindow() == wid.value<WId>());
        winfoWrap.setIsMinimized(winfo.hasState(NET::Hidden));
        winfoWrap.setIsMaxVert(winfo.hasState(NET::MaxVert));
        winfoWrap.setIsMaxHoriz(winfo.hasState(NET::MaxHoriz));
        winfoWrap.setIsFullscreen(winfo.hasState(NET::FullScreen));
        winfoWrap.setIsShaded(winfo.hasState(NET::Shaded));
        winfoWrap.setIsOnAllDesktops(winfo.onAllDesktops());
        winfoWrap.setIsOnAllActivities(winfo.activities().empty());
        winfoWrap.setGeometry(visibleGeometry(wid, winfo.frameGeometry()));
        winfoWrap.setIsKeepAbove(winfo.hasState(NET::KeepAbove));
        winfoWrap.setIsKeepBelow(winfo.hasState(NET::KeepBelow));
        winfoWrap.setHasSkipPager(winfo.hasState(NET::SkipPager));
        winfoWrap.setHasSkipSwitcher(winfo.hasState(NET::SkipSwitcher));
        winfoWrap.setHasSkipTaskbar(winfo.hasState(NET::SkipTaskbar));

        //! BEGIN:Window Abilities
        winfoWrap.setIsClosable(winfo.actionSupported(NET::ActionClose));
        winfoWrap.setIsFullScreenable(winfo.actionSupported(NET::ActionFullScreen));
        winfoWrap.setIsMaximizable(winfo.actionSupported(NET::ActionMax));
        winfoWrap.setIsMinimizable(winfo.actionSupported(NET::ActionMinimize));
        winfoWrap.setIsMovable(winfo.actionSupported(NET::ActionMove));
        winfoWrap.setIsResizable(winfo.actionSupported(NET::ActionResize));
        winfoWrap.setIsShadeable(winfo.actionSupported(NET::ActionShade));
        winfoWrap.setIsVirtualDesktopsChangeable(winfo.actionSupported(NET::ActionChangeDesktop));
        //! END:Window Abilities

        winfoWrap.setDisplay(winfo.visibleName());
        winfoWrap.setDesktops({ QString::number(winfo.desktop()) });
        winfoWrap.setActivities(winfo.activities());
    }

    if (plasmaBlockedWindow) {
        windowRemoved(wid);
    }

    return winfoWrap;
}

WindowInfoWrap XWindowInterface::requestInfoActive()
{
    return requestInfo(KX11Extras::activeWindow());
}

QString XWindowInterface::requestWindowClass(WindowId wid)
{
    return KWindowInfo(wid.toUInt(), NET::Supported, NET::WM2WindowClass).windowClassClass();
}

void XWindowInterface::clearViewStruts(QWindow* view)
{
    this->removeViewStruts(*view);
}

void XWindowInterface::startInitWindows()
{
    for (auto wid : KX11Extras::self()->windows()) {
        onWindowadded(wid);
    }
}

QString XWindowInterface::desktopFilePath(WindowId wid)
{
    const KWindowInfo info(wid.toUInt(), NET::Properties(), NET::WM2WindowClass | NET::WM2DesktopFileName);
    return Utils::instance()->desktopPathFromMetadata(info.windowClassClass(),
        NETWinInfo(QX11Info::connection(), wid.toUInt(),
            QX11Info::appRootWindow(),
            NET::WMPid,
            NET::Properties2())
            .pid(),
        info.windowClassName());
}

void XWindowInterface::setIconGeometry(WindowId wid, const QRect& rect)
{
    NETWinInfo info(QX11Info::connection(),
        wid.toUInt(),
        (WId)QX11Info::appRootWindow(),
        NET::WMIconGeometry,
        QFlags<NET::Property2>(1));
    NETRect nrect;
    nrect.pos.x = rect.x();
    nrect.pos.y = rect.y();
    nrect.size.height = rect.height();
    nrect.size.width = rect.width();
    info.setIconGeometry(nrect);
}
