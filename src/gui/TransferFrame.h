// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

namespace Ui {
class TransferFrame;
}

namespace WalletGui {

class AliasProvider;

class TransferFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(TransferFrame)

public:
  TransferFrame(QWidget* _parent);
  ~TransferFrame();

  QString getAddress() const;
  QString getLabel() const;
  qreal getAmount() const;
  QString getAmountString() const;
  QString getComment() const;

  void disableRemoveButton(bool _disable);
  void setAddress(const QString& _address);

protected:
  void timerEvent(QTimerEvent* _event) Q_DECL_OVERRIDE;

private:
  QScopedPointer<Ui::TransferFrame> m_ui;
  AliasProvider* m_aliasProvider;
  int m_addressInputTimer;

  void onAliasFound(const QString& _name, const QString& _address);
  Q_SLOT void addressBookClicked();
  Q_SLOT void addressEdited(const QString& _text);
  Q_SLOT void commentEdited(const QString& _text);
  Q_SLOT void pasteClicked();

Q_SIGNALS:
  void commentEditedSignal();
};

}
