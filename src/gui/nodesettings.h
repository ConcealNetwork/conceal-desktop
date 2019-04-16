// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui 
{

  class NodeSettings;
}

namespace WalletGui {
  class NodeSettings : public QDialog {

    Q_OBJECT

    public:
      NodeSettings(QWidget* _parent);
      ~NodeSettings();

      QString setConnectionMode() const;
      QString setRemoteHost() const;
      void initConnectionSettings();
    
    private:
      QScopedPointer<Ui::NodeSettings> m_ui;
  };
}
