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
#include <QSharedMemory>
#include <QTranslator>
#include <qqmlengine.h>
#include <qsharedmemory.h>
#include <QQmlComponent>

#include <LingmoLogger/QsLog.h>
#include <LingmoLogger/QsLogDest.h>

#include "view/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QQmlEngine engine;

    // Setup logging
    QsLogging::Logger& logger = QsLogging::Logger::instance();
    logger.addDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.setLoggingLevel(QsLogging::InfoLevel);

    // Assure running in single instance
    auto sharedMemory = QSharedMemory(QApplication::instance());
    sharedMemory.setKey("lingmo-dock-key");
    if (!sharedMemory.create(1 /*byte*/)) {
        // The failure might have been caused by a previous crash.
        sharedMemory.attach();
        sharedMemory.detach();
        // Now try again.
        if (!sharedMemory.create(1 /*byte*/)) {
            QLOG_ERROR() << "Another instance is already running.";
            return -1;
        }
    }

    // Set basic application information
    app.setOrganizationName("Lingmo");
    app.setOrganizationDomain("lingmo.org");
    app.setApplicationName("lingmo-dock");
    app.setWindowIcon(QIcon::fromTheme("lingmo-dock"));

    if (!QDBusConnection::sessionBus().registerService("com.lingmo.Dock")) {
        return -1;
    }

    qmlRegisterType<DockSettings>("Lingmo.Dock", 1, 0, "DockSettings");

    QString qmFilePath = QString("%1/%2.qm")
                             .arg("/usr/share/lingmo-dock/translations/")
                             .arg(QLocale::system().name());
    if (QFile::exists(qmFilePath)) {
        QTranslator* translator = new QTranslator(QApplication::instance());
        if (translator->load(qmFilePath)) {
            QGuiApplication::installTranslator(translator);
        } else {
            translator->deleteLater();
        }
    }

    MainWindow w(&engine);

    auto object = w.rootObject();

    if (!QDBusConnection::sessionBus().registerObject("/Dock", &w)) {
        return -1;
    }

    return app.exec();
}
