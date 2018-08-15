// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionconfirmation.h"
#include "ui_transactionconfirmation.h"

namespace WalletGui 
{

    transactionconfirmation::transactionconfirmation(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::transactionconfirmation) {
        m_ui->setupUi(this);  
    }

    transactionconfirmation::~transactionconfirmation()
    {
    }

}