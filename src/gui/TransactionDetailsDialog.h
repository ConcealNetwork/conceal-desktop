// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

#include "EditableStyle.h"

namespace Ui
{
  class TransactionDetailsDialog;
}

namespace WalletGui
{
  class TransactionDetailsDialog : public QDialog, public EditableStyle
  {
    Q_OBJECT
    Q_DISABLE_COPY(TransactionDetailsDialog)

  public:
    TransactionDetailsDialog(const QModelIndex &_index, QWidget *_parent);
    ~TransactionDetailsDialog();

    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::TransactionDetailsDialog> m_ui;
    const QString m_detailsTemplate;
  };

}  // namespace WalletGui
