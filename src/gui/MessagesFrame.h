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
class MessagesFrame;
}

namespace WalletGui {

class VisibleMessagesModel;

class MessagesFrame : public QFrame {
  Q_OBJECT

public:
  explicit MessagesFrame(QWidget* _parent);
  ~MessagesFrame();
  int test;

private:
  QScopedPointer<Ui::MessagesFrame> m_ui;
  QScopedPointer<VisibleMessagesModel> m_visibleMessagesModel;

  void currentMessageChanged(const QModelIndex& _currentIndex);

  Q_SLOT void messageDoubleClicked(const QModelIndex& _index);
  Q_SLOT void replyClicked();
  Q_SLOT void backClicked();
  Q_SLOT void newMessageClicked();

Q_SIGNALS:
  void backSignal();
  void newMessageSignal();
  void replyToSignal(const QModelIndex& _index);
};

}
