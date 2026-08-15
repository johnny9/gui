// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qml/bitcoin.h>

#ifdef ENABLE_TEST_AUTOMATION
#include <qml/test/testbridge.h>
#endif

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStringLiteral>
#include <QUrl>

#include <cstdlib>
#include <memory>

#ifdef ENABLE_TEST_AUTOMATION
namespace {
QString TestAutomationSocketPath(int argc, char* argv[])
{
    const QString prefix{QStringLiteral("-test-automation=")};
    for (int i = 1; i < argc; ++i) {
        const QString argument{QString::fromLocal8Bit(argv[i])};
        if (argument.startsWith(prefix)) {
            return argument.sliced(prefix.size());
        }
    }
    return {};
}
} // namespace
#endif

int QmlGuiMain(int argc, char* argv[])
{
    Q_INIT_RESOURCE(bitcoin_qml);

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(QGuiApplication::translate("bitcoin-core", "Bitcoin Core"));

    QQmlApplicationEngine engine;
    engine.load(QUrl{QStringLiteral("qrc:///qml/pages/MainWindow.qml")});
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

#ifdef ENABLE_TEST_AUTOMATION
    std::unique_ptr<TestBridge> test_bridge;
    if (const QString socket_path{TestAutomationSocketPath(argc, argv)}; !socket_path.isEmpty()) {
        test_bridge = std::make_unique<TestBridge>(engine, socket_path);
    }
#endif

    return app.exec();
}
