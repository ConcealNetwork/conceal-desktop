// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IMPORTSECRETKEYS_H
#define IMPORTSECRETKEYS_H

#include <QDialog>

namespace Ui {
class importSecretKeys;
}

namespace WalletGui {

class importSecretKeys : public QDialog {
    Q_OBJECT

public:
    importSecretKeys(QWidget* _parent);
    ~importSecretKeys();

    QString getSpendKeyString() const;
    QString getViewKeyString() const;    
    QString getFilePath() const;

private slots:
    void on_m_selectPathButton_clicked();

private:
    QScopedPointer<Ui::importSecretKeys> m_ui;
    Q_SLOT void selectPathClicked();    
};

}

#endif // IMPORTSECRETKEYS_H
