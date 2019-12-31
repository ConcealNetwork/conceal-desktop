// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers  
// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QTranslator>
#include <QLockFile>
#include <QMessageBox>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QRegularExpression>
#include "LogFileWatcher.h"

#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "LoggerAdapter.h"
#include "UpdateManager.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "SignalHandler.h"
#include "WalletAdapter.h"
#include "TranslatorManager.h"
#include "gui/MainWindow.h"

#define DEBUG 1

using namespace WalletGui;

const QRegularExpression LOG_SPLASH_REG_EXP("(?<=\] ).*");

QSplashScreen* splash(nullptr);

inline void newLogString(const QString& _string) {
  QRegularExpressionMatch match = LOG_SPLASH_REG_EXP.match(_string);
  if (match.hasMatch()) {
    QString message = match.captured(0).toUpper();
    splash->showMessage(message, Qt::AlignCenter | Qt::AlignBottom, Qt::white);
  }
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("Conceal Desktop");
  app.setApplicationVersion(Settings::instance().getVersion());
  app.setQuitOnLastWindowClosed(false);

  #ifndef Q_OS_MAC
    QApplication::setStyle(QStyleFactory::create("Fusion"));
  #endif
  
  CommandLineParser cmdLineParser(nullptr);
  Settings::instance().setCommandLineParser(&cmdLineParser);
  bool cmdLineParseResult = cmdLineParser.process(app.arguments());
  Settings::instance().load();

  //Translator must be created before the application's widgets.
  TranslatorManager* tmanager = TranslatorManager::instance();
  Q_UNUSED(tmanager)

  setlocale(LC_ALL, "");

  #ifdef Q_OS_WIN
    if(!cmdLineParseResult) {
      QMessageBox::critical(nullptr, QObject::tr("Error"), cmdLineParser.getErrorText());
      return app.exec();
    } else if (cmdLineParser.hasHelpOption()) {
      QMessageBox::information(nullptr, QObject::tr("Help"), cmdLineParser.getHelpText());
      return app.exec();
    }
  #endif

  LoggerAdapter::instance().init();

  QString dataDirPath = Settings::instance().getDataDir().absolutePath();
  if (!QDir().exists(dataDirPath)) {
    QDir().mkpath(dataDirPath);
  }

  QLockFile lockFile(Settings::instance().getDataDir().absoluteFilePath(QApplication::applicationName() + ".lock"));
  if (!lockFile.tryLock()) {
    QMessageBox::warning(nullptr, QObject::tr("Fail"), QString("%1 wallet already running").arg(CurrencyAdapter::instance().getCurrencyDisplayName()));
    return 0;
  }

  QLocale::setDefault(QLocale::c());

  SignalHandler::instance().init();
  QObject::connect(&SignalHandler::instance(), &SignalHandler::quitSignal, &app, &QApplication::quit);

  if (splash == nullptr) {
    splash = new QSplashScreen(QPixmap(":images/splash"), Qt::X11BypassWindowManagerHint);
  }

  if (!splash->isVisible()) {
    splash->show();
  }

  splash->showMessage(QObject::tr("LOADING WALLET"), Qt::AlignCenter | Qt::AlignBottom, Qt::white);

  LogFileWatcher* logWatcher(nullptr);
  if (logWatcher == nullptr) {
    logWatcher = new LogFileWatcher(Settings::instance().getDataDir().absoluteFilePath("Concealwallet.log"), &app);
    QObject::connect(logWatcher, &LogFileWatcher::newLogStringSignal, &app, &newLogString);
  }

  app.processEvents();
  qRegisterMetaType<CryptoNote::TransactionId>("CryptoNote::TransactionId");
  qRegisterMetaType<quintptr>("quintptr");
  if (!NodeAdapter::instance().init()) {
    return 0;
  }

  splash->finish(&MainWindow::instance());

  if (logWatcher != nullptr) {
    logWatcher->deleteLater();
    logWatcher = nullptr;
  }

  splash->deleteLater();
  splash = nullptr;

  Updater *d = new Updater();
  d->checkForUpdate();  
  MainWindow::instance().show();
  
  WalletAdapter::instance().open("");
  QObject::connect(QApplication::instance(), &QApplication::aboutToQuit, []() {
    MainWindow::instance().quit();
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }

    NodeAdapter::instance().deinit();
  });

  return app.exec();
}
