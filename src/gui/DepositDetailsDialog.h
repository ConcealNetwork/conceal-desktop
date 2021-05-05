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
  class DepositDetailsDialog;
}

namespace WalletGui
{
  class DepositDetailsDialog : public QDialog, public EditableStyle
  {
    Q_OBJECT

  public:
    DepositDetailsDialog(const QModelIndex &_index, QWidget *_parent);
    ~DepositDetailsDialog();

    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::DepositDetailsDialog> m_ui;
    const QString m_detailsTemplate;
  };

}  // namespace WalletGui
