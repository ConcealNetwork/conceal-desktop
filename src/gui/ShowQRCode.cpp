// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QFileDialog>
#include <Common/Base58.h>
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"

#include "ShowQRCode.h"
#include "QRLabel.h"

#include "ui_showqrcode.h"

namespace WalletGui {

    ShowQRCode::ShowQRCode(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ShowQRCode) 
    {
        m_ui->setupUi(this);
        connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &ShowQRCode::updateWalletAddress);

    }

    ShowQRCode::~ShowQRCode() 
    {

    }

    void ShowQRCode::updateWalletAddress(const QString& _address) 
    {

        m_ui->m_qrLabel->showQRCode(_address);
    }

    void ShowQRCode::showQR(const QString& _address) 
    {

        m_ui->m_qrLabel->showQRCode(_address);
    }

}
