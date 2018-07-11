// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//  
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QWidget>

class QMovie;

namespace Ui {
class ExitWidget;
}

namespace WalletGui {

class ExitWidget : public QWidget {
  Q_OBJECT
  Q_DISABLE_COPY(ExitWidget)

public:
  ExitWidget(QWidget* _parent);
  ~ExitWidget();

private:
  QScopedPointer<Ui::ExitWidget> m_ui;
  QMovie* m_clockMovie;
};

}
