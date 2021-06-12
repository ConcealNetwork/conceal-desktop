// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportSecretKeys.h"

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#include "Settings.h"
#include "ui_importsecretkeys.h"

namespace WalletGui
{
  ImportSecretKeys::ImportSecretKeys(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::ImportSecretKeys)
  {
    m_ui->setupUi(this);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    int startingFontSize = Settings::instance().getFontSize();
    EditableStyle::setStyles(startingFontSize);
  }

  ImportSecretKeys::~ImportSecretKeys() { }

  QString ImportSecretKeys::getSpendKeyString() const { return m_ui->m_spendKey->text().trimmed(); }

  QString ImportSecretKeys::getViewKeyString() const { return m_ui->m_viewKey->text().trimmed(); }

  QString ImportSecretKeys::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportSecretKeys::selectPathClicked()
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

  QList<QWidget *> ImportSecretKeys::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportSecretKeys::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportSecretKeys::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ImportSecretKeys::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui