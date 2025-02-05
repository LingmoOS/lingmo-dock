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

#ifndef XWINDOWINTERFACE_H
#define XWINDOWINTERFACE_H

#include "docksettings.h"
#include "windowinfowrap.h"
#include "abstractwindowinfo.h"
#include "global/singleton.h"

#include <QObject>

// KLIB
#include <KWindowInfo>
#include <KWindowEffects>

using namespace Lingmo::WindowSystem;

class XWindowInterface : public AbstractWindowInterface
{
    Q_OBJECT
    SINGLETON(XWindowInterface)

public:
    explicit XWindowInterface(QObject *parent = nullptr);
    ~XWindowInterface() override = default;

    void setViewStruts(QWindow &view, DockSettings::Direction direction, const QRect &rect, bool compositing) override;

    void removeViewStruts(QWindow &view) override;
    /**
     * Clear View Struts
     * @param view 指向 QWindow 对象的指针，表示需要清除边框的视图。
     *
     * @deprecated 自版本 3.0 起已弃用，请使用 removeViewStruts 替代。
     *
     * @since 1.0
     */
    void clearViewStruts(QWindow *view);


    WindowId activeWindow() override;

    WindowInfoWrap requestInfo(WindowId wid) override;
    WindowInfoWrap requestInfoActive() override;

    void skipTaskBar(const QDialog &dialog) override;
    void slideWindow(QWindow &view, Slide location) override;
    void enableBlurBehind(QWindow &view) override;
    void enableBlurBehind(QWindow &view, bool enable, const QRegion &region) override;

    void requestActivate(WindowId wid) override;
    /**
     * 强制激活窗口
     * @note This should not be used! Use requestActivate instead.
     * @param win
     * @since 1.0
     */
    void forceActiveWindow(WindowId win);
    void requestClose(WindowId wid) override;
    /**
     * @deprecated since 2.1.0，请使用 requestClose 替代
     * @param id
     */
    void closeWindow(WId id);
    void requestMoveWindow(WindowId wid, QPoint from) override;
    void requestToggleIsOnAllDesktops(WindowId wid) override;
    void requestToggleKeepAbove(WindowId wid) override;
    /**
     * 请求切换窗口的最小化状态
     *
     * 本函数旨在 toggling 一个窗口的最小化状态如果窗口当前是最小化状态，则将其恢复；
     * 否则，将窗口最小化此操作仅在窗口有效且位于当前桌面活动时执行
     *
     * @since 2.1.0
     * @param wid 窗口标识符，用于唯一标识一个窗口
     */
    void requestToggleMinimized(WindowId wid) override;
    /**
     * 最小化窗口
     * @deprecated since 2.1.0，请使用 requestToggleMinimized 替代
     * @since 1.0
     * @param win
     */
    void minimizeWindow(WindowId win);
    void requestToggleMaximized(WindowId wid) override;

    void setKeepAbove(WindowId wid, bool active) override;
    void setKeepBelow(WindowId wid, bool active) override;

    bool windowCanBeDragged(WindowId wid) override;
    bool windowCanBeMaximized(WindowId wid) override;

    QIcon iconFor(WindowId wid) override;

    /**
     * Return the window id for the given appId and geometry
     * @note Currently not implemented, only return the activeWindow
     * @param appId
     * @param geometry
     * @return
     */
    WindowId winIdFor(QString appId, QRect geometry) override;
    /**
     * Return the window id for the given appId and title
     * @note Currently not implemented, only return the activeWindow
     * @param appId
     * @param title
     * @return
     */
    WindowId winIdFor(QString appId, QString title) override;

    void switchToNextVirtualDesktop() override;
    void switchToPreviousVirtualDesktop() override;

    void setFrameExtents(QWindow *view, const QMargins &margins) override;
    void setInputMask(QWindow *window, const QRect &rect) override;


private:
    bool isAcceptableWindow(WindowId wid);
    bool isValidWindow(WindowId wid);

    QRect visibleGeometry(const WindowId &wid, const QRect &frameGeometry) const;

    void checkShapeExtension();

    void windowAddedProxy(WId wid);
    /**
     * @deprecated use windowAddedProxy instead
     * @param wid
     */
    void onWindowadded(WId wid);

    //xcb_shape
    bool m_shapeExtensionChecked{false};
    bool m_shapeAvailable{false};
        /////////////////////////////////////////////

public:


    QString requestWindowClass(WindowId wid);

    void startInitWindows();

    QString desktopFilePath(WindowId wid);

    void setIconGeometry(WindowId wid, const QRect &rect);

signals:
    void activeChanged(WindowId wid);

private:



};

#endif // XWINDOWINTERFACE_H
