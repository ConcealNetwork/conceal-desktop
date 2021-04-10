// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QDesktopWidget>
#include <QLocale>
#include <QLockFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleFactory>

#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "LogFileWatcher.h"
#include "LoggerAdapter.h"
#include "NodeAdapter.h"
#include "Settings.h"
#include "SignalHandler.h"
#include "TranslatorManager.h"
#include "UpdateManager.h"
#include "WalletAdapter.h"
#include "gui/MainWindow.h"
#include "gui/SplashScreen.h"

#define DEBUG 1

using namespace WalletGui;

const QRegularExpression LOG_SPLASH_REG_EXP("(?<=] ).*");

SplashScreen* splashScreen(nullptr);

inline void newLogString(const QString& _string)
{
  QRegularExpressionMatch match = LOG_SPLASH_REG_EXP.match(_string);
  if (match.hasMatch())
  {
    QString message = match.captured(0).toUpper();
    splashScreen->showMessage(message, Qt::AlignCenter | Qt::AlignBottom, Qt::darkGray);
  }
}

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
  QApplication app(argc, argv);
  QApplication::setApplicationName("Conceal Desktop");
  QApplication::setApplicationVersion(Settings::instance().getVersion());
  QApplication::setQuitOnLastWindowClosed(false);

#ifndef Q_OS_MAC
  QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

  CommandLineParser cmdLineParser(nullptr);
  Settings::instance().setCommandLineParser(&cmdLineParser);
  bool cmdLineParseResult = cmdLineParser.process(QApplication::arguments());
  Settings::instance().load();

  // Translator must be created before the application's widgets.
  TranslatorManager* tManager = TranslatorManager::instance();
  Q_UNUSED(tManager)

  setlocale(LC_ALL, "");

#ifdef Q_OS_WIN
  if (!cmdLineParseResult)
  {
    QMessageBox::critical(&MainWindow::instance(), QObject::tr("Error"),
                          cmdLineParser.getErrorText());
    return app.exec();
  }
  else if (cmdLineParser.hasHelpOption())
  {
    QMessageBox::information(&MainWindow::instance(), QObject::tr("Help"),
                             cmdLineParser.getHelpText());
    return app.exec();
  }
#else
  Q_UNUSED(cmdLineParseResult)
#endif

  LoggerAdapter::instance().init();

  QString dataDirPath = Settings::instance().getDataDir().absolutePath();
  if (!QDir().exists(dataDirPath))
  {
    QDir().mkpath(dataDirPath);
  }

  QLockFile lockFile(Settings::instance().getDataDir().absoluteFilePath(
      QApplication::applicationName() + ".lock"));
  if (!lockFile.tryLock())
  {
    QMessageBox::warning(nullptr, QObject::tr("Fail"),
                         QString("%1 wallet already running")
                             .arg(CurrencyAdapter::instance().getCurrencyDisplayName()));
    return 0;
  }

  QLocale::setDefault(QLocale::c());

  SignalHandler::instance().init();
  QObject::connect(&SignalHandler::instance(), &SignalHandler::quitSignal, &app,
                   &QApplication::quit);

  if (splashScreen == nullptr)
  {
    splashScreen = new SplashScreen();
    splashScreen->centerOnScreen(&app);
  }

  splashScreen->show();

  LogFileWatcher* logWatcher = new LogFileWatcher(
      Settings::instance().getDataDir().absoluteFilePath("Concealwallet.log"), &app);
  QObject::connect(logWatcher, &LogFileWatcher::newLogStringSignal, &app, &newLogString);

  QApplication::processEvents();
  qRegisterMetaType<CryptoNote::TransactionId>("CryptoNote::TransactionId");
  qRegisterMetaType<quintptr>("quintptr");
  if (!NodeAdapter::instance().init())
  {
    return 0;
  }

  splashScreen->finish(&MainWindow::instance());

  logWatcher->deleteLater();
  logWatcher = nullptr;

  splashScreen->deleteLater();
  splashScreen = nullptr;

  Updater* d = new Updater();
  d->checkForUpdate();
  MainWindow::instance().show();

  WalletAdapter::instance().open("");
  QObject::connect(QApplication::instance(), &QApplication::aboutToQuit, []() {
    MainWindow::instance().quit();
    if (WalletAdapter::instance().isOpen())
    {
      WalletAdapter::instance().close();
    }

    NodeAdapter::instance().deinit();
  });

  return QApplication::exec();
}
