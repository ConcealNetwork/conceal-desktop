// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui 
{

  class ShowQRCode;
}

namespace WalletGui 
{

  class ShowQRCode : public QDialog 
  {

    Q_OBJECT

    public:
      ShowQRCode(QWidget* _parent);
      ~ShowQRCode();
      
    void showQR(const QString& _address);

    private:
      QScopedPointer<Ui::ShowQRCode> m_ui;

    void updateWalletAddress(const QString& _address);


  };
}
