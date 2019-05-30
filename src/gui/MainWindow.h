// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QLabel>
#include <QLocale>
#include <QTranslator>
#include <QPushButton>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>

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

protected slots:
  void slotLanguageChanged(QAction* action);

private:
  QScopedPointer<Ui::MainWindow> m_ui;
  QSystemTrayIcon* m_trayIcon;
  QActionGroup* m_tabActionGroup;
  bool m_isAboutToQuit;
  QTranslator m_translator; // contains the translations for this application
  QTranslator m_translatorQt; // contains the translations for qt
  QString m_currLang; // contains the currently loaded language
  QString m_langPath; // Path of language files. This is always fixed to /languages

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
  void replyTo(const QModelIndex& _index);
  void loadLanguage(const QString& rLanguage);  
  void payTo(const QModelIndex& _index);
  void sendTo();
  void delay();
  void setRemoteWindowTitle();  
  void showQRCode(const QString& _address);
  void backupTo();
  void rescanTo();    
  void addressBookTo();
  void sendMessageTo();    
  void miningTo();

  Q_SLOT void dashboardTo();
  Q_SLOT void transactionTo();
  Q_SLOT void messageTo();
  Q_SLOT void depositTo();
  Q_SLOT void welcomeTo();  

  Q_SLOT void createWallet();
  Q_SLOT void openWallet();
  Q_SLOT void closeWallet();
  Q_SLOT void importKey();
  Q_SLOT void importsecretkeys();
  Q_SLOT void importSeed();
  Q_SLOT void importTracking();
  Q_SLOT void nodeSettings();  
  Q_SLOT void languageSettings();    
  Q_SLOT void backupWallet();
  Q_SLOT void resetWallet();
  Q_SLOT void optimizeClicked();  
  Q_SLOT void encryptWallet();
  Q_SLOT void aboutQt();
  Q_SLOT void about();
  Q_SLOT void disclaimer();  
  Q_SLOT void links();   
  Q_SLOT void setStartOnLogin(bool _on);
  Q_SLOT void setMinimizeToTray(bool _on);
  Q_SLOT void setCloseToTray(bool _on);

#ifdef Q_OS_MAC
public:
  void restoreFromDock();

private:
  void installDockHandler();
#elif defined(Q_OS_WIN)
protected:
  void changeEvent(QEvent* _event) Q_DECL_OVERRIDE;

private:
  void trayActivated(QSystemTrayIcon::ActivationReason _reason);
#endif
};

}
