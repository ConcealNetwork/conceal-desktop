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
  EditableStyle::~EditableStyle()
  {
    widgets.clear();
    buttons.clear();
    labels.clear();
  }

  int EditableStyle::setStyles(int change)
  {
    /** Set the base font sizes */
    baseFontSize = change;
    baseTitleSize = change + 7;
    baseSmallButtonSize = change - 3;
    baseLargeButtonSize = change - 1;

    int id = -2;

    QString fontName = Settings::instance().getFont();
    if (fontName == "Poppins")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");
    }
    else if (fontName == "Lekton")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/Lekton-Regular.ttf");
    }
    else if (fontName == "Roboto")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/RobotoSlab-Regular.ttf");
    }
    else if (fontName == "Montserrat")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/Montserrat-Regular.ttf");
    }
    else if (fontName == "Open Sans")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
    }
    else if (fontName == "Oswald")
    {
      id = QFontDatabase::addApplicationFont(":/fonts/Oswald-Regular.ttf");
    }
    else
    {
      id = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
    }

    currentFont.setFamily(fontName);
    currentFont.setPixelSize(baseFontSize);
    currentFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    currentFont.setHintingPreference(QFont::PreferFullHinting);
    currentFont.setStyleStrategy(QFont::PreferAntialias);

    smallButtonFont.setFamily(fontName);
    smallButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    smallButtonFont.setPixelSize(baseSmallButtonSize);
    smallButtonFont.setHintingPreference(QFont::PreferFullHinting);
    smallButtonFont.setStyleStrategy(QFont::PreferAntialias);

    largeButtonFont.setFamily(fontName);
    largeButtonFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    largeButtonFont.setPixelSize(baseLargeButtonSize);
    largeButtonFont.setHintingPreference(QFont::PreferFullHinting);
    largeButtonFont.setStyleStrategy(QFont::PreferAntialias);

    titleFont.setFamily(fontName);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    titleFont.setPixelSize(baseTitleSize);
    titleFont.setHintingPreference(QFont::PreferFullHinting);
    titleFont.setStyleStrategy(QFont::PreferAntialias);

    /* Create our common pool of styles */
    tableStyle = QString(
                     "QHeaderView::section"
                     "{"
                     "font-size: %1 px;"
                     "background-color: #282d31;"
                     "color: #fff;"
                     "font-weight: bold;"
                     "height: 37px;"
                     "border-top: 1px solid #444;"
                     "border-bottom: 1px solid #444;"
                     "}"
                     "QTreeView::item"
                     "{"
                     "color: #ccc;"
                     "height: 37px;"
                     "outline: none;"
                     "}"
                     "QTreeView::item:selected"
                     "{"
                     "background-color: orange;"
                     "color: black;"
                     "}"
                     "QTreeView"
                     "{"
                     "alternate-background-color: #212529;"
                     "background: #282d31"
                     "}")
                     .arg(baseFontSize);
    b1Style = QString(
                  "QPushButton"
                  "{"
                  "font-size: %1 px;"
                  "color:#fff;"
                  "border:1px solid orange;"
                  "border-radius:5px;"
                  "}"
                  "QPushButton:hover"
                  "{"
                  "color:orange;"
                  "}")
                  .arg(baseLargeButtonSize);
    b2Style = QString(
                  "QPushButton"
                  "{"
                  "font-size: %1 px;"
                  "color: orange;"
                  "border:1px solid orange;"
                  "border-radius: 5px"
                  "}"
                  "QPushButton:hover"
                  "{"
                  "color: gold;"
                  "}")
                  .arg(baseSmallButtonSize);
    fontStyle = QString("font-size: %1 px;").arg(baseFontSize);
    darkFontStyle = QString("font-size: %1 px; color: #999;").arg(baseFontSize);
    orangeFontStyle = QString("font-size: %1 px; color: orange;").arg(baseFontSize);

    widgets = getWidgets();
    foreach (QWidget *widget, widgets)
    {
      widget->setFont(currentFont);
    }

    buttons = getButtons();
    foreach (QPushButton *button, buttons)
    {
      /* Set the font and styling for b1 styled buttons */
      if (button->objectName().contains("b1_"))
      {
        button->setStyleSheet(b1Style);
        button->setFont(largeButtonFont);
        button->setMinimumHeight(30);
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
        button->setFont(currentFont);
      }

      /* Set the font and styling for sm styled buttons */
      if (button->objectName().contains("sm_"))
      {
        button->setFont(currentFont);
      }

      /* Set the font and styling for m styled buttons */
      if (button->objectName().contains("m_"))
      {
        button->setFont(currentFont);
      }
    }

    labels = getLabels();
    foreach (QLabel *label, labels)
    {
      if (label->objectName().contains("title_"))
      {
        label->setFont(titleFont);
      }
      else if (label->objectName().contains("o_"))
      {
        label->setStyleSheet(orangeFontStyle);
      }
      else
      {
        label->setFont(currentFont);
      }
    }

    applyStyles();
    return id;
  }

  QList<QWidget *> EditableStyle::getWidgets() { return QList<QWidget *>(); }

  QList<QPushButton *> EditableStyle::getButtons() { return QList<QPushButton *>(); }

  QList<QLabel *> EditableStyle::getLabels() { return QList<QLabel *>(); }

  void EditableStyle::applyStyles() { }
}  // namespace WalletGui
