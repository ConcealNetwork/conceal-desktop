// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WelcomeFrame.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFont>
#include "Settings.h"
#include <QFontDatabase>
#include <QMessageBox>

#include "MainWindow.h"
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
  else {
    QMessageBox::warning(
        &MainWindow::instance(), QObject::tr("Error"),
        tr("Unable to create a new wallet at the path provided. Please select another."));
  }
}

void WelcomeFrame::selectPathClicked()
{
  QString filePath = QFileDialog::getSaveFileName(
      this, tr("New wallet file"), QDir::homePath(), tr("Wallets (*.wallet)"));

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
  }
  else {
    QMessageBox::warning(
        &MainWindow::instance(), QObject::tr("Seed confirmation error"),
        tr("The words entered does not match the seed. Please try again."));
  }
}

void WelcomeFrame::setStyles(int change)
{
  /** Set the base font sizes */
  int baseFontSize = change;
  int baseTitleSize = 7 + change;
  int baseSmallButtonSize = change - 3;
  int baseLargeButtonSize = change - 1;

  int id;

  QString currentFont = Settings::instance().getFont();
  if (currentFont == "Poppins")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");
  }
  if (currentFont == "Lekton")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/Lekton-Regular.ttf");
  }
  if (currentFont == "Roboto")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/RobotoSlab-Regular.ttf");
  }
  if (currentFont == "Montserrat")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/Montserrat-Regular.ttf");
  }
  if (currentFont == "Open Sans")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
  }
  if (currentFont == "Oswald")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/Oswald-Regular.ttf");
  }
  if (currentFont == "Lato")
  {
    id = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
  }

  QFont font;
  font.setFamily(currentFont);
  font.setPixelSize(baseFontSize);
  font.setLetterSpacing(QFont::PercentageSpacing, 102);
  font.setHintingPreference(QFont::PreferFullHinting);
  font.setStyleStrategy(QFont::PreferAntialias);

  QFont smallButtonFont;
  smallButtonFont.setFamily("Poppins");
  smallButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
  smallButtonFont.setPixelSize(baseSmallButtonSize);

  QFont largeButtonFont;
  largeButtonFont.setFamily("Poppins");
  largeButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
  largeButtonFont.setPixelSize(baseLargeButtonSize);

  QFont titleFont;
  titleFont.setFamily("Poppins");
  titleFont.setLetterSpacing(QFont::PercentageSpacing, 102);
  titleFont.setPixelSize(baseTitleSize);

  /* Create our common pool of styles */
  QString tableStyle = "QHeaderView::section{font-size:" + QString::number(baseFontSize) + "px;background-color:#282d31;color:#fff;font-weight:700;height:37px;border-top:1px solid #444;border-bottom:1px solid #444}QTreeView::item{color:#ccc;height:37px}";
  QString b1Style = "QPushButton{font-size: " + QString::number(baseLargeButtonSize) + "px; color:#fff;border:1px solid orange;border-radius:5px;} QPushButton:hover{color:orange;}";
  QString b2Style = "QPushButton{font-size: " + QString::number(baseSmallButtonSize) + "px; color: orange; border:1px solid orange; border-radius: 5px} QPushButton:hover{color: gold;}";
  QString fontStyle = "font-size:" + QString::number(baseFontSize - 1) + "px;";

  QList<QPushButton *>
      buttons = m_ui->groupBox->findChildren<QPushButton *>();
  foreach (QPushButton *button, buttons)
  {
    /* Set the font and styling for b1 styled buttons */
    if (button->objectName().contains("b1_"))
    {
      button->setStyleSheet(b1Style);
      button->setFont(largeButtonFont);
    }

    /* Set the font and styling for b2 styled buttons */
    if (button->objectName().contains("b2_"))
    {
      button->setStyleSheet(b2Style);
      button->setFont(smallButtonFont);
    }

    /* Set the font and styling for lm styled buttons */
    if (button->objectName().contains("lm_"))
    {
      button->setFont(font);
    }

    /* Set the font and styling for sm styled buttons */
    if (button->objectName().contains("sm_"))
    {
      button->setFont(font);
    }
  }

  QList<QLabel *> labels = m_ui->groupBox->findChildren<QLabel *>();
  foreach (QLabel *label, labels)
  {
    if (label->objectName().contains("title_"))
    {
      label->setFont(titleFont);
    }
    else
    {
      label->setFont(font);
    }
  }

  m_ui->groupBox->update();
}

}  // namespace WalletGui
