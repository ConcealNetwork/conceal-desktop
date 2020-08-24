// Copyright (c) 2019-2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRANSLATORMANAGER_H
#define TRANSLATORMANAGER_H

#include <QMutex>
#include <QTranslator>

class TranslatorManager {
public:
  static TranslatorManager* instance();
  ~TranslatorManager();

  void switchTranslator(const QString& filename);
  inline QString getCurrentLang()
  {
    return m_keyLang;
  }

private:
  TranslatorManager();

  // Hide copy constructor and assignment operator.
  TranslatorManager(const TranslatorManager&);
  TranslatorManager& operator=(const TranslatorManager&);

  // Class instance.
  static TranslatorManager* m_Instance;

  QString m_keyLang;
  QString m_langPath = ":/translations/%1";       // path of translations for conceal-desktop app
  QString m_qtLangPath = ":/translations/qt/%1";  // path of translations for qt components
  QTranslator* translator;                        // translator for conceal-desktop app
  QTranslator* qtTranslator;                      // translator for qt components
};

#endif  // TRANSLATORMANAGER_H
