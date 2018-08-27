// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>

#include "importtracking.h"
#include "ui_importtracking.h"

namespace WalletGui 
{

  ImportTracking::ImportTracking(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ImportTracking) 
  {

    m_ui->setupUi(this);
  }

  ImportTracking::~ImportTracking() 
  {
  }

  QString ImportTracking::getKeyString() const 
  {

    return m_ui->m_trackingKey->text().trimmed();
  }

  QString ImportTracking::getFilePath() const 
  {

    return m_ui->m_pathEdit->text().trimmed();
  }

  void ImportTracking::selectPathClicked() 
  {

    QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"),
  #ifdef Q_OS_WIN
      QApplication::applicationDirPath(),
  #else
      QDir::homePath(),
  #endif
      tr("Wallets (*.wallet)")
      );

    if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) 
    {
      
      filePath.append(".wallet");
    }

    m_ui->m_pathEdit->setText(filePath);
  }

}
