// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>

#include "Settings.h"
#include "nodesettings.h"
#include "ui_nodesettings.h"

namespace WalletGui {

  NodeSettings::NodeSettings(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::NodeSettings) 
  {

    m_ui->setupUi(this);
  }

  NodeSettings::~NodeSettings() {
  }

  void NodeSettings::initConnectionSettings() 
  {

    QString connection = Settings::instance().getConnection();

    if(connection.compare("remote") == 0) 
    {
        m_ui->radioButton->setChecked(true);
    }
    else if(connection.compare("embedded") == 0) 
    {

        m_ui->radioButton_2->setChecked(true);
    }
  }

  QString NodeSettings::setConnectionMode() const 
  {

    QString connectionMode;
    
    if(m_ui->radioButton->isChecked())
    {

        connectionMode = "remote";
    }

    else if(m_ui->radioButton_2->isChecked())
    {
        
        connectionMode = "embedded";
    }  

    return connectionMode;
  }
}



