// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QThread>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <Common/Base58.h>
#include <Common/Util.h>
#include "Common/CommandLine.h"
#include "Common/SignalHandler.h"
#include "Common/StringTools.h"
#include "Common/PathTools.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Account.cpp"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "Mnemonics/electrum-words.cpp"
#include "ShowQRCode.h"
#include "AboutDialog.h"
#include "DisclaimerDialog.h"
#include "LinksDialog.h"
#include "AddressBookModel.h"
#include "AnimatedLabel.h"
#include "ChangePasswordDialog.h"
#include "CurrencyAdapter.h"
#include "ExitWidget.h"
#include "ImportKeyDialog.h"
#include "importsecretkeys.h"
#include "importseed.h"
#include "importtracking.h"
#include "transactionconfirmation.h"
#include "NodeSettings.h"
#include "MainWindow.h"
#include "MessagesModel.h"
#include "NewPasswordDialog.h"
#include "NodeAdapter.h"
#include "PasswordDialog.h"
#include "Settings.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"

#include "ui_mainwindow.h"

namespace WalletGui {

MainWindow* MainWindow::m_instance = nullptr;

MainWindow& MainWindow::instance() {

  if (m_instance == nullptr) {
    m_instance = new MainWindow;
  }
  return *m_instance;
}

MainWindow::MainWindow() : QMainWindow(), m_ui(new Ui::MainWindow), m_trayIcon(nullptr), m_tabActionGroup(new QActionGroup(this)),
  m_isAboutToQuit(false) {
  m_ui->setupUi(this);
  connectToSignals();
  initUi();
  walletClosed();
}

MainWindow::~MainWindow() {
}

void MainWindow::connectToSignals() {
  connect(&WalletAdapter::instance(), &WalletAdapter::openWalletWithPasswordSignal, this, &MainWindow::askForWalletPassword, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::changeWalletPasswordSignal, this, &MainWindow::encryptWallet, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &MainWindow::walletOpened);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &MainWindow::walletClosed);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionCreatedSignal, this, [this]() {
    QApplication::alert(this);
  });

  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, [this](CryptoNote::TransactionId _transactionId, int _error, const QString& _errorString) {
    if (_error == 0) {
      m_ui->m_transactionsAction->setChecked(true);
    }
  });
  connect(m_ui->m_exitAction, &QAction::triggered, qApp, &QApplication::quit);
  connect(m_ui->m_messagesFrame, &MessagesFrame::replyToSignal, this, &MainWindow::replyTo);
  connect(m_ui->m_addressBookFrame, &AddressBookFrame::payToSignal, this, &MainWindow::payTo);
  connect(m_ui->m_receiveFrame, &ReceiveFrame::backupSignal, this, &MainWindow::backupWallet);    

  connect(m_ui->m_overviewFrame, &OverviewFrame::newWalletSignal, this, &MainWindow::createWallet, Qt::QueuedConnection);
  connect(m_ui->m_overviewFrame, &OverviewFrame::newTransferSignal, this, &MainWindow::sendTo, Qt::QueuedConnection);
  connect(m_ui->m_overviewFrame, &OverviewFrame::newMessageSignal, this, &MainWindow::sendMessageTo);    

  connect(m_ui->m_welcomeFrame, &WelcomeFrame::createWalletClickedSignal, this, &MainWindow::createWallet, Qt::QueuedConnection);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::openWalletClickedSignal, this, &MainWindow::openWallet, Qt::QueuedConnection);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importSeedClickedSignal, this, &MainWindow::importSeed, Qt::QueuedConnection);  
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importsecretkeysClickedSignal, this, &MainWindow::importsecretkeys, Qt::QueuedConnection);  
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importKeyClickedSignal, this, &MainWindow::importKey, Qt::QueuedConnection);      

  /* signals from overview frame buttons */
  connect(m_ui->m_overviewFrame, &OverviewFrame::openWalletSignal, this, &MainWindow::openWallet, Qt::QueuedConnection);
  connect(m_ui->m_overviewFrame, &OverviewFrame::sendSignal, this, &MainWindow::sendTo);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::depositSignal, this, &MainWindow::depositTo);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::backupSignal, this, &MainWindow::backupTo);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::rescanSignal, this, &MainWindow::rescanTo);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::transactionSignal, this, &MainWindow::transactionTo);    
  connect(m_ui->m_overviewFrame, &OverviewFrame::messageSignal, this, &MainWindow::messageTo);      
  connect(m_ui->m_overviewFrame, &OverviewFrame::aboutSignal, this, &MainWindow::about);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::aboutQTSignal, this, &MainWindow::aboutQt);        
  connect(m_ui->m_overviewFrame, &OverviewFrame::disclaimerSignal, this, &MainWindow::disclaimer);        
  connect(m_ui->m_overviewFrame, &OverviewFrame::linksSignal, this, &MainWindow::links);     
  
  connect(m_ui->m_overviewFrame, &OverviewFrame::qrSignal, this, &MainWindow::showQRCode);
  connect(m_ui->m_overviewFrame, &OverviewFrame::optimizeSignal, this, &MainWindow::optimizeClicked);      
  connect(m_ui->m_overviewFrame, &OverviewFrame::importSeedSignal, this, &MainWindow::importSeed);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importGUIKeySignal, this, &MainWindow::importKey);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importSecretKeysSignal, this, &MainWindow::importsecretkeys);  
  connect(m_ui->m_overviewFrame, &OverviewFrame::connectionSettingsSignal, this, &MainWindow::nodeSettings);    
  connect(m_ui->m_overviewFrame, &OverviewFrame::encryptWalletSignal, this, &MainWindow::encryptWallet);      
  connect(m_ui->m_overviewFrame, &OverviewFrame::closeWalletSignal, this, &MainWindow::closeWallet);      

  connect(m_ui->m_sendFrame, &SendFrame::backSignal, this, &MainWindow::dashboardTo);  
  connect(m_ui->m_sendFrame, &SendFrame::addressFoundSignal, this, &MainWindow::setRemoteWindowTitle);  
  connect(m_ui->m_sendFrame, &SendFrame::addressBookSignal, this, &MainWindow::addressBookTo);  
  connect(m_ui->m_depositsFrame, &DepositsFrame::backSignal, this, &MainWindow::dashboardTo);  
  connect(m_ui->m_messagesFrame, &MessagesFrame::newMessageSignal, this, &MainWindow::sendMessageTo);    
  connect(m_ui->m_receiveFrame, &ReceiveFrame::backSignal, this, &MainWindow::dashboardTo);    
  connect(m_ui->m_addressBookFrame, &AddressBookFrame::backSignal, this, &MainWindow::dashboardTo);  
  connect(m_ui->m_transactionsFrame, &TransactionsFrame::backSignal, this, &MainWindow::dashboardTo);  
  connect(m_ui->m_messagesFrame, &MessagesFrame::backSignal, this, &MainWindow::dashboardTo);    
  connect(m_ui->m_sendMessageFrame, &SendMessageFrame::backSignal, this, &MainWindow::dashboardTo);      
  
}

void MainWindow::initUi() {
  setWindowTitle(QString("%1 Wallet %2").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()));

#ifdef Q_OS_WIN32
  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    m_trayIcon = new QSystemTrayIcon(QPixmap(":images/cryptonote"), this);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
  }
#endif
  m_ui->m_aboutCryptonoteAction->setText(QString(tr("About %1 Wallet")).arg(CurrencyAdapter::instance().getCurrencyDisplayName()));

  m_ui->m_overviewFrame->hide();
  m_ui->m_sendFrame->hide();
  m_ui->m_receiveFrame->hide();
  m_ui->m_transactionsFrame->hide();
  m_ui->m_addressBookFrame->hide();
  m_ui->m_messagesFrame->hide();
  m_ui->m_sendMessageFrame->hide();
  m_ui->m_depositsFrame->hide();

  m_tabActionGroup->addAction(m_ui->m_overviewAction);
  m_tabActionGroup->addAction(m_ui->m_sendAction);
  m_tabActionGroup->addAction(m_ui->m_receiveAction);
  m_tabActionGroup->addAction(m_ui->m_transactionsAction);
  m_tabActionGroup->addAction(m_ui->m_addressBookAction);
  m_tabActionGroup->addAction(m_ui->m_messagesAction);
  m_tabActionGroup->addAction(m_ui->m_sendMessageAction);
  m_tabActionGroup->addAction(m_ui->m_depositsAction);

  m_ui->m_overviewAction->toggle();


#ifdef Q_OS_MAC
  installDockHandler();
#endif

#ifndef Q_OS_WIN
  m_ui->m_minimizeToTrayAction->deleteLater();
  m_ui->m_closeToTrayAction->deleteLater();
#endif
}



#ifdef Q_OS_WIN
void MainWindow::minimizeToTray(bool _on) {
  if (_on) {
    hide();
    m_trayIcon->show();
  } else {
    showNormal();
    activateWindow();
    m_trayIcon->hide();
  }
}
#endif

void MainWindow::scrollToTransaction(const QModelIndex& _index) {
  m_ui->m_transactionsAction->setChecked(true);
  m_ui->m_transactionsFrame->scrollToTransaction(_index);
}

void MainWindow::quit() {
  Settings::instance().setCurrentFeeAddress("");    
  if (!m_isAboutToQuit) {
    ExitWidget* exitWidget = new ExitWidget(nullptr);
    exitWidget->show();
    m_isAboutToQuit = true;
    close();
  }
}

#ifdef Q_OS_MAC
void MainWindow::restoreFromDock() {
  if (m_isAboutToQuit) {
    return;
  }

  showNormal();
}
#endif

void MainWindow::closeEvent(QCloseEvent* _event) {
#ifdef Q_OS_WIN
  if (m_isAboutToQuit) {
    QMainWindow::closeEvent(_event);
    return;
  } else if (Settings::instance().isCloseToTrayEnabled()) {
    minimizeToTray(true);
    _event->ignore();
  } else {
    QApplication::quit();
    return;
  }
#elif defined(Q_OS_LINUX)
  if (!m_isAboutToQuit) {
    QApplication::quit();
    return;
  }
#endif
  QMainWindow::closeEvent(_event);

}

#ifdef Q_OS_WIN
void MainWindow::changeEvent(QEvent* _event) {
  QMainWindow::changeEvent(_event);
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    QMainWindow::changeEvent(_event);
    return;
  }

  switch (_event->type()) {
  case QEvent::WindowStateChange:
    if(Settings::instance().isMinimizeToTrayEnabled()) {
      minimizeToTray(isMinimized());
    }
    break;
  default:
    break;
  }

  QMainWindow::changeEvent(_event);
}
#endif

bool MainWindow::event(QEvent* _event) {
  switch (static_cast<WalletEventType>(_event->type())) {
    case WalletEventType::ShowMessage:
    showMessage(static_cast<ShowMessageEvent*>(_event)->messageText(), static_cast<ShowMessageEvent*>(_event)->messageType());
    return true;
  }

  return QMainWindow::event(_event);
}

void MainWindow::setRemoteWindowTitle() {
  setWindowTitle(QString("%1 Wallet %2 Connected to Remote Node").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()));
}

void MainWindow::delay()
{
    QTime dieTime= QTime::currentTime().addSecs(2);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::optimizeClicked() 
{
  transactionconfirmation dlg(this);
  quint64 numUnlockedOutputs;
  numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();

  if (numUnlockedOutputs >= 100) {
    dlg.setMessage("Optimization recommended [" + QString::number(numUnlockedOutputs) + "]");
  } else {
    dlg.setMessage("Optimization not required [" + QString::number(numUnlockedOutputs) + "]");
  }

  if (dlg.exec() == QDialog::Accepted) {

    WalletAdapter::instance().optimizeWallet();
    while (WalletAdapter::instance().getNumUnlockedOutputs()> 100) {
      numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
      if (numUnlockedOutputs == 0) break;
      WalletAdapter::instance().optimizeWallet();
      MainWindow::delay();
    }

    m_ui->m_overviewAction->trigger();
    m_ui->m_overviewFrame->show();

  }
}  


/* ----------------------------- CREATE A NEW WALLET ------------------------------------ */

void MainWindow::createWallet() {

  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"),

  #ifdef Q_OS_WIN
      QApplication::applicationDirPath(),
  #else
      QDir::homePath(),
  #endif
      tr("Wallets (*.wallet)"));

    if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }
    if (!filePath.isEmpty() && !QFile::exists(filePath)) {
      if (WalletAdapter::instance().isOpen()) {
        WalletAdapter::instance().close();
      }
      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().createWallet();
    }
}

void MainWindow::openWallet() {  
  QString walletFile = Settings::instance().getWalletFile();
  std::string wallet = walletFile.toStdString();

  QString walletDirectory = "";
  if (!wallet.empty()) {
    /* Get current wallet path and use it as a default opening location */
    const size_t last_slash_idx = wallet.find_last_of("\\/");
    if (std::string::npos != last_slash_idx) {
      wallet.erase(last_slash_idx + 1, wallet.length());
    }
    walletDirectory = QString::fromStdString(wallet);
  } else {
    #ifdef Q_OS_WIN
      walletDirectory = QApplication::applicationDirPath();
    #else
      walletDirectory = QDir::homePath();
    #endif
  }

  QString filePath = QFileDialog::getOpenFileName(this, tr("Open .wallet/.keys file"),
    walletDirectory,
    tr("Wallet (*.wallet *.keys)"));

  if (!filePath.isEmpty()) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().open("");
  }
}

void MainWindow::closeWallet() {  
  WalletAdapter::instance().close();
  walletClosed();
}

void MainWindow::importKey() {
  ImportKeyDialog dlg(this);

  if (dlg.exec() == QDialog::Accepted) {
    QString keyString = dlg.getKeyString().trimmed();
    QString filePath = dlg.getFilePath();

    if (keyString.isEmpty() || filePath.isEmpty()) {
      return;
    }

    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    uint64_t addressPrefix;
    std::string data;
    CryptoNote::AccountKeys keys;

    if (Tools::Base58::decode_addr(keyString.toStdString(), addressPrefix, data) && addressPrefix == CurrencyAdapter::instance().getAddressPrefix() &&
      data.size() == sizeof(keys)) {
      std::memcpy(&keys, data.data(), sizeof(keys));

      if (WalletAdapter::instance().isOpen()) {
        WalletAdapter::instance().close();
      }

      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().createWithKeys(keys);
    }
    else if (Tools::Base58::decode_addr(keyString.toStdString(), addressPrefix, data) && addressPrefix == CurrencyAdapter::instance().getAddressPrefix()) {

      //serialize as public keys then convert to secret keys
      CryptoNote::AccountPublicAddress decodedKeys;
      CryptoNote::fromBinaryArray(decodedKeys, Common::asBinaryArray(data));
      
      // convert to secret key then get real public keys
      Crypto::SecretKey spendSecretKey = reinterpret_cast<Crypto::SecretKey&>(decodedKeys.spendPublicKey);
      Crypto::PublicKey spendPublicKey;
      Crypto::secret_key_to_public_key(spendSecretKey, spendPublicKey);
      
      Crypto::SecretKey viewSecretKey = reinterpret_cast<Crypto::SecretKey&>(decodedKeys.viewPublicKey);
      Crypto::PublicKey viewPublicKey;
      Crypto::secret_key_to_public_key(viewSecretKey, viewPublicKey);
      
      CryptoNote::AccountPublicAddress publicKeys;
      publicKeys.spendPublicKey = spendPublicKey;
      publicKeys.viewPublicKey = viewPublicKey;
      
      CryptoNote::AccountKeys keys;
      keys.address = publicKeys;
      keys.spendSecretKey = spendSecretKey;
      keys.viewSecretKey = viewSecretKey;
      
      if (WalletAdapter::instance().isOpen()) 
      {

        WalletAdapter::instance().close();
      }

      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().createWithKeys(keys); 
    }
  }
}

void MainWindow::backupWallet() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Backup wallet to..."),
  #ifdef Q_OS_WIN
      QApplication::applicationDirPath(),
  #else
      QDir::homePath(),
  #endif
      tr("Wallets (*.wallet)")
      );
    if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    if (!filePath.isEmpty() && !QFile::exists(filePath)) {
      WalletAdapter::instance().backup(filePath);
    }
}

void MainWindow::resetWallet() {
  Q_ASSERT(WalletAdapter::instance().isOpen());
  if (QMessageBox::warning(this, tr("Warning"), tr("Your wallet will be reset and restored from blockchain.\n"
    "Are you sure?"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
    WalletAdapter::instance().reset();
    WalletAdapter::instance().open("");
  }
}

void MainWindow::encryptWallet() {
  if (Settings::instance().isEncrypted()) {
    bool error = false;
    do {
      ChangePasswordDialog dlg(this);
      if (dlg.exec() == QDialog::Rejected) {
        return;
      }

      QString oldPassword = dlg.getOldPassword();
      QString newPassword = dlg.getNewPassword();
      error = !WalletAdapter::instance().changePassword(oldPassword, newPassword);
    } while (error);
  } else {
      NewPasswordDialog dlg(this);
      bool error = false;
      do {
          if (dlg.exec() == QDialog::Accepted) {
            QString password = dlg.getPassword();
            if (password.isEmpty()) {
              return;
            }
            error = !WalletAdapter::instance().changePassword("", password);
          }
      } while (error);
  }
}

void MainWindow::aboutQt() {
  QMessageBox::aboutQt(this);
}

void MainWindow::setStartOnLogin(bool _on) {
  Settings::instance().setStartOnLoginEnabled(_on);
  m_ui->m_startOnLoginAction->setChecked(Settings::instance().isStartOnLoginEnabled());
}

void MainWindow::setMinimizeToTray(bool _on) {
#ifdef Q_OS_WIN
  Settings::instance().setMinimizeToTrayEnabled(_on);
#endif
}

void MainWindow::setCloseToTray(bool _on) {
#ifdef Q_OS_WIN
  Settings::instance().setCloseToTrayEnabled(_on);
#endif
}

void MainWindow::about() {
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::disclaimer() {
  DisclaimerDialog dlg(this);
  dlg.exec();
}

void MainWindow::links() {
  LinksDialog dlg(this);
  dlg.exec();
}

void MainWindow::showMessage(const QString& _text, QtMsgType _type) {
  switch (_type) {
  case QtCriticalMsg:
    QMessageBox::critical(this, tr("Wallet error"), _text);
    break;
  case QtDebugMsg:
    QMessageBox::information(this, tr("Wallet"), _text);
    break;
  default:
    break;
  }
}

/* ----------------------------- PASSWORD PROMPT ------------------------------------ */

void MainWindow::askForWalletPassword(bool _error) 
{
  /* hide the welcome frame when waiting for the password */
  m_ui->m_welcomeFrame->hide(); 

  PasswordDialog dlg(_error, this);

  if (dlg.exec() == QDialog::Accepted) 
  {

    QString password = dlg.getPassword();
    WalletAdapter::instance().open(password);
  } else {

    m_ui->m_welcomeFrame->raise();
    m_ui->m_welcomeFrame->show();



  }
  
  
  }

void MainWindow::walletOpened(bool _error, const QString& _error_text) {
    m_ui->m_welcomeFrame->hide();
    if (!_error) {

    m_ui->m_backupWalletAction->setEnabled(true);
    m_ui->m_resetAction->setEnabled(true);

    QList<QAction*> tabActions = m_tabActionGroup->actions();
    Q_FOREACH(auto action, tabActions) {
      action->setEnabled(true);
    }
    

    m_ui->m_overviewAction->trigger();
    m_ui->m_overviewFrame->show();
  } else {
    walletClosed();
  }
}

/* ----------------------------- WALLET CLOSED ------------------------------------ */
/* this is what happens when the wallet goes into a closed state which includes the 
   period between closing the current wallet and opening/creating a new one */

void MainWindow::walletClosed() 
{

  /* actions */
  m_ui->m_backupWalletAction->setEnabled(false);
  m_ui->m_encryptWalletAction->setEnabled(false);
  m_ui->m_changePasswordAction->setEnabled(false);
  m_ui->m_resetAction->setEnabled(false);

  /* frames */
  m_ui->m_overviewFrame->hide();
  m_ui->m_sendFrame->hide();
  m_ui->m_transactionsFrame->hide();
  m_ui->m_addressBookFrame->hide();
  m_ui->m_messagesFrame->hide();
  m_ui->m_sendMessageFrame->hide();
  m_ui->m_welcomeFrame->show();
  m_ui->m_depositsFrame->hide();

  /* labels */
  QList<QAction*> tabActions = m_tabActionGroup->actions();

  Q_FOREACH(auto action, tabActions) 
  {

    action->setEnabled(false);
  }
}

/* ------------------------------------------------------------------------------------- */

void MainWindow::replyTo(const QModelIndex& _index) {
  m_ui->m_sendMessageFrame->setAddress(_index.data(MessagesModel::ROLE_HEADER_REPLY_TO).toString());
  m_ui->m_sendMessageAction->trigger();
}

void MainWindow::payTo(const QModelIndex& _index) {
  m_ui->m_sendFrame->setAddress(_index.data(AddressBookModel::ROLE_ADDRESS).toString());
  if (_index.data(AddressBookModel::ROLE_PAYMENTID).toString() != "") 
  {
      m_ui->m_sendFrame->setPaymentId(_index.data(AddressBookModel::ROLE_PAYMENTID).toString());  
  }

  m_ui->m_sendAction->trigger();
}

void MainWindow::sendTo() {
  m_ui->m_sendAction->trigger();
}

void MainWindow::dashboardTo() {
  m_ui->m_overviewAction->trigger();
  m_ui->m_overviewFrame->raise();
}

void MainWindow::depositTo() {
  m_ui->m_depositsAction->trigger();
}

void MainWindow::backupTo() 
{

  m_ui->m_receiveAction->trigger();

}

void MainWindow::transactionTo() {
  m_ui->m_transactionsAction->trigger();
}

void MainWindow::addressBookTo() {
  m_ui->m_addressBookAction->trigger();
}

void MainWindow::messageTo() {
  m_ui->m_messagesAction->trigger();
}

void MainWindow::sendMessageTo() 
{

  m_ui->m_sendMessageAction->trigger();
}

void MainWindow::rescanTo() {
  MainWindow::resetWallet();
}


/* --------------------------- IMPORT SECRET KEYS --------------------------------------- */

void MainWindow::importsecretkeys() 
{

  importSecretKeys dlg(this);

  if (dlg.exec() == QDialog::Accepted) 
  {

    QString spendKey = dlg.getSpendKeyString().trimmed();
    QString viewKey = dlg.getViewKeyString().trimmed();    
    QString filePath = dlg.getFilePath();

    if (spendKey.isEmpty() || filePath.isEmpty()) 
    {

      return;
    }

    if (!filePath.endsWith(".wallet")) 
    {

      filePath.append(".wallet");
    }

    std::string private_spend_key_string = spendKey.toStdString();
    std::string private_view_key_string = viewKey.toStdString();

    Crypto::SecretKey private_spend_key;
    Crypto::SecretKey private_view_key;

    Crypto::Hash private_spend_key_hash;
    Crypto::Hash private_view_key_hash;

    size_t size;
    if (!Common::fromHex(private_spend_key_string, 
                         &private_spend_key_hash, 
                         sizeof(private_spend_key_hash), 
                         size) 
                         || size != sizeof(private_spend_key_hash)) 
    {

      return;
    }

    if (!Common::fromHex(private_view_key_string, &private_view_key_hash, sizeof(private_view_key_hash), size) || size != sizeof(private_spend_key_hash)) {
      return;
    }

    private_spend_key = *(struct Crypto::SecretKey *) &private_spend_key_hash;
    private_view_key = *(struct Crypto::SecretKey *) &private_view_key_hash;
    
    Crypto::PublicKey spendPublicKey;
    Crypto::PublicKey viewPublicKey;
    Crypto::secret_key_to_public_key(private_spend_key, spendPublicKey);
    Crypto::secret_key_to_public_key(private_view_key, viewPublicKey);
      
    CryptoNote::AccountPublicAddress publicKeys;
    publicKeys.spendPublicKey = spendPublicKey;
    publicKeys.viewPublicKey = viewPublicKey;
    
    CryptoNote::AccountKeys keys;
    keys.address = publicKeys;
    keys.spendSecretKey = private_spend_key;
    keys.viewSecretKey = private_view_key;
    
    if (WalletAdapter::instance().isOpen()) 
    {

      WalletAdapter::instance().close();
    }

    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createWithKeys(keys);   
  }
}

/* --------------------------- IMPORT MNEMONIC SEED --------------------------------------- */

void MainWindow::importSeed() 
{

  ImportSeed dlg(this);

  if (dlg.exec() == QDialog::Accepted) 
  {

    QString seed = dlg.getKeyString().trimmed();
    QString filePath = dlg.getFilePath();
    if (seed.isEmpty() || filePath.isEmpty()) 
    {

      return;
    }  

  static std::string languages[] = {"English"};
  static const int num_of_languages = 1;
  static const int mnemonic_phrase_length = 25;

  std::string mnemonic_phrase = seed.toStdString();

  std::vector<std::string> words;

  words = boost::split(words, mnemonic_phrase, ::isspace);

  Crypto::SecretKey private_spend_key;
  Crypto::SecretKey private_view_key;  

  crypto::ElectrumWords::words_to_bytes(mnemonic_phrase, 
                                        private_spend_key, 
                                        languages[0]);
  
  Crypto::PublicKey unused_dummy_variable;

  CryptoNote::AccountBase::generateViewFromSpend(private_spend_key, 
                                                 private_view_key,
                                                 unused_dummy_variable);

  Crypto::PublicKey spendPublicKey;
  Crypto::PublicKey viewPublicKey;
  Crypto::secret_key_to_public_key(private_spend_key, spendPublicKey);
  Crypto::secret_key_to_public_key(private_view_key, viewPublicKey);
    
  CryptoNote::AccountPublicAddress publicKeys;
  publicKeys.spendPublicKey = spendPublicKey;
  publicKeys.viewPublicKey = viewPublicKey;
  
  CryptoNote::AccountKeys keys;
  keys.address = publicKeys;
  keys.spendSecretKey = private_spend_key;
  keys.viewSecretKey = private_view_key;
  
  if (WalletAdapter::instance().isOpen()) 
  {

    WalletAdapter::instance().close();
  }

  WalletAdapter::instance().setWalletFile(filePath);
  WalletAdapter::instance().createWithKeys(keys);   
  }
}

/* --------------------------------------------------------------------------------------- */

void MainWindow::importTracking() {
  ImportTracking dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    QString keyString = dlg.getKeyString().trimmed();
    QString filePath = dlg.getFilePath();
    if (keyString.isEmpty() || filePath.isEmpty()) {
      return;
    }
    if (keyString.size() != 192) {
      QMessageBox::warning(this, tr("Tracking key is not valid"), tr("The tracking key you entered is not valid."), QMessageBox::Ok);
      return;
    }

    if (!filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    CryptoNote::AccountKeys keys;

    std::string public_spend_key_string = keyString.mid(0,64).toStdString();
    std::string public_view_key_string = keyString.mid(64,64).toStdString();
    std::string private_spend_key_string = "0000000000000000000000000000000000000000000000000000000000000000";
    std::string private_view_key_string = keyString.mid(192,64).toStdString();

    Crypto::Hash public_spend_key_hash;
    Crypto::Hash public_view_key_hash;
    Crypto::Hash private_spend_key_hash;
    Crypto::Hash private_view_key_hash;

    size_t size;
    if (!Common::fromHex(public_spend_key_string, &public_spend_key_hash, sizeof(public_spend_key_hash), size) || size != sizeof(public_spend_key_hash)) {
      QMessageBox::warning(this, tr("Key is not valid"), tr("The public spend key you entered is not valid."), QMessageBox::Ok);
      return;
    }
    if (!Common::fromHex(public_view_key_string, &public_view_key_hash, sizeof(public_view_key_hash), size) || size != sizeof(public_view_key_hash)) {
      QMessageBox::warning(this, tr("Key is not valid"), tr("The public view key you entered is not valid."), QMessageBox::Ok);
      return;
    }
    if (!Common::fromHex(private_spend_key_string, &private_spend_key_hash, sizeof(private_spend_key_hash), size) || size != sizeof(private_spend_key_hash)) {
      QMessageBox::warning(this, tr("Key is not valid"), tr("The private spend key you entered is not valid."), QMessageBox::Ok);
      return;
    }
    if (!Common::fromHex(private_view_key_string, &private_view_key_hash, sizeof(private_view_key_hash), size) || size != sizeof(private_spend_key_hash)) {
      QMessageBox::warning(this, tr("Key is not valid"), tr("The private view key you entered is not valid."), QMessageBox::Ok);
      return;
    }

    Crypto::PublicKey public_spend_key = *(struct Crypto::PublicKey *) &public_spend_key_hash;
    Crypto::PublicKey public_view_key = *(struct Crypto::PublicKey *) &public_view_key_hash;
    Crypto::SecretKey private_spend_key = *(struct Crypto::SecretKey *) &private_spend_key_hash;
    Crypto::SecretKey private_view_key = *(struct Crypto::SecretKey *) &private_view_key_hash;

    keys.address.spendPublicKey = public_spend_key;
    keys.address.viewPublicKey = public_view_key;
    keys.spendSecretKey = private_spend_key;
    keys.viewSecretKey = private_view_key;

      if (WalletAdapter::instance().isOpen()) {
        WalletAdapter::instance().close();
      }
      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().createWithKeys(keys);
   // }
  }
}


/* --------------------------- CONNECTION SETTINGS --------------------------------------- */

void MainWindow::nodeSettings() {
  NodeSettings dlg(this);

  dlg.initConnectionSettings();
  dlg.setConnectionMode();
  dlg.setRemoteHost();

  if (dlg.exec() == QDialog::Accepted) {
    QString connection = dlg.setConnectionMode();
    Settings::instance().setConnection(connection);
    if (connection == "remote") {
      QString remoteHost = dlg.setRemoteHost();
      Settings::instance().setCurrentRemoteNode(remoteHost);
    }
    QMessageBox::information(this, 
                             tr("Conection settings saved"), 
                             tr("Please restart the wallet for the new settings to take effect."), 
                             QMessageBox::Ok);
  }
}

void MainWindow::showQRCode(const QString& _address)
{

  ShowQRCode dlg(this);
  dlg.showQR(_address);

  if (dlg.exec() == QDialog::Accepted) 
  {

  }
}



#ifdef Q_OS_WIN
void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason _reason) {
  showNormal();
  m_trayIcon->hide();
}
#endif

}
