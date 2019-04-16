// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui 
{

class ImportTracking;
}

namespace WalletGui 
{

  class ImportTracking : public QDialog 
  {
      
      Q_OBJECT

    public:
      ImportTracking(QWidget* _parent);
      ~ImportTracking();

      QString getKeyString() const;
      QString getFilePath() const;

    private:
      QScopedPointer<Ui::ImportTracking> m_ui;

      Q_SLOT void selectPathClicked();
  };

}
