// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WelcomeFrame.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>

#include "MainWindow.h"
#include "Settings.h"
#include "WalletAdapter.h"
#include "ui_welcomeframe.h"

namespace WalletGui {

WelcomeFrame::WelcomeFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::WelcomeFrame) {
m_ui->setupUi(this);

int startingFontSize = Settings::instance().getFontSize();
setStyles(startingFontSize);

m_ui->box1->show();
m_ui->box2->hide();
m_ui->box3->hide();
m_ui->box4->hide();
m_ui->newWalletBox->hide();
}

WelcomeFrame::~WelcomeFrame() {
}

void WelcomeFrame::createWallet() {
  m_ui->box1->hide();
  m_ui->box2->hide();
  m_ui->box3->hide();  
  m_ui->box4->hide();
  m_ui->newWalletBox->show();
  m_ui->walletPathBox->show();
  m_ui->showSeedBox->hide();
  m_ui->confirmSeedBox->hide();
  m_ui->lineEdit->setText(Settings::instance().getDefaultWalletPath());
}

void WelcomeFrame::openWallet() {
Q_EMIT openWalletClickedSignal();
}

void WelcomeFrame::importSeed() {
Q_EMIT importSeedClickedSignal();
}

void WelcomeFrame::importsecretkeys() {
Q_EMIT importsecretkeysClickedSignal();
}

void WelcomeFrame::importKey() {
Q_EMIT importKeyClickedSignal();
}

void WelcomeFrame::nextOne() {
  m_ui->box1->hide();
  m_ui->box2->show();
  m_ui->box3->hide();  
  m_ui->box4->hide();
  m_ui->newWalletBox->hide();
}

void WelcomeFrame::nextTwo() {
  m_ui->box1->hide();    
  m_ui->box2->hide();
  m_ui->box3->show();
  m_ui->box4->hide();
  m_ui->newWalletBox->hide();
}


void WelcomeFrame::nextThree() {
  m_ui->box1->hide();    
  m_ui->box2->hide();
  m_ui->box3->hide();
  m_ui->box4->show();
  m_ui->newWalletBox->hide();
}

void WelcomeFrame::nextShowSeed()
{
  QString filePath = m_ui->lineEdit->text();

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  if (!filePath.isEmpty() && !QFile::exists(filePath)) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    m_ui->walletPathBox->hide();
    m_ui->showSeedBox->show();
    m_ui->confirmSeedBox->hide();
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createWallet();
    // add the observer once the wallet is created (if added before it is not confirming the seed
    // and jumping directly to the dashboard)
    WalletAdapter::instance().addObserver();
    std::string seed;
    if (WalletAdapter::instance().getMnemonicSeed(seed)) {
      m_ui->mnemonicSeed->setText(QString::fromStdString(seed));
    }
    else {
      QMessageBox::warning(&MainWindow::instance(), QObject::tr("Error"), tr("Unable to create the wallet."));
      nextThree();
    }
  }
  else
  {
    QMessageBox::warning(&MainWindow::instance(), QObject::tr("Error"),
                         tr("Unable to create a new wallet at the path provided. \n"
                            "Please choose another location."));
  }
}

void WelcomeFrame::selectPathClicked()
{
  QString filePath = QFileDialog::getSaveFileName(this, tr("New wallet file"),
                                                  Settings::instance().getDefaultWalletPath(),
                                                  tr("Wallets (*.wallet)"));
  if (!filePath.isEmpty() && !filePath.endsWith(".wallet"))
  {
    filePath.append(".wallet");
  }
  m_ui->lineEdit->setText(filePath);
}

void WelcomeFrame::copySeedClicked()
{
  QApplication::clipboard()->setText(m_ui->mnemonicSeed->text());
  Q_EMIT notifySignal("Seed copied to clipboard");
}

void WelcomeFrame::nextConfirmSeed()
{
  m_ui->walletPathBox->hide();
  m_ui->showSeedBox->hide();
  m_ui->confirmSeedBox->show();
}

void WelcomeFrame::backShowSeed()
{
  m_ui->walletPathBox->hide();
  m_ui->showSeedBox->show();
  m_ui->confirmSeedBox->hide();
}

void WelcomeFrame::nextValidate()
{
  if (!m_ui->validationCheckBox->isChecked())
  {
    QMessageBox::warning(
        &MainWindow::instance(), QObject::tr("Error"),
        tr("You must confirm that you have safely stored the mnemonic seed and understand that the "
           "Conceal Team cannot restore this wallet and is not responsible for loss of funds."));
    return;
  }
  QString walletSeed = m_ui->mnemonicSeed->text();
  QString confirmedWalletSeed = m_ui->mnemonicSeedConfirmation->toPlainText();
  if (walletSeed == confirmedWalletSeed) {
    if (WalletAdapter::instance().isOpen()) {
      WalletAdapter::instance().close();
    }
    WalletAdapter::instance().open("");
    m_ui->mnemonicSeedConfirmation->clear();
    m_ui->lineEdit->clear();
    m_ui->validationCheckBox->setChecked(false);
  }
  else {
    QMessageBox::warning(
        &MainWindow::instance(), QObject::tr("Seed confirmation error"),
        tr("The words entered does not match the seed. Please try again."));
  }
}

QList<QWidget *> WelcomeFrame::getWidgets() { return m_ui->groupBox->findChildren<QWidget *>(); }

QList<QPushButton *> WelcomeFrame::getButtons()
{
  return m_ui->groupBox->findChildren<QPushButton *>();
}

QList<QLabel *> WelcomeFrame::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

void WelcomeFrame::applyStyles() { m_ui->groupBox->update(); }

}  // namespace WalletGui
