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
  class ImportSecretKeysDialog;
}

namespace WalletGui
{
  class ImportSecretKeysDialog : public QDialog, public EditableStyle
  {
    Q_OBJECT

  public:
    explicit ImportSecretKeysDialog(QWidget *_parent);
    ~ImportSecretKeysDialog();
    QString getSpendKeyString() const;
    QString getViewKeyString() const;
    QString getFilePath() const;
    void setErrorMessage(QString message);
    void clearErrorMessage();

    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::ImportSecretKeysDialog> m_ui;
    Q_SLOT void selectPathClicked();
    Q_SLOT void importButtonClicked();
  };
}  // namespace WalletGui
