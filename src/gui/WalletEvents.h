// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QEvent>

namespace WalletGui
{
    enum class WalletEventType : quint32 {
        ShowMessage = QEvent::User
    };

    class ShowMessageEvent : public QEvent {

    public:
        ShowMessageEvent(const QString &_messageText, QtMsgType _messageType) : QEvent(static_cast<QEvent::Type>(WalletEventType::ShowMessage)),
            m_messageText(_messageText), m_messageType(_messageType) {
        }

        QString messageText() const {
          return m_messageText;
        }

        QtMsgType messageType() const {
          return m_messageType;
        }

    private:
        QString m_messageText;
        QtMsgType m_messageType;
    };

}
