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
  class MainPasswordDialog;
}

namespace WalletGui
{
  class MainPasswordDialog : public QDialog, EditableStyle
  {
    Q_OBJECT
    Q_DISABLE_COPY(MainPasswordDialog)

  public:
    MainPasswordDialog(bool _error, QWidget *_parent);
    ~MainPasswordDialog();
    QString getPassword() const;
    Q_SIGNAL void changeSignal();

  protected:
    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::MainPasswordDialog> m_ui;

    Q_SLOT void helpClicked();
    Q_SLOT void quitClicked();
    Q_SLOT void changeClicked();
  };

}  // namespace WalletGui
