// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2022 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>
#include <QModelIndex>

#include "EditableStyle.h"

namespace Ui
{
  class ContactDialog;
}

namespace WalletGui
{
  class ContactDialog : public QDialog, EditableStyle
  {
    Q_OBJECT
    Q_DISABLE_COPY(ContactDialog)

  public:
    explicit ContactDialog(QWidget *_parent);
    ~ContactDialog() override;

    QString getAddress() const;
    QString getLabel() const;
    QString getPaymentID() const;

    void setEditLabel(QString label);
    void setEditAddress(QString address);
    void setEditPaymentId(QString paymentId);
    void setTitle(QString title);
    void edit(QModelIndex index);
    void setErrorMessage(QString message);
    void clearErrorMessage();

    QList<QWidget *> getWidgets() override;
    QList<QPushButton *> getButtons() override;
    QList<QLabel *> getLabels() override;
    void applyStyles() override;

  private:
    QScopedPointer<Ui::ContactDialog> m_ui;
    bool isEdit = false;
    int editIndex = 0;
    Q_SLOT void okButtonClicked();
  };

}  // namespace WalletGui
