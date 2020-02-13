// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
class ReceiveFrame;
}

namespace WalletGui {

class ReceiveFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(ReceiveFrame)

public:
  ReceiveFrame(QWidget* _parent);
  ~ReceiveFrame();

private:
  QScopedPointer<Ui::ReceiveFrame> m_ui;

  void updateWalletAddress(const QString& _address);
  void walletOpened(int _error);
  void walletClosed();

  Q_SLOT void backClicked(); 
  Q_SLOT void copyGUIClicked(); 
  Q_SLOT void copySpendKeyClicked();
  Q_SLOT void copyViewKeyClicked();
  Q_SLOT void copySeedClicked(); 
  Q_SLOT void backupClicked();
  Q_SLOT void showSeed();
  Q_SLOT void showGUI();  
  Q_SLOT void showPrivate();  
  Q_SLOT void back2Clicked();    

Q_SIGNALS:
  void backSignal();
  void backupSignal();
};

}
