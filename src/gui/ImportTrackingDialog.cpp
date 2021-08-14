// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportTrackingDialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#include "Settings.h"
#include "ui_importtrackingdialog.h"

namespace WalletGui
{
  ImportTrackingDialog::ImportTrackingDialog(QWidget *_parent) : QDialog(_parent), m_ui(new Ui::ImportTrackingDialog)
  {
    m_ui->setupUi(this);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ImportTrackingDialog::~ImportTrackingDialog() { }

  QString ImportTrackingDialog::getKeyString() const { return m_ui->m_trackingKey->text().trimmed(); }

  QString ImportTrackingDialog::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportTrackingDialog::selectPathClicked()
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

  QList<QWidget *> ImportTrackingDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportTrackingDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportTrackingDialog::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ImportTrackingDialog::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
