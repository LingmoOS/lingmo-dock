/*
 * SPDX-FileCopyrightText: 2021 rekols <revenmartin@gmail.com>
 * SPDX-FileCopyrightText: 2024 Elysia <elysia@lingmo.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QDBusConnection>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  if (!QDBusConnection::sessionBus().registerService("com.lingmo.Dock")) {
    return -1;
  }

  qmlRegisterType<DockSettings>("Lingmo.Dock", 1, 0, "DockSettings");

  QString qmFilePath = QString("%1/%2.qm")
                           .arg("/usr/share/lingmo-dock/translations/")
                           .arg(QLocale::system().name());
  if (QFile::exists(qmFilePath)) {
    QTranslator *translator = new QTranslator(QApplication::instance());
    if (translator->load(qmFilePath)) {
      QGuiApplication::installTranslator(translator);
    } else {
      translator->deleteLater();
    }
  }

  MainWindow w;

  if (!QDBusConnection::sessionBus().registerObject("/Dock", &w)) {
    return -1;
  }

  return app.exec();
}
