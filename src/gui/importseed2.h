// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {
class ImportSeed2;
}

namespace WalletGui {

class ImportSeed2 : public QDialog {
  Q_OBJECT

public:
  ImportSeed2(QWidget* _parent);
  ~ImportSeed2();

  QString setConnectionMode() const;
  void initConnectionSettings();


private:
  QScopedPointer<Ui::ImportSeed2> m_ui;


};

}
