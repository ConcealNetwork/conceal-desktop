// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <INode.h>
#include <IWallet.h>

#include "CryptoNoteWrapper.h"

namespace cn {

class Currency;

}

namespace logging {
  class LoggerManager;
}

namespace WalletGui {

class InProcessNodeInitializer;
class RpcNodeInitializer;

class NodeAdapter : public QObject, public INodeCallback {
  Q_OBJECT
  Q_DISABLE_COPY(NodeAdapter)

public:
  static NodeAdapter& instance();

  quintptr getPeerCount() const;
  std::string convertPaymentId(const QString& _payment_id_string) const;
  QString extractPaymentId(const std::string& _extra) const;
  std::unique_ptr<cn::IWallet> createWallet() const;

  bool init();
  void deinit();
  quint64 getLastKnownBlockHeight() const;
  quint64 getLastLocalBlockHeight() const;
  QDateTime getLastLocalBlockTimestamp() const;
  void peerCountUpdated(Node& _node, size_t _count) Q_DECL_OVERRIDE;
  void localBlockchainUpdated(Node& _node, uint64_t _height) Q_DECL_OVERRIDE;
  void lastKnownBlockHeightUpdated(Node& _node, uint64_t _height) Q_DECL_OVERRIDE;

private:
  Node* m_node;
  QThread m_inProcessNodeInitializerThread;
  QThread m_rpcNodeInitializerThread;
  InProcessNodeInitializer* m_inProcessNodeInitializer;
  RpcNodeInitializer* m_rpcNodeInitializer;
  void downloadFinished(QNetworkReply *reply);

  NodeAdapter();
  ~NodeAdapter() override = default;

  bool initInProcessNode();
  bool initRpcNode();
  cn::CoreConfig makeCoreConfig() const;
  cn::NetNodeConfig makeNetNodeConfig() const;

Q_SIGNALS:
  void localBlockchainUpdatedSignal(quint64 _height);
  void lastKnownBlockHeightUpdatedSignal(quint64 _height);
  void nodeInitCompletedSignal();
  void peerCountUpdatedSignal(quintptr _count);
  void initInProcessNodeSignal(Node** _node, const cn::Currency* currency, INodeCallback* _callback, logging::LoggerManager* _loggerManager,
    const cn::CoreConfig& _coreConfig, const cn::NetNodeConfig& _netNodeConfig);
  void deinitInProcessNodeSignal(Node** _node);
  void initRpcNodeSignal(Node** _node, const cn::Currency* currency, INodeCallback* _callback,
                         logging::LoggerManager* _loggerManager, const std::string& _nodeHost,
                         const unsigned short& _nodePort);
  void deinitRpcNodeSignal(Node** _node);
};

}
