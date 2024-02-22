// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <CryptoNoteCore/CoreConfig.h>
#include <P2p/NetNodeConfig.h>
#include <Wallet/WalletErrors.h>
#include "CurrencyAdapter.h"
#include "LoggerAdapter.h"
#include "NodeAdapter.h"
#include "Settings.h"

namespace WalletGui
{

namespace
{

std::vector<std::string> convertStringListToVector(const QStringList &list)
{
  std::vector<std::string> result;
  Q_FOREACH (const QString &item, list)
  {
    result.push_back(item.toStdString());
  }

  return result;
}

} // namespace

class InProcessNodeInitializer : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(InProcessNodeInitializer)

Q_SIGNALS:
  void nodeInitCompletedSignal();
  void nodeInitFailedSignal(int _errorCode);
  void nodeDeinitCompletedSignal();

public:
  explicit InProcessNodeInitializer(QObject *_parent = nullptr)
  {
  }

  ~InProcessNodeInitializer() override = default;

  void start(Node **_node, const cn::Currency *currency, INodeCallback *_callback, logging::LoggerManager *_loggerManager,
             const cn::CoreConfig &_coreConfig, const cn::NetNodeConfig &_netNodeConfig)
  {
    (*_node) = createInprocessNode(*currency, *_loggerManager, _coreConfig, _netNodeConfig, *_callback);
    try
    {
      (*_node)->init([this](std::error_code _err) {
        if (_err)
        {
          Q_EMIT nodeInitFailedSignal(_err.value());
          QCoreApplication::processEvents();
          return;
        }

        Q_EMIT nodeInitCompletedSignal();
        QCoreApplication::processEvents();
      });
    }
    catch (std::exception&)
    {
      Q_EMIT nodeInitFailedSignal(cn::error::INTERNAL_WALLET_ERROR);
      QCoreApplication::processEvents();
      return;
    }

    delete *_node;
    *_node = nullptr;
    Q_EMIT nodeDeinitCompletedSignal();
  }

  void stop(Node **_node) const
  {
    Q_CHECK_PTR(_node);
    (*_node)->deinit();
  }
};

class RpcNodeInitializer : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(RpcNodeInitializer)

Q_SIGNALS:
  void nodeInitCompletedSignal();
  void nodeInitFailedSignal(int _errorCode);
  void nodeDeinitCompletedSignal();

public:
  explicit RpcNodeInitializer(QObject *_parent = nullptr)
  {
  }

  ~RpcNodeInitializer() override = default;

  void start(Node **_node, const cn::Currency *currency, INodeCallback *_callback, logging::LoggerManager *_loggerManager,
             const std::string& _nodeHost, const unsigned short &_nodePort)
  {
    (*_node) = createRpcNode(*currency, *_loggerManager, *_callback, _nodeHost, _nodePort);
    try
    {
      (*_node)->init([this](std::error_code _err) {
        if (_err)
        {
          Q_EMIT nodeInitFailedSignal(_err.value());
          QCoreApplication::processEvents();
          return;
        }

        Q_EMIT nodeInitCompletedSignal();
        QCoreApplication::processEvents();
      });
    }
    catch (std::exception&)
    {
      Q_EMIT nodeInitFailedSignal(cn::error::INTERNAL_WALLET_ERROR);
      QCoreApplication::processEvents();
      return;
    }

    delete *_node;
    *_node = nullptr;
    Q_EMIT nodeDeinitCompletedSignal();
  }

  void stop(Node **_node) const
  {
    Q_CHECK_PTR(_node);
    (*_node)->deinit();
  }
};

NodeAdapter &NodeAdapter::instance()
{
  static NodeAdapter inst;
  return inst;
}

NodeAdapter::NodeAdapter() : QObject(), m_inProcessNodeInitializer(new InProcessNodeInitializer), m_rpcNodeInitializer(new RpcNodeInitializer)
{
  m_inProcessNodeInitializer->moveToThread(&m_inProcessNodeInitializerThread);
  m_rpcNodeInitializer->moveToThread(&m_rpcNodeInitializerThread);

  qRegisterMetaType<cn::CoreConfig>("cn::CoreConfig");
  qRegisterMetaType<cn::NetNodeConfig>("cn::NetNodeConfig");
  qRegisterMetaType<std::string>("std::string");

  connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeInitCompletedSignal, this, &NodeAdapter::nodeInitCompletedSignal, Qt::QueuedConnection);
  connect(this, &NodeAdapter::initInProcessNodeSignal, m_inProcessNodeInitializer, &InProcessNodeInitializer::start, Qt::QueuedConnection);
  connect(this, &NodeAdapter::deinitInProcessNodeSignal, m_inProcessNodeInitializer, &InProcessNodeInitializer::stop, Qt::QueuedConnection);

  connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeInitCompletedSignal, this, &NodeAdapter::nodeInitCompletedSignal, Qt::QueuedConnection);
  connect(this, &NodeAdapter::initRpcNodeSignal, m_rpcNodeInitializer, &RpcNodeInitializer::start, Qt::QueuedConnection);
  connect(this, &NodeAdapter::deinitRpcNodeSignal, m_rpcNodeInitializer, &RpcNodeInitializer::stop, Qt::QueuedConnection);
}

quintptr NodeAdapter::getPeerCount() const
{
  Q_ASSERT(m_node != nullptr);
  return m_node->getPeerCount();
}

std::string NodeAdapter::convertPaymentId(const QString &_paymentIdString) const
{
  Q_CHECK_PTR(m_node);
  return m_node->convertPaymentId(_paymentIdString.toStdString());
}

QString NodeAdapter::extractPaymentId(const std::string &_extra) const
{
  Q_CHECK_PTR(m_node);
  return QString::fromStdString(m_node->extractPaymentId(_extra));
}

std::unique_ptr<cn::IWallet> NodeAdapter::createWallet() const
{
  Q_CHECK_PTR(m_node);
  return m_node->createWallet();
}

bool NodeAdapter::init()
{
  Q_ASSERT(m_node == nullptr);

  /* First get the connection type */
  QString connection = Settings::instance().getConnection();

  /* Autoremote is a the remote node conection which retrieves a random for-fee remoten node
     from the node pool on the explorer. */
  if (connection.compare("autoremote") == 0)
  {
    /* Pull a random node from the node pool list */
    auto *nam = new QNetworkAccessManager(this);
    connect(nam, &QNetworkAccessManager::finished, this, &NodeAdapter::downloadFinished);
    const QUrl url = QUrl::fromUserInput("https://explorer.conceal.network/pool/random?hasFeeAddr=true&isReachable=true&isSynced=true");
    QNetworkRequest request(url);
    nam->get(request);
  }

  /* If it is not an autoremote its either a local node, or a remote note. By default
     the wallet creates a local node and starts the sync process. */
  if (connection.compare("embedded") == 0 || Settings::instance().getCurrentRemoteNode() == "")
  {
    return initInProcessNode();
  }
  else
  {
    return initRpcNode();
  }
}

quint64 NodeAdapter::getLastKnownBlockHeight() const
{
  Q_CHECK_PTR(m_node);
  return m_node->getLastKnownBlockHeight();
}

quint64 NodeAdapter::getLastLocalBlockHeight() const
{
  Q_CHECK_PTR(m_node);
  return m_node->getLastLocalBlockHeight();
}

QDateTime NodeAdapter::getLastLocalBlockTimestamp() const
{
  Q_CHECK_PTR(m_node);
  return QDateTime::fromTime_t(m_node->getLastLocalBlockTimestamp(), Qt::UTC);
}

void NodeAdapter::peerCountUpdated(Node &_node, size_t _count)
{
  Q_UNUSED(_node);
}

void NodeAdapter::localBlockchainUpdated(Node &_node, uint64_t _height)
{
  Q_UNUSED(_node);
  Q_EMIT localBlockchainUpdatedSignal(_height);
}

void NodeAdapter::lastKnownBlockHeightUpdated(Node &_node, uint64_t _height)
{
  Q_UNUSED(_node);
  Q_EMIT lastKnownBlockHeightUpdatedSignal(_height);
}

/* Get a random for-fee remote node from the explorer
   remote node pool list and then save as the current remote
   node in the configuration */
void NodeAdapter::downloadFinished(QNetworkReply *reply)
{
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull())
  {
    return;
    Settings::instance().setCurrentRemoteNode("");
  }
  QJsonObject obj = doc.object();
  QString address = obj.value("url").toString();
  Settings::instance().setCurrentRemoteNode(address);
}

bool NodeAdapter::initInProcessNode()
{
  Q_ASSERT(m_node == nullptr);
  m_inProcessNodeInitializerThread.start();
  cn::CoreConfig coreConfig = makeCoreConfig();
  cn::NetNodeConfig netNodeConfig = makeNetNodeConfig();
  Q_EMIT initInProcessNodeSignal(&m_node, &CurrencyAdapter::instance().getCurrency(), this, &LoggerAdapter::instance().getLoggerManager(), coreConfig, netNodeConfig);
  QEventLoop waitLoop;
  bool initCompleted = false;
  connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeInitCompletedSignal, [&initCompleted]() {
    initCompleted = true;
  });
  connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeInitFailedSignal, [&initCompleted]() {
    initCompleted = false;
  });
  connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeInitCompletedSignal, &waitLoop, &QEventLoop::quit);
  connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeInitFailedSignal, &waitLoop, &QEventLoop::exit);

  if (waitLoop.exec() != 0 || !initCompleted)
  {
    return false;
  }

  Q_EMIT localBlockchainUpdatedSignal(getLastLocalBlockHeight());
  Q_EMIT lastKnownBlockHeightUpdatedSignal(getLastKnownBlockHeight());
  return true;
}

bool NodeAdapter::initRpcNode()
{
  Q_ASSERT(m_node == nullptr);
  m_rpcNodeInitializerThread.start();
  QUrl remoteNodeUrl = QUrl::fromUserInput(Settings::instance().getCurrentRemoteNode());
  Q_EMIT initRpcNodeSignal(&m_node, &CurrencyAdapter::instance().getCurrency(), this, &LoggerAdapter::instance().getLoggerManager(), remoteNodeUrl.host().toStdString(), remoteNodeUrl.port());
  QEventLoop waitLoop;
  bool initCompleted = false;
  connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeInitCompletedSignal, [&initCompleted]() {
    initCompleted = true;
  });
  connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeInitFailedSignal, [&initCompleted]() {
    initCompleted = false;
  });
  connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeInitCompletedSignal, &waitLoop, &QEventLoop::quit);
  connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeInitFailedSignal, &waitLoop, &QEventLoop::exit);

  if (waitLoop.exec() != 0 || !initCompleted)
  {
    return false;
  }

  Q_EMIT localBlockchainUpdatedSignal(getLastLocalBlockHeight());
  Q_EMIT lastKnownBlockHeightUpdatedSignal(getLastKnownBlockHeight());
  return true;
}

void NodeAdapter::deinit()
{
  if (m_node != nullptr)
  {
    if (m_inProcessNodeInitializerThread.isRunning())
    {
      m_inProcessNodeInitializer->stop(&m_node);
      QEventLoop waitLoop;
      connect(m_inProcessNodeInitializer, &InProcessNodeInitializer::nodeDeinitCompletedSignal, &waitLoop, &QEventLoop::quit, Qt::QueuedConnection);
      waitLoop.exec();
      m_inProcessNodeInitializerThread.quit();
      m_inProcessNodeInitializerThread.wait();
    }
    else if (m_rpcNodeInitializerThread.isRunning())
    {
      m_rpcNodeInitializer->stop(&m_node);
      QEventLoop waitLoop;
      connect(m_rpcNodeInitializer, &RpcNodeInitializer::nodeDeinitCompletedSignal, &waitLoop, &QEventLoop::quit, Qt::QueuedConnection);
      waitLoop.exec();
      m_rpcNodeInitializerThread.quit();
      m_rpcNodeInitializerThread.wait();
    } 
    else
    {
      delete m_node;
      m_node = nullptr;
    }
  }
}

cn::CoreConfig NodeAdapter::makeCoreConfig() const
{
  cn::CoreConfig config;
  boost::program_options::variables_map options;
  boost::any dataDir = Settings::instance().getDataDir().absolutePath().toStdString();
  boost::any testnet = Settings::instance().isTestnet();
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  options.insert(std::make_pair("testnet", boost::program_options::variable_value(testnet, false)));
  config.init(options);
  return config;
}

cn::NetNodeConfig NodeAdapter::makeNetNodeConfig() const
{
  cn::NetNodeConfig config;
  boost::program_options::variables_map options;
  boost::any p2pBindIp = Settings::instance().getP2pBindIp().toStdString();
  boost::any p2pBindPort = Settings::instance().getP2pBindPort();
  boost::any p2pExternalPort = Settings::instance().getP2pExternalPort();
  boost::any p2pAllowLocalIp = Settings::instance().hasAllowLocalIpOption();
  boost::any dataDir = Settings::instance().getDataDir().absolutePath().toStdString();
  boost::any hideMyPort = Settings::instance().hasHideMyPortOption();
  boost::any testnet = Settings::instance().isTestnet();
  options.insert(std::make_pair("p2p-bind-ip", boost::program_options::variable_value(p2pBindIp, false)));
  options.insert(std::make_pair("p2p-bind-port", boost::program_options::variable_value(p2pBindPort, false)));
  options.insert(std::make_pair("p2p-external-port", boost::program_options::variable_value(p2pExternalPort, false)));
  options.insert(std::make_pair("allow-local-ip", boost::program_options::variable_value(p2pAllowLocalIp, false)));
  options.insert(std::make_pair("testnet", boost::program_options::variable_value(testnet, false)));
  std::vector<std::string> peerList = convertStringListToVector(Settings::instance().getPeers());
  if (!peerList.empty())
  {
    options.insert(std::make_pair("add-peer", boost::program_options::variable_value(peerList, false)));
  }

  std::vector<std::string> priorityNodeList = convertStringListToVector(Settings::instance().getPriorityNodes());
  if (!priorityNodeList.empty())
  {
    options.insert(std::make_pair("add-priority-node", boost::program_options::variable_value(priorityNodeList, false)));
  }

  std::vector<std::string> exclusiveNodeList = convertStringListToVector(Settings::instance().getExclusiveNodes());
  if (!exclusiveNodeList.empty())
  {
    options.insert(std::make_pair("add-exclusive-node", boost::program_options::variable_value(exclusiveNodeList, false)));
  }

  std::vector<std::string> seedNodeList = convertStringListToVector(Settings::instance().getSeedNodes());
  if (!seedNodeList.empty())
  {
    options.insert(std::make_pair("seed-node", boost::program_options::variable_value(seedNodeList, false)));
  }

  options.insert(std::make_pair("hide-my-port", boost::program_options::variable_value(hideMyPort, false)));
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  config.init(options);
  config.setTestnet(Settings::instance().isTestnet());
  return config;
}

} // namespace WalletGui

#include "NodeAdapter.moc"
