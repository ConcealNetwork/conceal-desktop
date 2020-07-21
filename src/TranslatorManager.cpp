// Copyright (c) 2019-2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "TranslatorManager.h"

#include <QApplication>
#include <QLocale>

#include "Settings.h"

using namespace WalletGui;

TranslatorManager* TranslatorManager::m_Instance = 0;

TranslatorManager::TranslatorManager()
{
  m_langPath = ":/translations/%1";

  QString lang = Settings::instance().getLanguage();
  if (lang.isEmpty()) {
    lang = QLocale::system().name();
    lang.truncate(lang.lastIndexOf('_'));
  }

  translator = new QTranslator;
  switchTranslator(lang);
}

TranslatorManager::~TranslatorManager()
{
}

TranslatorManager* TranslatorManager::instance()
{
  static QMutex mutex;
  if (!m_Instance) {
    mutex.lock();

    if (!m_Instance)
      m_Instance = new TranslatorManager;

    mutex.unlock();
  }

  return m_Instance;
}

void TranslatorManager::switchTranslator(const QString& lang)
{
  // remove the old translator
  qApp->removeTranslator(translator);

  // load the new translator
  if (translator->load(m_langPath.arg(lang))) {
    qApp->installTranslator(translator);
    m_keyLang = lang;
  }
  else {
    m_keyLang = "en";
  }
}
