// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportSeedDialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#include "Settings.h"
#include "ui_importseeddialog.h"

namespace WalletGui
{
  ImportSeed::ImportSeed(QWidget *_parent) : QDialog(_parent), m_ui(new Ui::ImportSeed)
  {
    m_ui->setupUi(this);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    int startingFontSize = Settings::instance().getFontSize();
    EditableStyle::setStyles(startingFontSize);
  }

  ImportSeed::~ImportSeed() { }

  QString ImportSeed::getKeyString() const { return m_ui->m_seed->text().trimmed(); }

  QString ImportSeed::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportSeed::selectPathClicked()
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

  QList<QWidget *> ImportSeed::getWidgets() { return m_ui->groupBox->findChildren<QWidget *>(); }

  QList<QPushButton *> ImportSeed::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportSeed::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ImportSeed::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
