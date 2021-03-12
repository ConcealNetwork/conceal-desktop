// Copyright (c) 2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "EditableStyle.h"

#include <QApplication>
#include <QFontDatabase>

#include "Settings.h"

namespace WalletGui
{
  int EditableStyle::setStyles(int change)
  {
    /** Set the base font sizes */
    baseFontSize = change;
    baseTitleSize = 7 + change;
    baseSmallButtonSize = change - 3;
    baseLargeButtonSize = change - 1;

    int id = -2;

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

    font.setFamily(currentFont);
    font.setPixelSize(baseFontSize);
    font.setLetterSpacing(QFont::PercentageSpacing, 102);
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleStrategy(QFont::PreferAntialias);

    smallButtonFont.setFamily(currentFont);
    smallButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    smallButtonFont.setPixelSize(baseSmallButtonSize);

    largeButtonFont.setFamily(currentFont);
    largeButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    largeButtonFont.setPixelSize(baseLargeButtonSize);

    titleFont.setFamily(currentFont);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    titleFont.setPixelSize(baseTitleSize);

    /* Create our common pool of styles */
    tableStyle =
        "QHeaderView::section{font-size:" + QString::number(baseFontSize) +
        "px;background-color:#282d31;color:#fff;font-weight:700;height:37px;border-top:1px solid "
        "#444;border-bottom:1px solid #444}QTreeView::item{color:#ccc;height:37px}";
    b1Style = "QPushButton{font-size: " + QString::number(baseLargeButtonSize) +
              "px; color:#fff;border:1px solid orange;border-radius:5px;} "
              "QPushButton:hover{color:orange;}";
    b2Style = "QPushButton{font-size: " + QString::number(baseSmallButtonSize) +
              "px; color: orange; border:1px solid orange; border-radius: 5px} "
              "QPushButton:hover{color: gold;}";
    fontStyle = "font-size:" + QString::number(baseFontSize - 1) + "px;";

    buttons = getButtons();
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

      /* Set the font and styling for m styled buttons */
      if (button->objectName().contains("m_"))
      {
        button->setFont(font);
      }
    }

    labels = getLabels();
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

    applyStyles();
    return id;
  }

  QList<QPushButton *> EditableStyle::getButtons() { return QList<QPushButton *>(); }

  QList<QLabel *> EditableStyle::getLabels() { return QList<QLabel *>(); }

  void EditableStyle::applyStyles() { }
}  // namespace WalletGui
