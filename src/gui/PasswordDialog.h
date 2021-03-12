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
  class PasswordDialog;
}

namespace WalletGui
{
  class PasswordDialog : public QDialog, EditableStyle
  {
    Q_OBJECT
    Q_DISABLE_COPY(PasswordDialog)

  public:
    PasswordDialog(bool _error, QWidget *_parent);
    ~PasswordDialog();
    // int setStyles(int change) override;
    QString getPassword() const;

  protected:
    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::PasswordDialog> m_ui;
  };

}  // namespace WalletGui
