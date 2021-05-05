// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDataWidgetMapper>
#include <QDialog>

#include "EditableStyle.h"

namespace Ui
{
  class MessageDetailsDialog;
}

namespace WalletGui
{
  class MessageDetailsDialog : public QDialog, public EditableStyle
  {
    Q_OBJECT

  public:
    MessageDetailsDialog(const QModelIndex &_index, QWidget *_parent);
    ~MessageDetailsDialog();

    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

    QModelIndex getCurrentMessageIndex() const;

  private:
    QScopedPointer<Ui::MessageDetailsDialog> m_ui;
    QDataWidgetMapper m_dataMapper;

    Q_SLOT void prevClicked();
    Q_SLOT void nextClicked();
    Q_SLOT void saveClicked();
  };

}  // namespace WalletGui
