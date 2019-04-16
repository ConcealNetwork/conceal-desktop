// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QFont>
#include <QFontDatabase>

#include "MessagesFrame.h"
#include "MainWindow.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"
#include "VisibleMessagesModel.h"

#include "ui_messagesframe.h"

namespace WalletGui {

MessagesFrame::MessagesFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::MessagesFrame),
  m_visibleMessagesModel(new VisibleMessagesModel) {
  m_ui->setupUi(this);
  m_ui->m_messagesView->setModel(m_visibleMessagesModel.data());
  m_ui->m_messagesView->header()->resizeSection(MessagesModel::COLUMN_DATE, 140);

  int id2 = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
  QFont font2;
  font2.setFamily("Lato");
  font2.setPixelSize(15);
  m_ui->m_messagesView->setFont(font2);

}

MessagesFrame::~MessagesFrame() {
}

void MessagesFrame::messageDoubleClicked(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  MessageDetailsDialog dlg(_index, &MainWindow::instance());
  if (dlg.exec() == QDialog::Accepted) {
    Q_EMIT replyToSignal(dlg.getCurrentMessageIndex());
  }
}

void MessagesFrame::replyClicked() {
  Q_EMIT replyToSignal(m_ui->m_messagesView->selectionModel()->currentIndex());
}

void MessagesFrame::backClicked() {
  Q_EMIT backSignal();
}

void MessagesFrame::newMessageClicked() 
{

  Q_EMIT newMessageSignal();
}

}
