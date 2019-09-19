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

    /* Get current language */
    QString language = Settings::instance().getLanguage();
    if (language.compare("tr") == 0)
    {
        m_ui->m_turkish->setChecked(true);
    }
    if (language.compare("ru") == 0)
    {
        m_ui->m_russian->setChecked(true);
    }
    else
    {
        m_ui->m_english->setChecked(true);
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

    /* Set current connection options */
    QString connection = Settings::instance().getConnection();
    QString remoteHost = Settings::instance().getCurrentRemoteNode();
    m_ui->m_hostEdit->setText(remoteHost);

    /* If the connection is a remote node, then load the current (or default)
      remote node into the text field. */
    if (connection.compare("remote") == 0)
    {
        m_ui->radioButton->setChecked(true);
    }

    if (connection.compare("autoremote") == 0)
    {
        m_ui->radioButton_3->setChecked(true);
    }
    /* It is an embedded node, so let only check that */
    else if (connection.compare("embedded") == 0)
    {
        m_ui->radioButton_2->setChecked(true);
    }
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

void SettingsFrame::saveLanguageClicked()
{
    QString language;
    if (m_ui->m_russian->isChecked())
    {
        language = "ru";
    }
    else if (m_ui->m_turkish->isChecked())
    {
        language = "tr";
    }
    else
    {
        language = "en";
    }
    Settings::instance().setLanguage(language);
}

void SettingsFrame::saveConnectionClicked()
{
    QString connectionMode;
    if (m_ui->radioButton->isChecked())
    {
        connectionMode = "remote";
    }
    else if (m_ui->radioButton_2->isChecked())
    {
        connectionMode = "embedded";
    }
    else if (m_ui->radioButton_3->isChecked())
    {
        connectionMode = "autoremote";
    }
    Settings::instance().setConnection(connectionMode);

    QString remoteHost;
    /* If it is a remote connection, commit the entered remote node. There is no validation of the 
     remote node. If the connection is embedded then take no action */
    if (m_ui->radioButton->isChecked())
    {
        remoteHost = m_ui->m_hostEdit->text();
    }
    if (m_ui->radioButton_3->isChecked())
    {
        remoteHost = m_ui->m_hostEdit->text();
    }
    Settings::instance().setCurrentRemoteNode(remoteHost);
}

void SettingsFrame::minToTrayClicked()
{
    if (!Settings::instance().isMinimizeToTrayEnabled())
    {
        Settings::instance().setMinimizeToTrayEnabled(true);
        m_ui->m_minToTrayButton->setText(tr("DISABLE"));
    }
    else
    {
        Settings::instance().setMinimizeToTrayEnabled(false);
        m_ui->m_minToTrayButton->setText(tr("ENABLE"));
    }
}

void SettingsFrame::closeToTrayClicked()
{
    if (!Settings::instance().isCloseToTrayEnabled())
    {
        Settings::instance().setCloseToTrayEnabled(true);
        m_ui->m_closeToTrayButton->setText(tr("DISABLE"));
    }
    else
    {
        Settings::instance().setCloseToTrayEnabled(false);
        m_ui->m_closeToTrayButton->setText(tr("ENABLE"));
    }
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
