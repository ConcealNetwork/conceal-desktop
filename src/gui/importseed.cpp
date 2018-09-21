// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>

#include "importseed.h"
#include "ui_importseed.h"

namespace WalletGui {

ImportSeed::ImportSeed(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ImportSeed) {
  m_ui->setupUi(this);


}

ImportSeed::~ImportSeed() {
}

QString ImportSeed::getKeyString() const {
  return m_ui->m_seed->text().trimmed();
}

QString ImportSeed::getFilePath() const {
  return m_ui->m_pathEdit->text().trimmed();
}

void ImportSeed::selectPathClicked() {
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
