// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QTranslator>
#include <QMainWindow>
#include <QSystemTrayIcon>

class QActionGroup;

namespace Ui {
class MainWindow;
}

namespace WalletGui {

class MainWindow : public QMainWindow {
  Q_OBJECT
  Q_DISABLE_COPY(MainWindow)

public:
  static MainWindow& instance();
  void scrollToTransaction(const QModelIndex& _index);
  void quit();

protected:
  void closeEvent(QCloseEvent* _event) Q_DECL_OVERRIDE;
  bool event(QEvent* _event) Q_DECL_OVERRIDE;

protected Q_SLOTS:
#ifndef QT_NO_SYSTEMTRAYICON
  void restoreFromTray();
#endif

private:
  QScopedPointer<Ui::MainWindow> m_ui;
  QSystemTrayIcon* m_trayIcon;
  QActionGroup* m_tabActionGroup;
  bool m_isAboutToQuit;

  static MainWindow* m_instance;

  MainWindow();
  ~MainWindow();

  void connectToSignals();
  void initUi();

  void minimizeToTray(bool _on);
  void showMessage(const QString& _text, QtMsgType _type);
  void askForWalletPassword(bool _error);
  void walletOpened(bool _error, const QString& _error_text);
  void walletClosed();
  void payTo(const QModelIndex& _index);
  void delay();
  void setRemoteWindowTitle();  
  void showQRCode(const QString& _address);
  void backupTo();
  void rescanTo();
  void dashboardTo();
  void checkTrackingMode();
  
  Q_SLOT void createWallet();
  Q_SLOT void openWallet();
  Q_SLOT void closeWallet();
  Q_SLOT void importKey();
  Q_SLOT void importsecretkeys();
  Q_SLOT void importSeed();
  Q_SLOT void importTracking();
  Q_SLOT void backupWallet();
  Q_SLOT void resetWallet();
  Q_SLOT void encryptWallet();
  Q_SLOT void aboutQt();
  Q_SLOT void setStartOnLogin(bool _on);
  Q_SLOT void setMinimizeToTray(bool _on);
  Q_SLOT void setCloseToTray(bool _on);

#ifdef Q_OS_MAC
public:
  void restoreFromDock();

private:
  void installDockHandler();
#endif
#ifndef QT_NO_SYSTEMTRAYICON
protected:
  void changeEvent(QEvent* _event) Q_DECL_OVERRIDE;

private:
  void trayActivated(QSystemTrayIcon::ActivationReason _reason);
#endif
};

}
