// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "DisclaimerDialog.h"
#include "Settings.h"

#include "ui_disclaimerdialog.h"

namespace WalletGui {

DisclaimerDialog::DisclaimerDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::DisclaimerDialog) {
  m_ui->setupUi(this);
  m_ui->m_aboutLabel->setAttribute(Qt::WA_MacShowFocusRect, 0);
}

DisclaimerDialog::~DisclaimerDialog() {
}
  
}
