// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include "LanguageSettings.h"
#include "MainWindow.h"
#include "Settings.h"
#include "ui_languagesettings.h"

namespace WalletGui {

LanguageSettings::LanguageSettings(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::LanguageSettings) 
{
  m_ui->setupUi(this);
}

LanguageSettings::~LanguageSettings() {
}

/* Initialize the existing language */
void LanguageSettings::initLanguageSettings() 
{
  QString language = Settings::instance().getLanguage();
  if(language.compare("tr") == 0) 
  {
      m_ui->m_turkish->setChecked(true);
  }
  if(language.compare("ru") == 0) 
  {
      m_ui->m_russian->setChecked(true);
  }
  if(language.compare("cn") == 0) 
  {
      m_ui->m_chinese->setChecked(true);
  }  
  else 
  {
      m_ui->m_english->setChecked(true);
  }
}

/* Save the language settings */
QString LanguageSettings::setLanguage() const 
{
  QString language;
  if(m_ui->m_russian->isChecked())
  {
    language = "ru";
  }
  else if(m_ui->m_turkish->isChecked()) 
  {        
    language = "tr";
  } 
  else if(m_ui->m_chinese->isChecked()) 
  {        
    language = "cn";
  }   
  else 
  {        
    language = "en";
  } 
  return language;
}

}



