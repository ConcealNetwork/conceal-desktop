// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

#include "EditableStyle.h"

namespace Ui {
class ChangePasswordDialog;
}

namespace WalletGui {

class ChangePasswordDialog : public QDialog, EditableStyle {
  Q_OBJECT
  Q_DISABLE_COPY(ChangePasswordDialog)

public:
  explicit ChangePasswordDialog(QWidget* _parent);
  ~ChangePasswordDialog();
  QString getNewPassword() const;
  QString getOldPassword() const;

protected:
  QList<QWidget *> getWidgets() override;
  QList<QPushButton *> getButtons() override;
  QList<QLabel *> getLabels() override;
  void applyStyles() override;

private:
  QScopedPointer<Ui::ChangePasswordDialog> m_ui;

  Q_SLOT void checkPassword(const QString& _password);
};

}
