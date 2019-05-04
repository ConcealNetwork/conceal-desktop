// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui {  
    class transactionconfirmation;
}

namespace WalletGui {
    class transactionconfirmation : public QDialog {  
        Q_OBJECT

        public:
            transactionconfirmation(QWidget* _parent);
            ~transactionconfirmation();
            void setMessage(QString optimizationMessage);

        private:
            QScopedPointer<Ui::transactionconfirmation> m_ui;   
    };

}
