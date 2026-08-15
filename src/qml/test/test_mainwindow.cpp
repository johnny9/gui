// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qml/test/qt_test_registry.h>

#include <QScopedPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTest>
#include <QUrl>

class MainWindowTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadsFromResources()
    {
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("nodeModel"), static_cast<QObject*>(nullptr));
        QQmlComponent component{&engine, QUrl{QStringLiteral("qrc:///qml/pages/MainWindow.qml")}};
        QScopedPointer<QObject> window{component.create()};

        QVERIFY2(window, qPrintable(component.errorString()));
        QCOMPARE(window->objectName(), QStringLiteral("mainWindow"));
        QCOMPARE(window->property("title").toString(), QStringLiteral("Bitcoin Core"));
        QCOMPARE(window->property("nodeStatus").toString(), QStringLiteral("Not connected"));
    }
};

BITCOINQML_REGISTER_QT_TEST(MainWindowTests)

#include "test_mainwindow.moc"
