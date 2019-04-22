// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2016 The Karbowanec developers
// Copyright (c) 2018 The Circle Foundation

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCodec>
#include <Common/Util.h>
#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "Settings.h"

namespace WalletGui {

Q_DECL_CONSTEXPR char OPTION_WALLET_FILE[] = "walletFile";
Q_DECL_CONSTEXPR char OPTION_ENCRYPTED[] = "encrypted";
Q_DECL_CONSTEXPR char OPTION_MINING_POOLS[] = "miningPools";
Q_DECL_CONSTEXPR char OPTION_CONNECTION[] = "connectionMode";
Q_DECL_CONSTEXPR char OPTION_RPCNODES[] = "remoteNodes";
Q_DECL_CONSTEXPR char OPTION_DAEMON_PORT[] = "daemonPort";
Q_DECL_CONSTEXPR char OPTION_REMOTE_NODE[] = "remoteNode";
Q_DECL_CONSTEXPR char OPTION_FEE_ADDRESS[] = "feeAddress";

Settings& Settings::instance() {
  static Settings inst;
  return inst;
}

Settings::Settings() : QObject(), m_cmdLineParser(nullptr) {
}

Settings::~Settings() {
}

void Settings::setCommandLineParser(CommandLineParser* _cmdLineParser) {
  Q_CHECK_PTR(_cmdLineParser);
  m_cmdLineParser = _cmdLineParser;
}

void Settings::load() {

  QFile cfgFile(getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".cfg"));

  if (cfgFile.open(QIODevice::ReadOnly)) 
  {
    m_settings = QJsonDocument::fromJson(cfgFile.readAll()).object();
    cfgFile.close();

    if (!m_settings.contains(OPTION_WALLET_FILE)) {
      m_addressBookFile = getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".addressbook");
    } else {
      m_addressBookFile = m_settings.value(OPTION_WALLET_FILE).toString();
      m_addressBookFile.replace(m_addressBookFile.lastIndexOf(".wallet"), 7, ".addressbook");
    }

    if (!m_settings.contains(OPTION_FEE_ADDRESS)) {
      m_settings.insert(OPTION_FEE_ADDRESS, ""); 
    }

    if (!m_settings.contains(OPTION_DAEMON_PORT)) {
      m_settings.insert(OPTION_DAEMON_PORT, CryptoNote::RPC_DEFAULT_PORT); // default daemon port
    }    

  } else {
    m_addressBookFile = getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".addressbook");
  }

  if (!m_settings.contains(OPTION_CONNECTION)) {
    m_settings.insert(OPTION_CONNECTION, "embedded");
  }

  if (!m_settings.contains(OPTION_REMOTE_NODE)) {
    m_settings.insert(OPTION_REMOTE_NODE, "node.conceal.network:16000");
  }

  if (m_settings.contains(OPTION_CONNECTION)) {
    m_connectionMode = m_settings.value(OPTION_CONNECTION).toString();
  }

  if (m_settings.contains(OPTION_REMOTE_NODE)) {
    m_connectionMode = m_settings.value(OPTION_REMOTE_NODE).toString();
  }

  QStringList defaultPoolList;

  defaultPoolList << "pool.conceal.network:3333" << "ccx.go-mine.it:3333" << "conceal.herominers.com:10361" << "ccx.scecf.org:20000" << "ccx.bluerockpools.net:5009 " << "ccx.heigh-ho.funkypenguin.co.nz:3344";
  if (!m_settings.contains(OPTION_MINING_POOLS)) {

    setMiningPoolList(QStringList() << defaultPoolList);
  } else 
  {

    QStringList poolList = getMiningPoolList();

    Q_FOREACH (const QString& pool, defaultPoolList) 
    {

      if (!poolList.contains(pool)) 
      {

        poolList << pool;
      }
    }

    setMiningPoolList(poolList);
  }

  QStringList defaultNodesList;
  defaultNodesList << "node.conceal.network:16000";
  
  if (!m_settings.contains(OPTION_RPCNODES)) 
  {

    setRpcNodesList(QStringList() << defaultNodesList);
  } else {

    QStringList nodesList = getRpcNodesList();
    Q_FOREACH (const QString& node, defaultNodesList) 
    {

      if (!nodesList.contains(node)) 
      {

        nodesList << node;
      }
    }
    setRpcNodesList(nodesList);
  }
}


QString Settings::getVersion() const {
  return VERSION;
}


bool Settings::isTestnet() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->hasTestnetOption();
}

bool Settings::hasAllowLocalIpOption() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->hasAllowLocalIpOption();
}

bool Settings::hasHideMyPortOption() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->hasHideMyPortOption();
}

QString Settings::getP2pBindIp() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getP2pBindIp();
}

quint16 Settings::getP2pBindPort() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getP2pBindPort();
}

quint16 Settings::getP2pExternalPort() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getP2pExternalPort();
}

QStringList Settings::getPeers() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getPeers();
}

QStringList Settings::getPriorityNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getPiorityNodes();
}

QStringList Settings::getExclusiveNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getExclusiveNodes();
}

QStringList Settings::getSeedNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getSeedNodes();
}

QStringList Settings::getRpcNodesList() const {
  QStringList res;
  if (m_settings.contains(OPTION_RPCNODES)) {
    res << m_settings.value(OPTION_RPCNODES).toVariant().toStringList();
  }

  return res;
}

QString Settings::getCurrentFeeAddress() const {
    QString feeAddress;
    if (m_settings.contains(OPTION_FEE_ADDRESS)) {
      feeAddress = m_settings.value(OPTION_FEE_ADDRESS).toString();
    }
    return feeAddress;
}

QString Settings::getCurrentRemoteNode() const {
    QString remotenode;
    if (m_settings.contains(OPTION_REMOTE_NODE)) {
      remotenode = m_settings.value(OPTION_REMOTE_NODE).toString();
    }
    return remotenode;
}

QString Settings::getConnection() const {
    QString connection;
    if (m_settings.contains(OPTION_CONNECTION)) {
        connection = m_settings.value(OPTION_CONNECTION).toString();
    }
    else {
      connection = "remote"; 
    }
    return connection;
}
void Settings::setCurrentRemoteNode(const QString& _remoteNode) {
    if (!_remoteNode.isEmpty()) {
    m_settings.insert(OPTION_REMOTE_NODE, _remoteNode);
    }
    saveSettings();
}

void Settings::setCurrentFeeAddress(const QString& _feeAddress) {
    m_settings.insert(OPTION_FEE_ADDRESS, _feeAddress);
    saveSettings();
}

void Settings::setConnection(const QString& _connection) {
    m_settings.insert(OPTION_CONNECTION, _connection);
    saveSettings();
}

void Settings::setRpcNodesList(const QStringList &_RpcNodesList) {
  if (getRpcNodesList() != _RpcNodesList) {
    m_settings.insert(OPTION_RPCNODES, QJsonArray::fromStringList(_RpcNodesList));
  }
  saveSettings();
}

quint16 Settings::getCurrentLocalDaemonPort() const {
    quint16 port;
    if (m_settings.contains(OPTION_DAEMON_PORT)) {
        port = m_settings.value(OPTION_DAEMON_PORT).toVariant().toInt();
    }
    return port;
}

QDir Settings::getDataDir() const {
  Q_CHECK_PTR(m_cmdLineParser);
  return QDir(m_cmdLineParser->getDataDir());
}

QString Settings::getWalletFile() const {
  return m_settings.contains(OPTION_WALLET_FILE) ? m_settings.value(OPTION_WALLET_FILE).toString() :
    getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".wallet");
}

QString Settings::getWalletName() const {
  /* Get the wallet file name */
  QString walletFile = getWalletFile();
  std::string wallet = walletFile.toStdString();

  /* Remove directory if present.
     do this before extension removal in case directory has a period character. */
  const size_t last_slash_idx = wallet.find_last_of("\\/");
  if (std::string::npos != last_slash_idx) {
    wallet.erase(0, last_slash_idx + 1);
  }
  /*  Remove extension if present */
  const size_t period_idx = wallet.rfind('.');
  if (std::string::npos != period_idx) {
    wallet.erase(period_idx);
  }
  /* Return QString */
  return QString::fromStdString(wallet);
}

QString Settings::getAddressBookFile() const {
  return m_addressBookFile;
}

bool Settings::isEncrypted() const {
  return m_settings.contains(OPTION_ENCRYPTED) ? m_settings.value(OPTION_ENCRYPTED).toBool() : false;
}

QStringList Settings::getMiningPoolList() const {
  QStringList res;
  if (m_settings.contains(OPTION_MINING_POOLS)) {
    res << m_settings.value(OPTION_MINING_POOLS).toVariant().toStringList();
  }

  return res;
}

bool Settings::isStartOnLoginEnabled() const {
  bool res = false;
#ifdef Q_OS_MAC
  QDir autorunDir = QDir::home();
  if (!autorunDir.cd("Library") || !autorunDir.cd("LaunchAgents")) {
    return false;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".plist");
  if (!QFile::exists(autorunFilePath)) {
    return false;
  }

  QSettings autorunSettings(autorunFilePath, QSettings::NativeFormat);
  res = autorunSettings.value("RunAtLoad", false).toBool();
#elif defined(Q_OS_LINUX)
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (configPath.isEmpty()) {
    return false;
  }

  QDir autorunDir(configPath);
  if (!autorunDir.cd("autostart")) {
    return false;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".desktop");
  res = QFile::exists(autorunFilePath);
#elif defined(Q_OS_WIN)
  QSettings autorunSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString keyName = QString("%1Wallet").arg(CurrencyAdapter::instance().getCurrencyDisplayName());
  res = autorunSettings.contains(keyName) &&
    !QDir::fromNativeSeparators(autorunSettings.value(keyName).toString()).compare(QCoreApplication::applicationFilePath());
#endif
  return res;
}

#ifdef Q_OS_WIN
bool Settings::isMinimizeToTrayEnabled() const {
  return m_settings.contains("minimizeToTray") ? m_settings.value("minimizeToTray").toBool() : false;
}

bool Settings::isCloseToTrayEnabled() const {
  return m_settings.contains("closeToTray") ? m_settings.value("closeToTray").toBool() : false;
}
#endif

void Settings::setWalletFile(const QString& _file) {
  if (_file.endsWith(".wallet") || _file.endsWith(".keys")) {
    m_settings.insert(OPTION_WALLET_FILE, _file);
  } else {
    m_settings.insert(OPTION_WALLET_FILE, _file + ".wallet");
  }

  saveSettings();
  m_addressBookFile = m_settings.value(OPTION_WALLET_FILE).toString();
  m_addressBookFile.replace(m_addressBookFile.lastIndexOf(".wallet"), 7, ".addressbook");
}

void Settings::setEncrypted(bool _encrypted) {
  if (isEncrypted() != _encrypted) {
    m_settings.insert(OPTION_ENCRYPTED, _encrypted);
    saveSettings();
  }
}

void Settings::setCurrentTheme(const QString& _theme) {
}

void Settings::setStartOnLoginEnabled(bool _enable) {
#ifdef Q_OS_MAC
  QDir autorunDir = QDir::home();
  if (!autorunDir.cd("Library") || !autorunDir.cd("LaunchAgents")) {
    return;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".plist");
  QSettings autorunSettings(autorunFilePath, QSettings::NativeFormat);
  autorunSettings.setValue("Label", "org." + QCoreApplication::applicationName());
  autorunSettings.setValue("Program", QCoreApplication::applicationFilePath());
  autorunSettings.setValue("RunAtLoad", _enable);
  autorunSettings.setValue("ProcessType", "InterActive");
#elif defined(Q_OS_LINUX)
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (configPath.isEmpty()) {
    return;
  }

  QDir autorunDir(configPath);
  if(!autorunDir.exists("autostart")) {
    autorunDir.mkdir("autostart");
  }

  if (!autorunDir.cd("autostart")) {
    return;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".desktop");
  QFile autorunFile(autorunFilePath);
  if (!autorunFile.open(QFile::WriteOnly | QFile::Truncate)) {
    return;
  }

  if (_enable) {
    autorunFile.write("[Desktop Entry]\n");
    autorunFile.write("Type=Application\n");
    autorunFile.write(QString("Name=%1 Wallet\n").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).toLocal8Bit());
    autorunFile.write(QString("Exec=%1\n").arg(QCoreApplication::applicationFilePath()).toLocal8Bit());
    autorunFile.write("Terminal=false\n");
    autorunFile.write("Hidden=false\n");
    autorunFile.close();
  } else {
    QFile::remove(autorunFilePath);
  }
#elif defined(Q_OS_WIN)
  QSettings autorunSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString keyName = QString("%1Wallet").arg(CurrencyAdapter::instance().getCurrencyDisplayName());
  if (_enable) {
    autorunSettings.setValue(keyName, QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
  } else {
    autorunSettings.remove(keyName);
  }
#endif
}

void Settings::setMiningPoolList(const QStringList &_miningPoolList) {
  if (getMiningPoolList() != _miningPoolList) {
    m_settings.insert(OPTION_MINING_POOLS, QJsonArray::fromStringList(_miningPoolList));
  }

  saveSettings();
}

#ifdef Q_OS_WIN
void Settings::setMinimizeToTrayEnabled(bool _enable) {
  if (isMinimizeToTrayEnabled() != _enable) {
    m_settings.insert("minimizeToTray", _enable);
    saveSettings();
  }
}

void Settings::setCloseToTrayEnabled(bool _enable) {
  if (isCloseToTrayEnabled() != _enable) {
    m_settings.insert("closeToTray", _enable);
    saveSettings();
  }
}
#endif

void Settings::saveSettings() const {
  QFile cfgFile(getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".cfg"));
  if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QJsonDocument cfg_doc(m_settings);
    cfgFile.write(cfg_doc.toJson());
    cfgFile.close();
  }
}

}
