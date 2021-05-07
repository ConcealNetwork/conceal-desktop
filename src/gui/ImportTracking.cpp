// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportTracking.h"

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#include "Settings.h"
#include "ui_importtracking.h"

namespace WalletGui
{
  ImportTracking::ImportTracking(QWidget *_parent) : QDialog(_parent), m_ui(new Ui::ImportTracking)
  {
    m_ui->setupUi(this);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ImportTracking::~ImportTracking() { }

  QString ImportTracking::getKeyString() const { return m_ui->m_trackingKey->text().trimmed(); }

  QString ImportTracking::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportTracking::selectPathClicked()
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

  QList<QWidget *> ImportTracking::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportTracking::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportTracking::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ImportTracking::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
