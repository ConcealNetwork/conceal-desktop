// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
//  
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2020 Conceal Network & Conceal Devs
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "CryptoNoteWrapper.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/Currency.h"
#include "NodeRpcProxy/NodeRpcProxy.h"
#include "CryptoNoteCore/CoreConfig.h"
#include "P2p/NetNodeConfig.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "InProcessNode/InProcessNode.h"
#include "P2p/NetNode.h"
#include "WalletLegacy/WalletLegacy.h"
#include "Logging/LoggerManager.h"
#include "System/Dispatcher.h"

namespace WalletGui {

namespace {

bool parsePaymentId(const std::string& payment_id_str, crypto::Hash& payment_id) {
  return cn::parsePaymentId(payment_id_str, payment_id);
}

std::string convertPaymentId(const std::string& paymentIdString) {
  if (paymentIdString.empty()) {
    return "";
  }

  crypto::Hash paymentId;
  if (!parsePaymentId(paymentIdString, paymentId)) {
    std::stringstream errorStr;
    errorStr << "Payment id has invalid format: \"" + paymentIdString + "\", expected 64-character string";
    throw std::runtime_error(errorStr.str());
  }

  std::vector<uint8_t> extra;
  cn::BinaryArray extraNonce;
  cn::setPaymentIdToTransactionExtraNonce(extraNonce, paymentId);
  if (!cn::addExtraNonceToTransactionExtra(extra, extraNonce)) {
    std::stringstream errorStr;
    errorStr << "Something went wrong with payment_id. Please check its format: \"" + paymentIdString + "\", expected 64-character string";
    throw std::runtime_error(errorStr.str());
  }

  return std::string(extra.begin(), extra.end());
}

std::string extractPaymentId(const std::string& extra) {
  std::vector<cn::TransactionExtraField> extraFields;
  std::vector<uint8_t> extraVector;
  std::copy(extra.begin(), extra.end(), std::back_inserter(extraVector));

  if (!cn::parseTransactionExtra(extraVector, extraFields)) {
    throw std::runtime_error("Can't parse extra");
  }

  std::string result;
  cn::TransactionExtraNonce extraNonce;
  if (cn::findTransactionExtraFieldByType(extraFields, extraNonce)) {
    crypto::Hash paymentIdHash;
    if (cn::getPaymentIdFromTransactionExtraNonce(extraNonce.nonce, paymentIdHash)) {
      unsigned char* buff = reinterpret_cast<unsigned char *>(&paymentIdHash);
      for (size_t i = 0; i < sizeof(paymentIdHash); ++i) {
        result.push_back("0123456789ABCDEF"[buff[i] >> 4]);
        result.push_back("0123456789ABCDEF"[buff[i] & 15]);
      }
    }
  }

  return result;
}


}

Node::~Node() {
}

class RpcNode : cn::INodeObserver, public Node {
public:
  RpcNode(const cn::Currency& currency, logging::LoggerManager& logManager, INodeCallback& callback, const std::string& nodeHost, unsigned short nodePort) :
    m_callback(callback),
    m_currency(currency),
    m_logger(logManager),
    m_node(nodeHost, nodePort) {
    m_node.addObserver(this);
  }

  ~RpcNode() override {
  }

  void init(const std::function<void(std::error_code)>& callback) override {
    m_node.init(callback);
  }

  void deinit() override {
    /* nothing to be done here */
  }

  std::string convertPaymentId(const std::string& paymentIdString) override {
    return WalletGui::convertPaymentId(paymentIdString);
  }

  std::string extractPaymentId(const std::string& extra) override {
    return WalletGui::extractPaymentId(extra);
  }

  uint64_t getLastKnownBlockHeight() const override {
    return m_node.getLastKnownBlockHeight();
  }

  uint64_t getLastLocalBlockHeight() const override {
    return m_node.getLastLocalBlockHeight();
  }

  uint64_t getLastLocalBlockTimestamp() const override {
    return m_node.getLastLocalBlockTimestamp();
  }

  uint64_t getPeerCount() const override {
    return m_node.getPeerCount();
  }

  cn::IWalletLegacy* createWallet() override {
    return new cn::WalletLegacy(m_currency, m_node, m_logger);
  }

private:
  INodeCallback& m_callback;
  const cn::Currency& m_currency;
  cn::NodeRpcProxy m_node;
  logging::LoggerManager& m_logger;

  void peerCountUpdated(size_t count) {
    m_callback.peerCountUpdated(*this, count);
  }

  void localBlockchainUpdated(uint64_t height) {
    m_callback.localBlockchainUpdated(*this, height);
  }

  void lastKnownBlockHeightUpdated(uint64_t height) {
    m_callback.lastKnownBlockHeightUpdated(*this, height);
  }
};

class InprocessNode : cn::INodeObserver, public Node {
public:
  InprocessNode(const cn::Currency& currency, logging::LoggerManager& logManager, const cn::CoreConfig& coreConfig,
    const cn::NetNodeConfig& netNodeConfig, INodeCallback& callback) :
    m_currency(currency), 
    m_dispatcher(),
    m_loggerManager(logManager),
    m_callback(callback),
    m_coreConfig(coreConfig),
    m_netNodeConfig(netNodeConfig),
    m_protocolHandler(currency, m_dispatcher, m_core, nullptr, logManager),
    m_core(currency, &m_protocolHandler, logManager, false, false),
    m_nodeServer(m_dispatcher, m_protocolHandler, logManager),
    m_node(m_core, m_protocolHandler) {

    cn::Checkpoints checkpoints(logManager);
    checkpoints.load_checkpoints();
    checkpoints.load_checkpoints_from_dns();
    m_core.set_checkpoints(std::move(checkpoints));
    m_core.set_cryptonote_protocol(&m_protocolHandler);
    m_protocolHandler.set_p2p_endpoint(&m_nodeServer);
  }

  ~InprocessNode() override {

  }

  void init(const std::function<void(std::error_code)>& callback) override {
    try {
      if (!m_core.init(m_coreConfig, cn::MinerConfig(), true)) {
        callback(make_error_code(cn::error::NOT_INITIALIZED));
        return;
      }

      if (!m_nodeServer.init(m_netNodeConfig)) {
        callback(make_error_code(cn::error::NOT_INITIALIZED));
        return;
      }
    } catch (std::runtime_error&) {
      callback(make_error_code(cn::error::NOT_INITIALIZED));
      return;
    }

    m_node.init([this, callback](std::error_code ec) {
      m_node.addObserver(this);
      callback(ec);
    });

    m_nodeServer.run();
    m_nodeServer.deinit();
    m_node.shutdown();
    m_core.deinit();
  }

  void deinit() override {
    m_nodeServer.sendStopSignal();
  }

  std::string convertPaymentId(const std::string& paymentIdString) override {
    return WalletGui::convertPaymentId(paymentIdString);
  }

  std::string extractPaymentId(const std::string& extra) override {
    return WalletGui::extractPaymentId(extra);
  }

  uint64_t getLastKnownBlockHeight() const override {
    return m_node.getLastKnownBlockHeight();
  }

  uint64_t getLastLocalBlockHeight() const override {
    return m_node.getLastLocalBlockHeight();
  }

  uint64_t getLastLocalBlockTimestamp() const override {
    return m_node.getLastLocalBlockTimestamp();
  }

  uint64_t getPeerCount() const override {
    return m_node.getPeerCount();
  }

  cn::IWalletLegacy* createWallet() override {
    return new cn::WalletLegacy(m_currency, m_node, m_loggerManager);
  }

private:
  INodeCallback& m_callback;
  const cn::Currency& m_currency;
  platform_system::Dispatcher m_dispatcher;
  logging::LoggerManager& m_loggerManager;
  cn::CoreConfig m_coreConfig;
  cn::NetNodeConfig m_netNodeConfig;
  cn::core m_core;
  cn::CryptoNoteProtocolHandler m_protocolHandler;
  cn::NodeServer m_nodeServer;
  cn::InProcessNode m_node;
  std::future<bool> m_nodeServerFuture;


  void peerCountUpdated(size_t count) {
    m_callback.peerCountUpdated(*this, count);
  }

  void localBlockchainUpdated(uint64_t height) {
    m_callback.localBlockchainUpdated(*this, height);
  }

  void lastKnownBlockHeightUpdated(uint64_t height) {
    m_callback.lastKnownBlockHeightUpdated(*this, height);
  }
};

Node* createRpcNode(const cn::Currency& currency, logging::LoggerManager& logManager, INodeCallback& callback, const std::string& nodeHost, unsigned short nodePort) {
  return new RpcNode(currency, logManager, callback, nodeHost, nodePort);
}

Node* createInprocessNode(const cn::Currency& currency, logging::LoggerManager& logManager,
  const cn::CoreConfig& coreConfig, const cn::NetNodeConfig& netNodeConfig, INodeCallback& callback) {
  return new InprocessNode(currency, logManager, coreConfig, netNodeConfig, callback);
}

}
