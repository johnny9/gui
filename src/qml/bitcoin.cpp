// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qml/bitcoin.h>

#include <clientversion.h>
#include <common/args.h>
#include <common/init.h>
#include <common/system.h>
#include <init.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <node/interface_ui.h>
#include <noui.h>
#include <qml/initexecutor.h>
#include <qml/models/nodemodel.h>
#ifdef ENABLE_TEST_AUTOMATION
#include <qml/test/testbridge.h>
#endif
#include <util/threadnames.h>
#include <util/translation.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringLiteral>
#include <QTimer>
#include <QUrl>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>

int QmlGuiMain(int argc, char* argv[])
{
    Q_INIT_RESOURCE(bitcoin_qml);

#ifdef WIN32
    common::WinCmdLineArgs win_args;
    std::tie(argc, argv) = win_args.get();
#endif

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(QGuiApplication::translate("bitcoin-core", "Bitcoin Core"));
    QGuiApplication::setQuitOnLastWindowClosed(false);

    SetupEnvironment();
    util::ThreadSetInternalName("main");
    noui_connect();

    std::unique_ptr<interfaces::Init> init{interfaces::MakeGuiInit(argc, argv)};

    SetupServerArgs(gArgs, init->canListenIpc());
#ifdef ENABLE_TEST_AUTOMATION
    gArgs.AddArg("-test-automation=<path>", "Enable test automation bridge on the given local socket path", ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
#endif
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        InitError(Untranslated(strprintf("Error parsing command line arguments: %s", error)));
        return EXIT_FAILURE;
    }

    if (HelpRequested(gArgs) || gArgs.GetBoolArg("-version", false)) {
        std::cout << init->exeName() << " " << FormatFullVersion() << "\n";
        if (!gArgs.GetBoolArg("-version", false)) {
            std::cout << "\n" << gArgs.GetHelpMessage();
        }
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc; ++i) {
        if (!IsSwitchChar(argv[i][0])) {
            InitError(Untranslated(strprintf("Command line contains unexpected token '%s', see bitcoin-qml -h for a list of options.", argv[i])));
            return EXIT_FAILURE;
        }
    }

    if (auto config_error{common::InitConfig(gArgs)}) {
        InitError(config_error->message, config_error->details);
        return EXIT_FAILURE;
    }

    gArgs.SoftSetBoolArg("-printtoconsole", false);
    InitLogging(gArgs);
    InitParameterInteraction(gArgs);

    std::unique_ptr<interfaces::Node> node{init->makeNode()};
    if (!node->baseInitialize()) {
        return EXIT_FAILURE;
    }

    qRegisterMetaType<interfaces::BlockAndHeaderTipInfo>("interfaces::BlockAndHeaderTipInfo");

    NodeModel node_model{*node};
    QmlInitExecutor init_executor{*node};

    QObject::connect(&node_model, &NodeModel::requestedInitialize, &init_executor, &QmlInitExecutor::initialize);
    QObject::connect(&node_model, &NodeModel::requestedShutdown, &init_executor, &QmlInitExecutor::shutdown);
    QObject::connect(&init_executor, &QmlInitExecutor::initializeResult, &node_model, &NodeModel::initializeResult);
    QObject::connect(&init_executor, &QmlInitExecutor::shutdownResult, &node_model, &NodeModel::shutdownResult);
    QObject::connect(&init_executor, &QmlInitExecutor::runawayException, &node_model, &NodeModel::handleRunawayException);
    QObject::connect(&node_model, &NodeModel::shutdownComplete, &app, [&app, &node] {
        app.exit(node->getExitStatus());
    });
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &node_model, &NodeModel::requestShutdown);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("nodeModel"), &node_model);
    engine.load(QUrl{QStringLiteral("qrc:///qml/pages/MainWindow.qml")});
    if (engine.rootObjects().isEmpty()) {
        node->startShutdown();
        node->appShutdown();
        return EXIT_FAILURE;
    }

#ifdef ENABLE_TEST_AUTOMATION
    std::unique_ptr<TestBridge> test_bridge;
    if (gArgs.IsArgSet("-test-automation")) {
        const QString socket_path{QString::fromStdString(gArgs.GetArg("-test-automation", ""))};
        if (!socket_path.isEmpty()) {
            test_bridge = std::make_unique<TestBridge>(engine, socket_path);
        }
    }
#endif

    QTimer::singleShot(0, &node_model, &NodeModel::start);
    return app.exec();
}
