// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QTime>
#include <QMessageBox>

#include "MainWindow.h"
#include "BankingFrame2.h"
#include "WalletAdapter.h"
#include "Settings.h"

#include "ui_bankingframe2.h"

namespace WalletGui
{

BankingFrame2::BankingFrame2(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::BankingFrame2)
{

  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &BankingFrame2::synchronizationCompleted, Qt::QueuedConnection);


  m_ui->setupUi(this);

  /* Get current language */
  QString language = Settings::instance().getLanguage();
  if (language.compare("tr") == 0)
  {
    m_ui->m_turkish->setChecked(true);
  }
  else if (language.compare("ru") == 0)
  {
    m_ui->m_russian->setChecked(true);
  }
  else if (language.compare("cn") == 0)
  {
    m_ui->m_chinese->setChecked(true);
  }
  else
  {
    m_ui->m_english->setChecked(true);
  }

  /* Get current currency */
  QString currency = Settings::instance().getCurrentCurrency();
  if (currency.compare("EUR") == 0)
  {
    m_ui->m_eur->setChecked(true);
  }
  else 
  {
    m_ui->m_usd->setChecked(true);
  }


  m_ui->m_minToTrayButton->setText(tr("CLICK TO ENABLE"));
  m_ui->m_closeToTrayButton->setText(tr("CLICK TO ENABLE"));

#ifdef Q_OS_WIN
  /* Set minimize to tray button status */
  if (!Settings::instance().isMinimizeToTrayEnabled())
  {
    m_ui->m_minToTrayButton->setText(tr("CLICK TO ENABLE"));
  }
  else
  {
    m_ui->m_minToTrayButton->setText(tr("CLICK TO DISABLE"));
  }

  /* Set close to tray button status */
  if (!Settings::instance().isCloseToTrayEnabled())
  {
    m_ui->m_closeToTrayButton->setText(tr("CLICK TO ENABLE"));
  }
  else
  {
    m_ui->m_closeToTrayButton->setText(tr("CLICK TO DISABLE"));
  }
#endif

  /* Set current connection options */
  QString connection = Settings::instance().getConnection();
  QString remoteHost = Settings::instance().getCurrentRemoteNode();
  m_ui->m_hostEdit->setText(remoteHost);

  /* If the connection is a remote node, then load the current (or default)
      remote node into the text field. */
  if (connection.compare("remote") == 0)
  {
    m_ui->radioButton->setChecked(true);
  }

  if (connection.compare("autoremote") == 0)
  {
    m_ui->radioButton_3->setChecked(true);
  }
  /* It is an embedded node, so let only check that */
  else if (connection.compare("embedded") == 0)
  {
    m_ui->radioButton_2->setChecked(true);
  }

 if (Settings::instance().getAutoOptimizationStatus() == "enabled") {
   m_ui->m_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));
 }
 else
 {
   m_ui->m_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));   
 }






}

BankingFrame2::~BankingFrame2()
{
}

void BankingFrame2::optimizeClicked()
{
  quint64 numUnlockedOutputs;
  numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
  WalletAdapter::instance().optimizeWallet();
  while (WalletAdapter::instance().getNumUnlockedOutputs() > 100)
  {
    numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
    if (numUnlockedOutputs == 0)
      break;
    WalletAdapter::instance().optimizeWallet();
    delay();
  }
  backClicked();
}

void BankingFrame2::autoOptimizeClicked() 
{
 if (Settings::instance().getAutoOptimizationStatus() == "enabled") {
   Settings::instance().setAutoOptimizationStatus("disabled");
   m_ui->m_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));
  QMessageBox::information(this,
                           tr("Auto Optimization"),
                           tr("Auto Optimization Disabled."),
                           QMessageBox::Ok);   
 }
 else
 {
   Settings::instance().setAutoOptimizationStatus("enabled");
   m_ui->m_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));   
  QMessageBox::information(this,
                           tr("Auto Optimization"),
                           tr("Auto Optimization Enabled. Your wallet will be optimized automatically every 15 minutes."),
                           QMessageBox::Ok);      
 }  
}

void BankingFrame2::synchronizationCompleted() {
  quint64 numUnlockedOutputs;
  numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
  if (numUnlockedOutputs >= 100)
  {
    m_ui->m_optimizationMessage->setText("Optimization recommended [" + QString::number(numUnlockedOutputs) + " outputs]");
  }
  else
  {
    m_ui->m_optimizationMessage->setText("Optimization not required [" + QString::number(numUnlockedOutputs) + " outputs]");
  }  
}




void BankingFrame2::delay()
{
  QTime dieTime = QTime::currentTime().addSecs(2);
  while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void BankingFrame2::saveLanguageClicked()
{
  QString language;
  if (m_ui->m_russian->isChecked())
  {
    language = "ru";
  }
  else if (m_ui->m_turkish->isChecked())
  {
    language = "tr";
  }
  else if (m_ui->m_chinese->isChecked())
  {
    language = "cn";
  }
  else
  {
    language = "en";
  }
  Settings::instance().setLanguage(language);

  QMessageBox::information(this,
                           tr("Language settings saved"),
                           tr("Please restart the wallet for the new settings to take effect."),
                           QMessageBox::Ok);
}

void BankingFrame2::saveCurrencyClicked()
{
  QString currency;
  if (m_ui->m_eur->isChecked())
  {
    currency = "EUR";
  }
  else 
  {
    currency = "USD";
  }
  Settings::instance().setCurrentCurrency(currency);

  QMessageBox::information(this,
                           tr("Currecy settings saved"),
                           tr("Please restart the wallet for the new settings to take effect."),
                           QMessageBox::Ok);
}

void BankingFrame2::saveConnectionClicked()
{
  QString connectionMode;
  if (m_ui->radioButton->isChecked())
  {
    connectionMode = "remote";
  }
  else if (m_ui->radioButton_2->isChecked())
  {
    connectionMode = "embedded";
  }
  else if (m_ui->radioButton_3->isChecked())
  {
    connectionMode = "autoremote";
  }
  Settings::instance().setConnection(connectionMode);

  QString remoteHost;
  /* If it is a remote connection, commit the entered remote node. There is no validation of the 
     remote node. If the connection is embedded then take no action */
  if (m_ui->radioButton->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  if (m_ui->radioButton_3->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  Settings::instance().setCurrentRemoteNode(remoteHost);

  QMessageBox::information(this,
                           tr("Connection settings saved"),
                           tr("Please restart the wallet for the new settings to take effect."),
                           QMessageBox::Ok);
}

void BankingFrame2::minToTrayClicked()
{
#ifdef Q_OS_WIN  
  if (!Settings::instance().isMinimizeToTrayEnabled())
  {
    Settings::instance().setMinimizeToTrayEnabled(true);
    m_ui->m_minToTrayButton->setText(tr("CLICK TO DISABLE"));
  }
  else
  {
    Settings::instance().setMinimizeToTrayEnabled(false);
    m_ui->m_minToTrayButton->setText(tr("CLICK TO ENABLE"));
  }
#endif  
}

void BankingFrame2::closeToTrayClicked()
{
#ifdef Q_OS_WIN  
  if (!Settings::instance().isCloseToTrayEnabled())
  {
    Settings::instance().setCloseToTrayEnabled(true);
    m_ui->m_closeToTrayButton->setText(tr("CLICK TO DISABLE"));
  }
  else
  {
    Settings::instance().setCloseToTrayEnabled(false);
    m_ui->m_closeToTrayButton->setText(tr("CLICK TO ENABLE"));
  }
#endif  
}

void BankingFrame2::backClicked()
{
  Q_EMIT backSignal();
}

void BankingFrame2::rescanClicked()
{
  Q_EMIT rescanSignal();
}

} // namespace WalletGui
