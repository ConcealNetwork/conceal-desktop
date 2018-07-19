// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>
#include "importsecretkeys.h"
#include "ui_importsecretkeys.h"

namespace WalletGui {

importSecretKeys::importSecretKeys(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::importSecretKeys) {
  m_ui->setupUi(this);
}

importSecretKeys::~importSecretKeys() {
}

QString importSecretKeys::getSpendKeyString() const {
  return m_ui->m_spendKey->text().trimmed();
}

QString importSecretKeys::getViewKeyString() const {
  return m_ui->m_viewKey->text().trimmed();
}

QString importSecretKeys::getFilePath() const {
  return m_ui->m_pathEdit->text().trimmed();
}

void importSecretKeys::selectPathClicked() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"),
#ifdef Q_OS_WIN
    QApplication::applicationDirPath(),
#else
    QDir::homePath(),
#endif
    tr("Wallets (*.wallet)")
    );

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  m_ui->m_pathEdit->setText(filePath);
}

}

void WalletGui::importSecretKeys::on_m_selectPathButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"),
  #ifdef Q_OS_WIN
      QApplication::applicationDirPath(),
  #else
      QDir::homePath(),
  #endif
      tr("Wallets (*.wallet)")
      );

    if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
      filePath.append(".wallet");
    }

    m_ui->m_pathEdit->setText(filePath);
}
