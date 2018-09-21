// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QClipboard>
#include "AddressListModel.h"
#include "CurrencyAdapter.h"
#include "AddressBookModel.h"
#include "AddressBookFrame.h"
#include "MainWindow.h"
#include "NewAddressDialog.h"
#include "WalletEvents.h"

#include "ui_addressbookframe.h"

namespace WalletGui {

  AddressBookFrame::AddressBookFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::AddressBookFrame), m_addressBookModel(new AddressListModel) {
    m_ui->setupUi(this);
    m_ui->m_addressBookView->setModel(m_addressBookModel.data());
    m_ui->m_addressBookView->setAttribute(Qt::WA_MacShowFocusRect, 0); 
    m_ui->m_addressBookView->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_LABEL, 100);    
    m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_ADDRESS, 750);        
    m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_PAYMENTID, 140);         

    connect(m_ui->m_addressBookView->selectionModel(), &QItemSelectionModel::currentChanged, this, &AddressBookFrame::currentAddressChanged);
  }

  AddressBookFrame::~AddressBookFrame() {
  }

  void AddressBookFrame::addClicked() {
    NewAddressDialog dlg(&MainWindow::instance());
    if (dlg.exec() == QDialog::Accepted) {
      QString label = dlg.getLabel();
      QString address = dlg.getAddress();
      QString paymentId = dlg.getPaymentId();      

      if (address.toStdString().length() == 186) {
        AddressBookModel::instance().addAddress(label, address, "");     
      } else if (!CurrencyAdapter::instance().validateAddress(address)) {
        QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid address"), QtCriticalMsg));
        return;
      } else if (label.trimmed().isEmpty()) {
        QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Empty label"), QtCriticalMsg));
        return;
      } else {
      AddressBookModel::instance().addAddress(label, address, paymentId);
      }
    }
  }

  void AddressBookFrame::copyClicked() {
    QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
  }

  void AddressBookFrame::deleteClicked() {
    int row = m_ui->m_addressBookView->currentIndex().row();
    AddressBookModel::instance().removeAddress(row);
  }

  void AddressBookFrame::currentAddressChanged(const QModelIndex& _index) {
    m_ui->m_copyAddressButton->setEnabled(_index.isValid());
    m_ui->m_deleteAddressButton->setEnabled(_index.isValid());
    m_ui->m_copyAddressButton->setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_ui->m_deleteAddressButton->setAttribute(Qt::WA_MacShowFocusRect, 0);    
  }

void AddressBookFrame::backClicked() {
  Q_EMIT backSignal();
}

  void AddressBookFrame::addressDoubleClicked(const QModelIndex& _index) {
    if (!_index.isValid()) {
      return;
    }

    Q_EMIT payToSignal(_index);
  }

}
