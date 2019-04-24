// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>
#include "NodeSettings.h"
#include "CurrencyAdapter.h"
#include "NewNodeDialog.h"
#include "MainWindow.h"
#include "NodeModel.h"
#include "Settings.h"
#include "ui_nodesettings.h"

namespace WalletGui {

NodeSettings::NodeSettings(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::NodeSettings) 
{
  m_ui->setupUi(this);
}

NodeSettings::~NodeSettings() {
}

/* Initialize the existing connection values */
void NodeSettings::initConnectionSettings() 
{
  QString connection = Settings::instance().getConnection();
  QString remoteHost = Settings::instance().getCurrentRemoteNode();
  m_ui->m_hostEdit->setText(remoteHost);
  
  /* If the connection is a remote node, then load the current (or default)
      remote node into the text field. */
  if(connection.compare("remote") == 0) 
  {
      m_ui->radioButton->setChecked(true);
  }

  if(connection.compare("autoremote") == 0) 
  {
      m_ui->radioButton_3->setChecked(true);
  }
  /* It is an embedded node, so let only check that */
  else if(connection.compare("embedded") == 0) 
  {
      m_ui->radioButton_2->setChecked(true);
  }
}

/* Save the connection settings */
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
  else if(m_ui->radioButton_3->isChecked()) 
  {        
    connectionMode = "autoremote";
  } 
  return connectionMode;
}

/* Save remote node host */
QString NodeSettings::setRemoteHost() const 
{
  QString remoteHost;
  /* If it is a remote connection, commit the entered remote node. There is no validation of the 
     remote node. If the connection is embedded then take no action */    
  if(m_ui->radioButton->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  if(m_ui->radioButton_3->isChecked())
  {
    remoteHost = m_ui->m_hostEdit->text();
  }
  return remoteHost;
}

}



