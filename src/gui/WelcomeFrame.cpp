// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MainWindow.h"
#include "WelcomeFrame.h"
#include "ui_welcomeframe.h"
#include "Settings.h"
#include <QFont>
#include <QFontDatabase>

namespace WalletGui {

WelcomeFrame::WelcomeFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::WelcomeFrame) {
m_ui->setupUi(this);

int change = 2;

/** Set the base font sizes */
int baseFontSize = 12 + change;
int baseTitleSize = 19 + change;
int baseSmallButtonSize = 9 + change;
int baseLargeButtonSize = 11 + change;

/** Set the font and have two sizes, one for regular text
     * and the other for the titles, we can then later change all the 
     * font sizes */
int id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");

QFont font;
font.setFamily("Poppins");
font.setPixelSize(baseFontSize);

QFont smallButtonFont;
font.setFamily("Poppins");
font.setPixelSize(baseSmallButtonSize);

QFont largeButtonFont;
font.setFamily("Poppins");
font.setPixelSize(baseLargeButtonSize);

QFont titleFont;
titleFont.setFamily("Poppins");
titleFont.setPixelSize(baseTitleSize);

/* Create our common pool of styles */
QString b1Style = "QPushButton{color:#fff;border:1px solid orange;border-radius:5px;}QPushButton:hover{color:orange;border:1px solid orange;border-radius:5px}";
QString b2Style = "QPushButton{color:orange;border:1px solid orange;border-radius:5px}QPushButton#hover{color:gold;border:1px solid orange;font-size:11px;border-radius:5px}";
QString fontStyle = "font-size:" + QString::number(baseFontSize) + "px;";

QList<QPushButton *> buttons = m_ui->groupBox->findChildren<QPushButton *>();
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

m_ui->box1->show();
m_ui->box2->hide();
m_ui->box3->hide();
m_ui->box4->hide();
}

WelcomeFrame::~WelcomeFrame() {
}

void WelcomeFrame::createWallet() {
Q_EMIT createWalletClickedSignal();
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
}

void WelcomeFrame::nextTwo() {
  m_ui->box1->hide();    
  m_ui->box2->hide();
  m_ui->box3->show();
  m_ui->box4->hide();
}


void WelcomeFrame::nextThree() {
  m_ui->box1->hide();    
  m_ui->box2->hide();
  m_ui->box3->hide();
  m_ui->box4->show();
}

}
