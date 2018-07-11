// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QLabel>

namespace WalletGui {

class QRLabel : public QLabel {
  Q_OBJECT

public:
  QRLabel(QWidget* _parent);
  ~QRLabel();

  void showQRCode(const QString& _dataString);
};

}
