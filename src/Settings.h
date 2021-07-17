// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
//
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDir>
#include <QJsonObject>
#include <QObject>

namespace WalletGui
{
  class CommandLineParser;

  class Settings : public QObject
  {
    Q_OBJECT
    Q_DISABLE_COPY(Settings)

  public:
    static Settings &instance();

    void setCommandLineParser(CommandLineParser *_cmd_line_parser);
    void load();
    void setOptions();

    bool hasAllowLocalIpOption() const;
    bool hasHideMyPortOption() const;
    bool isTestnet() const;
    QDir getDataDir() const;
    QString getP2pBindIp() const;
    quint16 getLocalRpcPort() const;
    quint16 getP2pBindPort() const;
    quint16 getP2pExternalPort() const;
    QStringList getExclusiveNodes() const;
    QStringList getPeers() const;
    quint16 getCurrentLocalDaemonPort() const;
    QStringList getPriorityNodes() const;
    QStringList getSeedNodes() const;
    QStringList getRpcNodesList() const;
    QString getCurrentRemoteNode() const;
    QString getCurrentFeeAddress() const;
    QString getCurrentCurrency() const;
    QString getAutoOptimizationStatus() const;
    QString getMaximizedStatus() const;
    QString getConnection() const;
    QString getWalletFile() const;
    QString getWalletName() const;
    QString getAddressBookFile() const;
    QStringList getMiningPoolList() const;
    bool isEncrypted() const;
    bool isTrackingMode() const;
    quint64 getOptimizationInterval() const;
    QString getVersion() const;
    QString getLanguage() const;
    QString getFont() const;
    int getFontSize() const;
    bool isStartOnLoginEnabled() const;
    QString getDefaultWalletPath() const;
    QString getDefaultWalletDir() const;
#ifndef QT_NO_SYSTEMTRAYICON
    bool isMinimizeToTrayEnabled() const;
    bool isCloseToTrayEnabled() const;
#endif
    bool isAutoRefreshData() const;

    void setWalletFile(const QString &_file);
    void setEncrypted(bool _encrypted);
    void setTrackingMode(bool _tracking);
    void setStartOnLoginEnabled(bool _enable);
    void setLanguage(const QString &_language);
    void setFont(const QString &_font);
    void setFontSize(const int &_change);
    void setConnection(const QString &_connection);
    void setCurrentLocalDaemonPort(const quint16 &_daemonPort);
    void setCurrentRemoteNode(const QString &_remoteNode);
    void setCurrentFeeAddress(const QString &_feeAddress);
    void setCurrentCurrency(const QString &_currency);
    void setAutoOptimizationStatus(const QString &_status);
    void setMaximizedStatus(const QString &_status);
    void setOptimizationInterval(quint64 _interval);
    void setRpcNodesList(const QStringList &_RpcNodesList);
    void setMiningPoolList(const QStringList &_miningPoolList);
    void setAutoRefreshData(bool autoRefreshData);

#ifndef QT_NO_SYSTEMTRAYICON
    void setMinimizeToTrayEnabled(bool _enable);
    void setCloseToTrayEnabled(bool _enable);
#endif

  private:
    QJsonObject m_settings;
    QString m_addressBookFile;
    CommandLineParser *m_cmdLineParser;
    QString m_connectionMode;
    QString m_autoOptimizationStatus;
    QString m_currentLang;
    QString m_currentCurrency;
    QString m_remoteNode;
    quint16 m_daemonPort;

    Settings();
    ~Settings();

    void saveSettings() const;
  };

}  // namespace WalletGui