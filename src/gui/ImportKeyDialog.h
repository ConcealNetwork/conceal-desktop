// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>

namespace Ui 
{
  class ImportKeyDialog;
}

namespace WalletGui 
{

  class ImportKeyDialog : public QDialog 
  {
    Q_OBJECT

    public:
      ImportKeyDialog(QWidget* _parent);
      ~ImportKeyDialog();
      QString getKeyString() const;
      QString getFilePath() const;

    private:
      QScopedPointer<Ui::ImportKeyDialog> m_ui;
      Q_SLOT void selectPathClicked();
  };
}
