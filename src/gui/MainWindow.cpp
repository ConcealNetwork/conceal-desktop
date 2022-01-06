// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MainWindow.h"

#include <Common/Base58.h>
#include <CryptoNoteCore/Account.h>
#include <CryptoNoteCore/CryptoNoteTools.h>

#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QThread>
#include <QTranslator>
#include <QAction>
#include <QMenu>
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
#include "AddressBookModel.h"
#include "ChangePasswordDialog.h"
#include "CurrencyAdapter.h"
#include "ExitWidget.h"
#include "ImportGUIKeyDialog.h"
#include "ImportSeedDialog.h"
#include "NewPasswordDialog.h"
#include "OptimizationManager.h"
#include "PasswordDialog.h"
#include "MainPasswordDialog.h"
#include "Settings.h"
#include "ShowQRCode.h"
#include "TranslatorManager.h"

#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "ImportSecretKeysDialog.h"
#include "ImportTrackingDialog.h"
#include "ui_mainwindow.h"

namespace WalletGui
{

MainWindow *MainWindow::m_instance = nullptr;

MainWindow &MainWindow::instance()
{

  if (m_instance == nullptr)
  {
    m_instance = new MainWindow;
  }
  return *m_instance;
}

MainWindow::MainWindow() : QMainWindow(), m_ui(new Ui::MainWindow), m_trayIcon(nullptr), m_tabActionGroup(new QActionGroup(this)),
                           m_isAboutToQuit(false)
{
  m_ui->setupUi(this);
  connectToSignals();
  initUi();
  walletClosed();
}

MainWindow::~MainWindow()
{
}

void MainWindow::connectToSignals()
{
  connect(&WalletAdapter::instance(), &WalletAdapter::openWalletWithPasswordSignal, this, &MainWindow::askForWalletPassword, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::changeWalletPasswordSignal, this, &MainWindow::encryptWallet, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &MainWindow::walletOpened);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &MainWindow::walletClosed);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionCreatedSignal, this, [this]() {
    QApplication::alert(this);
  });

  /* Transaction sent, bring the user to the overview */
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &MainWindow::dashboardTo);

  /* This is the previous method that sent the user to the transaction history screen everytime a transaction was sent
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, [this](cn::TransactionId _transactionId, int _error, const QString &_errorString) {
    if (_error == 0)
    {
      m_ui->m_transactionsAction->setChecked(true);
    }
  });

*/

  connect(m_ui->m_receiveFrame, &ReceiveFrame::backupSignal, this, &MainWindow::backupWallet);
  connect(m_ui->m_overviewFrame, &OverviewFrame::payToSignal, this, &MainWindow::payTo);
  connect(m_ui->m_overviewFrame, &OverviewFrame::messageToSignal, this, &MainWindow::messageTo);

  connect(m_ui->m_overviewFrame, &OverviewFrame::newWalletSignal, this, &MainWindow::createWallet, Qt::QueuedConnection);
  connect(m_ui->m_overviewFrame, &OverviewFrame::welcomeFrameSignal, this, &MainWindow::welcomeFrame);

  connect(m_ui->m_welcomeFrame, &WelcomeFrame::openWalletClickedSignal, this, &MainWindow::openWallet, Qt::QueuedConnection);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importSeedClickedSignal, this, &MainWindow::importSeed, Qt::QueuedConnection);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importsecretkeysClickedSignal, this, &MainWindow::importSecretkeys, Qt::QueuedConnection);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::importKeyClickedSignal, this, &MainWindow::importKey, Qt::QueuedConnection);

  /* signals from overview frame buttons */
  connect(m_ui->m_overviewFrame, &OverviewFrame::openWalletSignal, this, &MainWindow::openWallet, Qt::QueuedConnection);
  connect(m_ui->m_overviewFrame, &OverviewFrame::backupSignal, this, &MainWindow::backupTo);
  connect(m_ui->m_overviewFrame, &OverviewFrame::backupFileSignal, this, &MainWindow::backupWallet);
  connect(m_ui->m_overviewFrame, &OverviewFrame::rescanSignal, this, &MainWindow::rescanTo);
  connect(m_ui->m_overviewFrame, &OverviewFrame::aboutQTSignal, this, &MainWindow::aboutQt);

  connect(m_ui->m_overviewFrame, &OverviewFrame::qrSignal, this, &MainWindow::showQRCode);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importSeedSignal, this, &MainWindow::importSeed);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importGUIKeySignal, this, &MainWindow::importKey);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importTrackingKeySignal, this, &MainWindow::importTracking);
  connect(m_ui->m_overviewFrame, &OverviewFrame::importSecretKeysSignal, this, &MainWindow::importSecretkeys);
  connect(m_ui->m_overviewFrame, &OverviewFrame::encryptWalletSignal, this, &MainWindow::encryptWallet);
  connect(m_ui->m_overviewFrame, &OverviewFrame::closeWalletSignal, this, &MainWindow::closeWallet);

  connect(m_ui->m_receiveFrame, &ReceiveFrame::backSignal, this, &MainWindow::dashboardTo);

  connect(m_ui->m_overviewFrame, &OverviewFrame::notifySignal, this, &MainWindow::notify);
  connect(m_ui->m_receiveFrame, &ReceiveFrame::notifySignal, this, &MainWindow::notify);
  connect(m_ui->m_welcomeFrame, &WelcomeFrame::notifySignal, this, &MainWindow::notify);
}

void MainWindow::initUi()
{
#ifndef QT_NO_SYSTEMTRAYICON32
  if (QSystemTrayIcon::isSystemTrayAvailable())
  {
    QAction* showAction = new QAction(tr("Show"), this);
    connect(showAction, &QAction::triggered, this, &MainWindow::restoreFromTray);

    QAction* quitAction = new QAction(tr("Quit Conceal Desktop"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    QMenu* trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QPixmap(":/images/conceal-logo"));
    m_trayIcon->setContextMenu(trayIconMenu);
#ifndef Q_OS_MAC
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
#endif
  }
#endif

  if (Settings::instance().getMaximizedStatus() == "enabled") {
    showMaximized();
  }
    
  setRemoteWindowTitle();

  m_ui->m_overviewFrame->hide();
  m_ui->m_receiveFrame->hide();

  m_tabActionGroup->addAction(m_ui->m_overviewAction);
  m_tabActionGroup->addAction(m_ui->m_receiveAction);

  m_ui->m_overviewAction->toggle();

#ifdef Q_OS_MAC
  installDockHandler();
#endif

  // OptimizationManager *optimizationManager = new OptimizationManager(this);
  notification = new Notification(this);
  EditableStyle::setStyles(Settings::instance().getFontSize());
}

#ifndef QT_NO_SYSTEMTRAYICON
void MainWindow::restoreFromTray()
{
  activateWindow();
  showNormal();
}

void MainWindow::minimizeToTray(bool _on)
{
  if (_on)
  {
    hide();
    m_trayIcon->show();
  }
  else
  {
    m_trayIcon->hide();
  }
}
#endif

void MainWindow::scrollToTransaction(const QModelIndex &_index)
{
  m_ui->m_overviewFrame->scrollToTransaction(_index);
}

void MainWindow::quit()
{
  Settings::instance().setCurrentFeeAddress("");
  if (!m_isAboutToQuit)
  {
    ExitWidget *exitWidget = new ExitWidget(nullptr);
    exitWidget->show();
    m_isAboutToQuit = true;
    close();
  }
}

#ifdef Q_OS_MAC
void MainWindow::restoreFromDock()
{
  if (m_isAboutToQuit)
  {
    return;
  }

  showNormal();
}
#endif

void MainWindow::closeEvent(QCloseEvent *_event)
{
#ifndef QT_NO_SYSTEMTRAYICON
  if (m_isAboutToQuit)
  {
    QMainWindow::closeEvent(_event);
    return;
  }
  else if (Settings::instance().isCloseToTrayEnabled())
  {
    minimizeToTray(true);
    _event->ignore();
  }
  else
  {
    QApplication::quit();
    return;
  }
#elif defined(Q_OS_LINUX)
  if (!m_isAboutToQuit)
  {
    QApplication::quit();
    return;
  }
#endif
  QMainWindow::closeEvent(_event);
}

#ifndef QT_NO_SYSTEMTRAYICON
void MainWindow::changeEvent(QEvent* _event)
{
  QMainWindow::changeEvent(_event);
  if (!QSystemTrayIcon::isSystemTrayAvailable())
  {
    QMainWindow::changeEvent(_event);
    return;
  }
  switch (_event->type())
  {
    case QEvent::WindowStateChange:
    {
      if (Settings::instance().isMinimizeToTrayEnabled())
      {
        minimizeToTray(isMinimized());
      }
      break;
    }
    default:
      break;
  }
}
#endif

bool MainWindow::event(QEvent *_event)
{
  switch (static_cast<WalletEventType>(_event->type()))
  {
  case WalletEventType::ShowMessage:
    showMessage(static_cast<ShowMessageEvent *>(_event)->messageText(), static_cast<ShowMessageEvent *>(_event)->messageType());
    return true;
  }

  return QMainWindow::event(_event);
}

void MainWindow::setRemoteWindowTitle()
{
  QString connection = Settings::instance().getConnection();
  if ((connection == "remote") || (connection == "autoremote"))
  {
    if (Settings::instance().isTrackingMode())
    {
      setWindowTitle(QString("%1 Wallet %2 connected to remote node %3 [Tracking Wallet]").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()).arg(Settings::instance().getCurrentRemoteNode()));
    }
    else
    {
      setWindowTitle(QString("%1 Wallet %2 connected to remote node %3").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()).arg(Settings::instance().getCurrentRemoteNode()));
    }
  }
  else
  {
    if (Settings::instance().isTrackingMode())
    {
      setWindowTitle(QString("%1 Wallet %2 [Tracking Wallet]").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()));
    }
    else
    {
      setWindowTitle(QString("%1 Wallet %2").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).arg(Settings::instance().getVersion()));
    }
  }
}

void MainWindow::delay()
{
  QTime dieTime = QTime::currentTime().addSecs(2);
  while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/* ----------------------------- CREATE A NEW WALLET ------------------------------------ */

void MainWindow::createWallet()
{
  m_ui->m_overviewFrame->hide();
  m_ui->m_welcomeFrame->show();
  m_ui->m_welcomeFrame->createWallet();
}

void MainWindow::welcomeFrame() {
  m_ui->m_overviewFrame->hide();
  m_ui->m_welcomeFrame->show();
  m_ui->m_welcomeFrame->nextThree();
}

void MainWindow::openWallet()
{
  bool welcomeFrameVisible = m_ui->m_welcomeFrame->isVisible();
  m_ui->m_welcomeFrame->hide();
  QString walletFile = Settings::instance().getWalletFile();
  std::string wallet = walletFile.toStdString();

  QString walletDirectory = "";
  if (!wallet.empty())
  {
    QFileInfo fileInfo(walletFile);
    walletDirectory = fileInfo.absolutePath();
  }
  else
  {
    walletDirectory = Settings::instance().getDefaultWalletDir();
  }

  QString filePath = QFileDialog::getOpenFileName(this, tr("Open .wallet/.keys file"),
                                                  walletDirectory,
                                                  tr("Wallet (*.wallet *.keys)"));

  if (!filePath.isEmpty())
  {
    if (WalletAdapter::instance().isOpen())
    {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().open("");
  }
  m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
  m_ui->m_overviewFrame->dashboardClicked();
}

void MainWindow::closeWallet()
{
  m_ui->m_welcomeFrame->show();
  WalletAdapter::instance().close();
  walletClosed();
}

void MainWindow::importKey()
{
  bool welcomeFrameVisible = m_ui->m_welcomeFrame->isVisible();
  m_ui->m_welcomeFrame->hide();
  ImportGUIKeyDialog dlg(this);
  dlg.setModal(true);
  dlg.setWindowFlags(Qt::FramelessWindowHint);
  dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);
  if (dlg.exec() == QDialog::Accepted)
  {
    QString keyString = dlg.getKeyString().trimmed();
    QString filePath = dlg.getFilePath();

    if (keyString.isEmpty() || filePath.isEmpty())
    {
      m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
      return;
    }

    if (!filePath.endsWith(".wallet"))
    {
      filePath.append(".wallet");
    }

    uint64_t addressPrefix;
    std::string data;
    cn::AccountKeys keys;

    if (tools::base_58::decode_addr(keyString.toStdString(), addressPrefix, data) && addressPrefix == CurrencyAdapter::instance().getAddressPrefix() &&
        data.size() == sizeof(keys))
    {
      std::memcpy(&keys, data.data(), sizeof(keys));

      if (WalletAdapter::instance().isOpen())
      {
        WalletAdapter::instance().close();
      }

      WalletAdapter::instance().setWalletFile(filePath);
      WalletAdapter::instance().createWithKeys(keys);
    }
    else if (tools::base_58::decode_addr(keyString.toStdString(), addressPrefix, data) && addressPrefix == CurrencyAdapter::instance().getAddressPrefix())
    {

      //serialize as public keys then convert to secret keys
      cn::AccountPublicAddress decodedKeys;
      cn::fromBinaryArray(decodedKeys, common::asBinaryArray(data));

      // convert to secret key then get real public keys
      crypto::SecretKey spendSecretKey = reinterpret_cast<crypto::SecretKey &>(decodedKeys.spendPublicKey);
      crypto::PublicKey spendPublicKey;
      crypto::secret_key_to_public_key(spendSecretKey, spendPublicKey);

      crypto::SecretKey viewSecretKey = reinterpret_cast<crypto::SecretKey &>(decodedKeys.viewPublicKey);
      crypto::PublicKey viewPublicKey;
      crypto::secret_key_to_public_key(viewSecretKey, viewPublicKey);

      cn::AccountPublicAddress publicKeys;
      publicKeys.spendPublicKey = spendPublicKey;
      publicKeys.viewPublicKey = viewPublicKey;

      cn::AccountKeys keys;
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
  m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
}

void MainWindow::backupWallet()
{
  QString filePath = QFileDialog::getSaveFileName(this, tr("Backup wallet to..."),
                                                  Settings::instance().getDefaultWalletDir(),
                                                  tr("Wallets (*.wallet)"));
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet"))
  {
    filePath.append(".wallet");
  }

  if (!filePath.isEmpty() && !QFile::exists(filePath))
  {
    WalletAdapter::instance().backup(filePath);
  }
}

void MainWindow::resetWallet()
{
  Q_ASSERT(WalletAdapter::instance().isOpen());
  if (QMessageBox::warning(this, tr("Warning"), tr("Your wallet will be reset and restored from blockchain.\n"
                                                   "Are you sure?"),
                           QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
  {
    WalletAdapter::instance().reset();
    WalletAdapter::instance().open("");
  }
}

void MainWindow::encryptWallet()
{
  if (Settings::instance().isEncrypted())
  {
    bool error = false;
    do
    {
      ChangePasswordDialog dlg(this);
      dlg.setModal(true);
      dlg.setWindowFlags(Qt::FramelessWindowHint);
      dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);
      if (dlg.exec() == QDialog::Rejected)
      {
        return;
      }

      QString oldPassword = dlg.getOldPassword();
      QString newPassword = dlg.getNewPassword();
      error = !WalletAdapter::instance().changePassword(oldPassword, newPassword);
    } while (error);
  }
  else
  {
    NewPasswordDialog dlg(this);
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::FramelessWindowHint);
    dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);
    bool error = false;
    do
    {
      if (dlg.exec() == QDialog::Accepted)
      {
        QString password = dlg.getPassword();
        if (password.isEmpty())
        {
          return;
        }
        error = !WalletAdapter::instance().changePassword("", password);
      }
    } while (error);
  }
}

void MainWindow::aboutQt()
{
  QMessageBox::aboutQt(this);
}

void MainWindow::setStartOnLogin(bool _on)
{
  Settings::instance().setStartOnLoginEnabled(_on);
}

void MainWindow::setMinimizeToTray(bool _on)
{
#ifndef QT_NO_SYSTEMTRAYICON
  Settings::instance().setMinimizeToTrayEnabled(_on);
#endif
}

void MainWindow::setCloseToTray(bool _on)
{
#ifndef QT_NO_SYSTEMTRAYICON
  Settings::instance().setCloseToTrayEnabled(_on);
#endif
}

void MainWindow::showMessage(const QString &_text, QtMsgType _type)
{
  switch (_type)
  {
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

void MainWindow::askForWalletPassword(bool _error) {
  /* hide the welcome frame when waiting for the password */
  m_ui->m_welcomeFrame->hide();

  MainPasswordDialog dlg(_error, this);
  dlg.setModal(true);
  dlg.setWindowFlags(Qt::FramelessWindowHint);
  dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);

  if (dlg.exec() == QDialog::Accepted) {
    QString password = dlg.getPassword();
    WalletAdapter::instance().open(password);
  } else {
    welcomeFrame();
  }
}

void MainWindow::walletOpened(bool _error, const QString &_error_text)
{
  m_ui->m_welcomeFrame->hide();
  if (!_error)
  {
    QList<QAction *> tabActions = m_tabActionGroup->actions();
    Q_FOREACH (auto action, tabActions)
    {
      action->setEnabled(true);
    }
    m_ui->m_overviewAction->trigger();
    m_ui->m_overviewFrame->show();
    checkTrackingMode();
    setRemoteWindowTitle();
  }
  else
  {
    walletClosed();
  }
}

/* ----------------------------- WALLET CLOSED ------------------------------------ */
/* this is what happens when the wallet goes into a closed state which includes the 
   period between closing the current wallet and opening/creating a new one */

void MainWindow::walletClosed()
{
  /* frames */
  m_ui->m_overviewFrame->hide();
  m_ui->m_welcomeFrame->show();

  /* labels */
  QList<QAction *> tabActions = m_tabActionGroup->actions();
  Q_FOREACH (auto action, tabActions)
  {
    action->setEnabled(false);
  }
}

void MainWindow::checkTrackingMode()
{
  cn::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  if (keys.spendSecretKey == boost::value_initialized<crypto::SecretKey>())
  {
    Settings::instance().setTrackingMode(true);
  }
  else
  {
    Settings::instance().setTrackingMode(false);
  }
}

void MainWindow::payTo(const QModelIndex& _index)
{
  if (_index.data(AddressBookModel::ROLE_PAYMENTID).toString() != "")
  {
    m_ui->m_overviewFrame->setPaymentId(_index.data(AddressBookModel::ROLE_PAYMENTID).toString());
  }
  else
  {
    m_ui->m_overviewFrame->setPaymentId("");
  }
  m_ui->m_overviewFrame->setAddress(_index.data(AddressBookModel::ROLE_ADDRESS).toString());
  m_ui->m_overviewFrame->show();
  m_ui->m_overviewAction->trigger();
  m_ui->m_overviewFrame->raise();
}

void MainWindow::messageTo(const QModelIndex& _index)
{
  m_ui->m_overviewFrame->setAddress(_index.data(AddressBookModel::ROLE_ADDRESS).toString());
  m_ui->m_overviewFrame->show();
  m_ui->m_overviewAction->trigger();
  m_ui->m_overviewFrame->raise();
}

void MainWindow::dashboardTo()
{
  m_ui->m_overviewFrame->show();
  m_ui->m_overviewAction->trigger();
  m_ui->m_overviewFrame->raise();
}

void MainWindow::backupTo()
{
  m_ui->m_receiveAction->trigger();
}

void MainWindow::rescanTo()
{
  MainWindow::resetWallet();
}

/* --------------------------- IMPORT SECRET KEYS --------------------------------------- */

void MainWindow::importSecretkeys()
{
  bool welcomeFrameVisible = m_ui->m_welcomeFrame->isVisible();
  m_ui->m_welcomeFrame->hide();
  ImportSecretKeysDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_ui->m_overviewFrame->dashboardClicked();
  }
  m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
}

void MainWindow::importSeed()
{
  bool welcomeFrameVisible = m_ui->m_welcomeFrame->isVisible();
  m_ui->m_welcomeFrame->hide();
  ImportSeedDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_ui->m_overviewFrame->dashboardClicked();
  }
  m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
}

void MainWindow::importTracking()
{
  bool welcomeFrameVisible = m_ui->m_welcomeFrame->isVisible();
  m_ui->m_welcomeFrame->hide();
  ImportTrackingDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    m_ui->m_overviewFrame->dashboardClicked();
  }
  m_ui->m_welcomeFrame->setVisible(welcomeFrameVisible);
}

void MainWindow::showQRCode(const QString &_address)
{
  ShowQRCode dlg(this);
  dlg.showQR(_address);
  dlg.exec();
}

#ifndef QT_NO_SYSTEMTRAYICON
void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason _reason)
{
  if (_reason == QSystemTrayIcon::Trigger)
  {
    restoreFromTray();
  }
}
#endif

void MainWindow::notify(const QString& message) {
  notification->notify(message);
}

QList<QWidget *> MainWindow::getWidgets() { return m_ui->centralwidget->findChildren<QWidget *>(); }

QList<QPushButton *> MainWindow::getButtons()
{
  return m_ui->centralwidget->findChildren<QPushButton *>();
}

QList<QLabel *> MainWindow::getLabels() { return m_ui->centralwidget->findChildren<QLabel *>(); }

void MainWindow::applyStyles()
{
  QApplication::setFont(EditableStyle::currentFont);
  m_ui->centralwidget->update();
}

}  // namespace WalletGui
