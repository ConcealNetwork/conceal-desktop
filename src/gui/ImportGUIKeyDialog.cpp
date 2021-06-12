// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "ImportGUIKeyDialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#include "Settings.h"
#include "ui_importguikeydialog.h"

namespace WalletGui
{
  ImportGUIKeyDialog::ImportGUIKeyDialog(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::ImportGUIKeyDialog)
  {
    m_ui->setupUi(this);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    int startingFontSize = Settings::instance().getFontSize();
    EditableStyle::setStyles(startingFontSize);
  }

  ImportGUIKeyDialog::~ImportGUIKeyDialog() { }

  QString ImportGUIKeyDialog::getKeyString() const { return m_ui->m_keyEdit->text().trimmed(); }

  QString ImportGUIKeyDialog::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportGUIKeyDialog::selectPathClicked()
  {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"),
                                                    Settings::instance().getDefaultWalletPath(),
                                                    tr("Wallets (*.wallet)"));
    if (!filePath.isEmpty() && !filePath.endsWith(".wallet"))
    {
      filePath.append(".wallet");
    }
    m_ui->m_pathEdit->setText(filePath);
  }

  QList<QWidget *> ImportGUIKeyDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportGUIKeyDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportGUIKeyDialog::getLabels()
  {
    return m_ui->groupBox->findChildren<QLabel *>();
  }

  void ImportGUIKeyDialog::applyStyles() { m_ui->groupBox->update(); }

}  // namespace WalletGui
