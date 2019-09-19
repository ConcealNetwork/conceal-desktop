// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QTime>

#include "MainWindow.h"
#include "SettingsFrame.h"
#include "WalletAdapter.h"
#include "Settings.h"

#include "ui_settingsframe.h"

namespace WalletGui
{

SettingsFrame::SettingsFrame(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::SettingsFrame)
{
    m_ui->setupUi(this);
    m_ui->m_optimizationMessage->setText("");

    /* Show optimization recommendation */
    quint64 numUnlockedOutputs;
    numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();

    if (numUnlockedOutputs >= 100)
    {
        setMessage("Optimization recommended [" + QString::number(numUnlockedOutputs) + "]");
    }
    else
    {
        setMessage("Optimization not required [" + QString::number(numUnlockedOutputs) + "]");
    }

    m_ui->m_minToTrayButton->setText(tr("ENABLE"));
    m_ui->m_closeToTrayButton->setText(tr("ENABLE"));

#ifdef Q_OS_WIN
    /* Set minimize to tray button status */
    if (!Settings::instance().isMinimizeToTrayEnabled())
    {
        m_ui->m_minToTrayButton->setText(tr("ENABLE"));
    }
    else
    {
        m_ui->m_minToTrayButton->setText(tr("DISABLE"));
    }

    /* Set close to tray button status */
    if (!Settings::instance().isCloseToTrayEnabled())
    {
        m_ui->m_closeToTrayButton->setText(tr("ENABLE"));
    }
    else
    {
        m_ui->m_closeToTrayButton->setText(tr("DISABLE"));
    }
#endif
}

SettingsFrame::~SettingsFrame()
{
}

void SettingsFrame::setMessage(QString optimizationMessage)
{
    m_ui->m_optimizationMessage->setText(optimizationMessage);
}

void SettingsFrame::optimizeClicked()
{
    quint64 numUnlockedOutputs;
    numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();

    WalletAdapter::instance().optimizeWallet();
    while (WalletAdapter::instance().getNumUnlockedOutputs() > 100)
    {
        numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
        if (numUnlockedOutputs == 0)
            break;
        WalletAdapter::instance().optimizeWallet();
        delay();
    }
    backClicked();
}

void SettingsFrame::delay()
{
    QTime dieTime = QTime::currentTime().addSecs(2);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void SettingsFrame::backClicked()
{
    Q_EMIT backSignal();
}

void SettingsFrame::rescanClicked()
{
    Q_EMIT rescanSignal();
}

} // namespace WalletGui
