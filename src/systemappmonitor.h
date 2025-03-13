/*
 * SPDX-FileCopyrightText: 2021 rekols <revenmartin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SYSTEMAPPMONITOR_H
#define SYSTEMAPPMONITOR_H

#include <QObject>
#include "systemappitem.h"

class SystemAppMonitor : public QObject
{
    Q_OBJECT

public:
    static SystemAppMonitor *self();

    explicit SystemAppMonitor(QObject *parent = nullptr);
    ~SystemAppMonitor();

    SystemAppItem *find(const QString &filePath);
    QList<SystemAppItem *> applications() { return m_items; }

signals:
    void refreshed();

private:
    void refresh();
    void addApplication(const QString &filePath);
    void removeApplication(SystemAppItem *item);

private:
    QList<SystemAppItem *> m_items;
};

#endif // SYSTEMAPPMONITOR_H
