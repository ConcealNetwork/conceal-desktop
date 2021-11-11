// Copyright (c) 2011-2015 The Cryptonote developers
// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ContactDialog.h"

#include <QRegExpValidator>

#include "AddressBookModel.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"
#include "ui_contactdialog.h"

namespace WalletGui
{
  ContactDialog::ContactDialog(QWidget *_parent) : QDialog(_parent), m_ui(new Ui::ContactDialog)
  {
    m_ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint);
    setTitle(tr("New Contact"));
    if (_parent != nullptr)
    {
      move((_parent->width() - width()) / 2, (_parent->height() - height()) / 2);
    }
    QRegExp hexMatcher("^[0-9A-F]{64}$", Qt::CaseInsensitive);
    QValidator *validator = new QRegExpValidator(hexMatcher, this);
    m_ui->m_contactPaymentIdEdit->setValidator(validator);
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ContactDialog::~ContactDialog() { }

  QString ContactDialog::getAddress() const { return m_ui->m_addressEdit->text(); }

  QString ContactDialog::getPaymentID() const { return m_ui->m_contactPaymentIdEdit->text(); }

  QString ContactDialog::getLabel() const { return m_ui->m_labelEdit->text(); }

  void ContactDialog::setEditLabel(QString label) { m_ui->m_labelEdit->setText(label); }

  void ContactDialog::setEditAddress(QString address) { m_ui->m_addressEdit->setText(address); }

  void ContactDialog::setEditPaymentId(QString paymentid)
  {
    m_ui->m_contactPaymentIdEdit->setText(paymentid);
  }

  void ContactDialog::setTitle(QString title)
  {
    setWindowTitle(title);
    m_ui->dTitle_contactDialog->setText(title);
  }

  void ContactDialog::edit(QModelIndex index)
  {
    isEdit = true;
    editIndex = index.row();
    setTitle(tr("Edit Contact"));
    setEditLabel(index.data(AddressBookModel::ROLE_LABEL).toString());
    setEditAddress(index.data(AddressBookModel::ROLE_ADDRESS).toString());
    setEditPaymentId(index.data(AddressBookModel::ROLE_PAYMENTID).toString());
  }

  void ContactDialog::setErrorMessage(QString message) { m_ui->m_errorLabel->setText(message); }

  void ContactDialog::clearErrorMessage() { m_ui->m_errorLabel->setText(""); }

  QList<QWidget *> ContactDialog::getWidgets() { return m_ui->groupBox->findChildren<QWidget *>(); }

  QList<QPushButton *> ContactDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ContactDialog::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ContactDialog::applyStyles() { m_ui->groupBox->update(); }

  void ContactDialog::okButtonClicked()
  {
    QString label = getLabel();
    QString address = getAddress();
    QByteArray paymentId = getPaymentID().toUtf8();
    clearErrorMessage();
    if (!CurrencyAdapter::instance().validateAddress(address))
    {
      setErrorMessage(tr("Invalid address"));
      return;
    }

    if (!WalletAdapter::isValidPaymentId(paymentId))
    {
      setErrorMessage(tr("Invalid payment ID"));
      return;
    }

    QModelIndex contactIndex =
        AddressBookModel::instance().indexFromContact(label, AddressBookModel::COLUMN_LABEL);
    QString contactLabel = contactIndex.data(AddressBookModel::ROLE_LABEL).toString();
    if (label == contactLabel && (!isEdit || (isEdit && editIndex != contactIndex.row())))
    {
      setErrorMessage(tr("Contact with such label already exists."));
      return;
    }
    AddressBookModel::instance().addAddress(label, address, paymentId);
    accept();
  }

}  // namespace WalletGui
