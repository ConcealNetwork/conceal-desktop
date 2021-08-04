// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDataWidgetMapper>
#include <QFrame>
#include <QModelIndex>
#include <QStyledItemDelegate>

#include "EditableStyle.h"

namespace Ui
{
  class TransactionFrame;
}

namespace WalletGui
{
  class TransactionFrame : public QFrame, EditableStyle
  {
    Q_OBJECT
    Q_DISABLE_COPY(TransactionFrame)

  public:
    TransactionFrame(const QModelIndex &_index, QWidget *_parent);
    ~TransactionFrame();
    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  protected:
    void mousePressEvent(QMouseEvent *_event) Q_DECL_OVERRIDE;

  private:
    QScopedPointer<Ui::TransactionFrame> m_ui;
    QDataWidgetMapper m_dataMapper;
    QPersistentModelIndex m_index;
  };

}  // namespace WalletGui
