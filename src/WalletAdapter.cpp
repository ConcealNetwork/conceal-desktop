// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletAdapter.h"
#include "LoggerAdapter.h"

#include <CryptoNoteCore/Account.h>
#include <CryptoNoteCore/TransactionExtra.h>
#include <CryptoNoteProtocol/CryptoNoteProtocolHandler.h>
#include <Mnemonics/Mnemonics.h>
#include <Wallet/LegacyKeysImporter.h>
#include <Wallet/WalletErrors.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QVector>

#include "NodeAdapter.h"
#include "Settings.h"

namespace WalletGui {

const quint32 MSECS_IN_HOUR = 60 * 60 * 1000;
const quint32 MSECS_IN_MINUTE = 60 * 1000;

const quint32 LAST_BLOCK_INFO_UPDATING_INTERVAL = 1 * MSECS_IN_MINUTE;
const quint32 LAST_BLOCK_INFO_WARNING_INTERVAL = 1 * MSECS_IN_HOUR;

WalletAdapter& WalletAdapter::instance() {
  static WalletAdapter inst;
  return inst;
}

WalletAdapter::WalletAdapter() : QObject(), m_wallet(nullptr), m_mutex(), m_isBackupInProgress(false),
  m_isSynchronized(false), m_newTransactionsNotificationTimer(),
  m_lastWalletTransactionId(std::numeric_limits<quint64>::max()),
  m_sentTransactionId(cn::WALLET_INVALID_TRANSACTION_ID), m_sentMessageId(cn::WALLET_INVALID_TRANSACTION_ID),
  m_depositId(cn::WALLET_INVALID_TRANSACTION_ID), m_depositWithdrawalId(cn::WALLET_INVALID_TRANSACTION_ID) {
  connect(this, &WalletAdapter::walletInitCompletedSignal, this, &WalletAdapter::onWalletInitCompleted, Qt::QueuedConnection);
  connect(this, &WalletAdapter::walletSendTransactionCompletedSignal, this, &WalletAdapter::onWalletSendTransactionCompleted, Qt::QueuedConnection);
  connect(this, &WalletAdapter::updateBlockStatusTextSignal, this, &WalletAdapter::updateBlockStatusText, Qt::QueuedConnection);
  connect(this, &WalletAdapter::updateBlockStatusTextWithDelaySignal, this, &WalletAdapter::updateBlockStatusTextWithDelay, Qt::QueuedConnection);
  connect(&m_newTransactionsNotificationTimer, &QTimer::timeout, this, &WalletAdapter::notifyAboutLastTransaction);
  connect(this, &WalletAdapter::walletSynchronizationProgressUpdatedSignal, this, [&]() {
    if (!m_newTransactionsNotificationTimer.isActive()) {
      m_newTransactionsNotificationTimer.start();
    }
  }, Qt::QueuedConnection);

  connect(this, &WalletAdapter::walletSynchronizationCompletedSignal, this, [&]() {
    m_newTransactionsNotificationTimer.stop();
    notifyAboutLastTransaction();
  }, Qt::QueuedConnection);

  m_newTransactionsNotificationTimer.setInterval(500);
}

WalletAdapter::~WalletAdapter() {
}

QString WalletAdapter::getAddress() const {
  try {
    return m_wallet ? QString::fromStdString(m_wallet->getAddress(0)) : QString();
  } catch (std::system_error&) {
    return QString();
  }
}

quint64 WalletAdapter::getActualBalance() const {
  try {
    return m_wallet ? m_wallet->getActualBalance() : 0;
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getPendingBalance() const {
  try {
    return m_wallet ? m_wallet->getPendingBalance() : 0;
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getActualDepositBalance() const {
  try {
    return m_wallet ? m_wallet->getUnlockedDepositBalance() : 0;
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getActualInvestmentBalance() const {
  return 0;
}

/* Get the current maximum we can send because of dust outputs without optimizing the wallet */
quint64 WalletAdapter::getWalletMaximum() const
{
  try
  {
    return m_wallet ? m_wallet->getActualBalance() : 0;
  }
  catch (std::system_error&)
  {
    return 0;
  }
}

quint64 WalletAdapter::getPendingInvestmentBalance() const {
  return 0;
}

quint64 WalletAdapter::getPendingDepositBalance() const {
  try {
    return m_wallet ? m_wallet->getLockedDepositBalance() : 0;
  } catch (std::system_error&) {
    return 0;
  }
}

void WalletAdapter::open(const QString& _password) {
  Settings::instance().setEncrypted(!_password.isEmpty());
  Q_EMIT walletStateChangedSignal(tr("Opening wallet"),"");

  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);

  if (QFile::exists(Settings::instance().getWalletFile())) {
    if (Settings::instance().getWalletFile().endsWith(".keys")) {
      if(!importLegacyWallet(_password)) {
        return;
      }
    }

    try {
      m_wallet->load(Settings::instance().getWalletFile().toStdString(), _password.toStdString());
      LoggerAdapter::instance().log("loaded");
    } catch (std::system_error&) {
      Q_EMIT openWalletWithPasswordSignal(!_password.isEmpty());
    }
  }
}

void WalletAdapter::createWallet() {
  Settings::instance().setEncrypted(false);
  Q_EMIT walletStateChangedSignal(tr("Creating wallet"), "");

  m_wallet = NodeAdapter::instance().createWallet();

  try {
    crypto::SecretKey private_view_key;
    cn::KeyPair spendKey;

    crypto::generate_keys(spendKey.publicKey, spendKey.secretKey);

    crypto::PublicKey unused;

    cn::AccountBase::generateViewFromSpend(spendKey.secretKey, private_view_key, unused);

    m_wallet->initializeWithViewKey(Settings::instance().getWalletFile().toStdString(), "", private_view_key);
    m_wallet->createAddress(spendKey.secretKey);
    m_wallet->save(cn::WalletSaveLevel::SAVE_KEYS_ONLY);
  } catch (std::system_error&) {
    m_wallet.reset();
  }
}

void WalletAdapter::addObserver()
{
  m_wallet->addObserver(this);
}

void WalletAdapter::createWithKeys(const cn::AccountKeys& _keys) {
    m_wallet = NodeAdapter::instance().createWallet();
    Settings::instance().setEncrypted(false);
    Q_EMIT walletStateChangedSignal(tr("Importing keys"),"");
    m_wallet->initializeWithViewKey(Settings::instance().getWalletFile().toStdString(), "", _keys.viewSecretKey);
    if (_keys.spendSecretKey != cn::NULL_SECRET_KEY) {
      m_wallet->createAddress(_keys.spendSecretKey);
    } else {
      m_wallet->createAddress(_keys.address.spendPublicKey);
    }
    m_wallet->reset(0);
    addObserver();
}

bool WalletAdapter::isOpen() const {
  return m_wallet.get() != nullptr;
}

bool WalletAdapter::importLegacyWallet(const QString &_password) {
  QString fileName = Settings::instance().getWalletFile();
  Settings::instance().setEncrypted(!_password.isEmpty());
  try {
    fileName.replace(fileName.lastIndexOf(".keys"), 5, ".wallet");
    
      m_wallet.reset();
      return false;
    Settings::instance().setWalletFile(fileName);
    return true;
  } catch (std::system_error& _err) {
    if (_err.code().value() == cn::error::WRONG_PASSWORD) {
      Settings::instance().setEncrypted(true);
      Q_EMIT openWalletWithPasswordSignal(!_password.isEmpty());
    }
  } catch (std::runtime_error&) {
  }

  m_wallet.reset();
  return false;
}

void WalletAdapter::close() {
  QMutexLocker locker(&m_mutex);
  save(true, true);
  m_wallet->removeObserver(this);
  m_isSynchronized = false;
  m_newTransactionsNotificationTimer.stop();
  m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  Q_EMIT walletCloseCompletedSignal();
  QCoreApplication::processEvents();
  m_wallet.reset();
}

bool WalletAdapter::save(bool _details, bool _cache) {
  return save(Settings::instance().getWalletFile() + ".temp", _details, _cache);
}

bool WalletAdapter::save(const QString& _file, bool _details, bool _cache) {
  try {
    cn::WalletSaveLevel level = _details ? cn::WalletSaveLevel::SAVE_ALL : cn::WalletSaveLevel::SAVE_KEYS_ONLY;
    m_wallet->save(level);
  } catch (std::system_error&) {
    return false;
  }
  Q_EMIT walletStateChangedSignal(tr("Saving data"), "");

  return true;
}

void WalletAdapter::backup(const QString& _file) {
  m_wallet->exportWallet(_file.toStdString(), cn::WalletSaveLevel::SAVE_ALL);
}

void WalletAdapter::reset() {
  QMutexLocker locker(&m_mutex);
  m_wallet->reset(0);
}

quint64 WalletAdapter::getTransactionCount() const 
{
  try 
  {
    return m_wallet->getTransactionCount();
  } 
  catch (std::system_error&) 
  {
    return 0;
  }
}

quint64 WalletAdapter::getTransferCount() const 
{
  try 
  {
    return 0;
  } 
  catch (std::system_error&) 
  {
    return 0;
  }
}

quint64 WalletAdapter::getDepositCount() const 
{
  try 
  {
    return m_wallet->getWalletDepositCount();
  } 
  catch (std::system_error&) 
  {
    return 0;
  }
}

bool WalletAdapter::getTransaction(cn::TransactionId _id, cn::WalletTransaction& _transaction) const
{
  try 
  {
    _transaction= m_wallet->getTransaction(_id);
    return true;
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getTransfer(size_t transactionIndex, size_t transferIndex, cn::WalletTransfer& transfer) const
{
  try 
  {
    transfer = m_wallet->getTransactionTransfer(transactionIndex, transferIndex);
    return true;
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getDeposit(cn::DepositId _id, cn::Deposit& _deposit) {
  try 
  {
    _deposit = m_wallet->getDeposit(_id);
    return true;
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getAccountKeys(cn::AccountKeys& _keys) 
{
  try 
  {
    cn::KeyPair viewKey = m_wallet->getViewKey();
    cn::KeyPair spendKey = m_wallet->getAddressSpendKey(0);
    _keys.address.spendPublicKey = spendKey.publicKey;
    _keys.address.viewPublicKey = viewKey.publicKey;
    _keys.spendSecretKey = spendKey.secretKey;
    _keys.viewSecretKey = viewKey.secretKey;
    return true;
  } 
  catch (std::system_error&) 
  {
    return false;    
  }
}

bool WalletAdapter::getMnemonicSeed(std::string& _seed)
{
  cn::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);

  /* check if the wallet is deterministic
     generate a view key from the spend key and them compare it to the existing view key */
  crypto::PublicKey unused_dummy_variable;
  crypto::SecretKey deterministic_private_view_key;
  cn::AccountBase::generateViewFromSpend(
      keys.spendSecretKey, deterministic_private_view_key, unused_dummy_variable);
  bool deterministic_private_keys = deterministic_private_view_key == keys.viewSecretKey;

  if (deterministic_private_keys) {
    _seed = mnemonics::privateKeyToMnemonic(keys.spendSecretKey);
    return true;
  }
  else {
    _seed = "Your wallet does not support the use of a mnemonic seed. Please create a new wallet.";
    return false;
  }
}

void WalletAdapter::sendTransaction(QVector<cn::WalletOrder>& _transfers,
                                    quint64 _fee,
                                    const QString& _paymentId,
                                    const QVector<cn::WalletMessage>& _messages,
                                    quint64 _mixin)
{
  QMutexLocker locker(&m_mutex);
  try
  {
    crypto::SecretKey _transactionsk;
    std::vector<cn::WalletOrder> transfers = _transfers.toStdVector();
    LoggerAdapter::instance().log("Sending transaction to WalletGreen");
    cn::TransactionParameters sendParams;
    sendParams.destinations = transfers;
    sendParams.messages = _messages.toStdVector();
    sendParams.unlockTimestamp = 0;
    sendParams.changeDestination = m_wallet->getAddress(0);

    if (!_paymentId.isEmpty()) {
      cn::addPaymentIdToExtra(_paymentId.toStdString(), sendParams.extra);
    }

    m_sentTransactionId = m_wallet->transfer(sendParams, _transactionsk);
    Q_EMIT walletStateChangedSignal(tr("Sending transaction"), "");
    LoggerAdapter::instance().log("Transaction sent by WalletGreen");
  }
  catch (std::system_error&)
  {
  }
}

quint64 WalletAdapter::getNumUnlockedOutputs() const {
  return m_wallet->getUnspentOutputsCount();
}

quint64 WalletAdapter::getTransferCount(cn::TransactionId id) const {
  return m_wallet->getTransactionTransferCount(id);
}

void WalletAdapter::optimizeWallet() {
  QMutexLocker locker(&m_mutex);
  try {
    m_sentTransactionId = m_wallet->createOptimizationTransaction(m_wallet->getAddress(0));
    Q_EMIT walletStateChangedSignal(tr("Optimizing wallet"), "");
  } catch (std::system_error&) {
  }
}

void WalletAdapter::sendMessage(QVector<cn::WalletOrder>& _transfers,
                                quint64 _fee,
                                const QVector<cn::WalletMessage>& _messages,
                                quint64 _ttl,
                                quint64 _mixin)
{
  QMutexLocker locker(&m_mutex);
  crypto::SecretKey _transactionsk;
  try
  {
    std::vector<cn::WalletOrder> transfers = _transfers.toStdVector();
    cn::TransactionParameters sendParams;
    sendParams.destinations = transfers;
    sendParams.messages = _messages.toStdVector();
    sendParams.unlockTimestamp = 0;
    sendParams.changeDestination = m_wallet->getAddress(0);
    m_sentMessageId = m_wallet->transfer(sendParams, _transactionsk);
    Q_EMIT walletStateChangedSignal(tr("Sending message"), "");
  }
  catch (std::system_error&)
  {
  }
}

void WalletAdapter::deposit(quint32 _term, quint64 _amount, quint64 _fee, quint64 _mixIn)
{
  QMutexLocker locker(&m_mutex);
  try
  {
    std::string address = m_wallet->getAddress(0);
    std::string tx_hash;
    m_wallet->createDeposit(_amount, _term, address, address, tx_hash);
    Q_EMIT walletStateChangedSignal(tr("Creating deposit"), "");
  }
  catch (std::system_error&)
  {
  }
}

void WalletAdapter::withdrawUnlockedDeposits(QVector<cn::DepositId> _depositIds, quint64 _fee) {
  QMutexLocker locker(&m_mutex);
  try {
    std::string tx_hash;
    m_wallet->withdrawDeposit(_depositIds.toStdVector()[0], tx_hash);
    Q_EMIT walletStateChangedSignal(tr("Withdrawing deposit"), "");
  } catch (std::system_error&) {
  }
}

bool WalletAdapter::changePassword(const QString& _oldPassword, const QString& _newPassword) {
  try {
    m_wallet->changePassword(_oldPassword.toStdString(), _newPassword.toStdString());
    return true;
  } catch (std::system_error&) {
    return false;
  }

  Settings::instance().setEncrypted(!_newPassword.isEmpty());
  return save(true, true);
}

void WalletAdapter::setWalletFile(const QString& _path) {
  Settings::instance().setWalletFile(_path);
}

void WalletAdapter::initCompleted(std::error_code _error) {
  Q_EMIT walletInitCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
}

void WalletAdapter::onWalletInitCompleted(int _error, const QString& _errorText) {
  switch(_error) {
  case 0: {
    Q_EMIT walletActualBalanceUpdatedSignal(m_wallet->getActualBalance());
    Q_EMIT walletPendingBalanceUpdatedSignal(m_wallet->getPendingBalance());
    Q_EMIT walletActualDepositBalanceUpdatedSignal(m_wallet->getLockedDepositBalance());
    Q_EMIT walletPendingDepositBalanceUpdatedSignal(m_wallet->getUnlockedDepositBalance());
    Q_EMIT walletActualInvestmentBalanceUpdatedSignal(0);
    Q_EMIT walletPendingInvestmentBalanceUpdatedSignal(0);    
    Q_EMIT updateWalletAddressSignal(QString::fromStdString(m_wallet->getAddress(0)));
    Q_EMIT reloadWalletTransactionsSignal();
    Q_EMIT walletStateChangedSignal(tr("Ready"),"");
    QTimer::singleShot(5000, this, SLOT(updateBlockStatusText()));
    if (!QFile::exists(Settings::instance().getWalletFile())) {
      save(true, true);
    }

    break;
  }
  case cn::error::WRONG_PASSWORD:
    Q_EMIT openWalletWithPasswordSignal(Settings::instance().isEncrypted());
    Settings::instance().setEncrypted(true);
    m_wallet.reset();
    break;
  default: {
    m_wallet.reset();
    break;
  }
  }
}

void WalletAdapter::saveCompleted(std::error_code _error) {
  if (!_error && !m_isBackupInProgress) {
    Q_EMIT walletStateChangedSignal(tr("Ready"),"");
    Q_EMIT updateBlockStatusTextWithDelaySignal();
  } else if (m_isBackupInProgress) {
    m_isBackupInProgress = false;
  }

  Q_EMIT walletSaveCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
}

void WalletAdapter::synchronizationProgressUpdated(uint32_t _current, uint32_t _total) {
  m_isSynchronized = false;
  qreal syncedPercentage = (static_cast<qreal>(_current)) / _total;
  QString status = QString("%1 (%2%)").arg(tr("Synchronizing")).arg(QString::number(syncedPercentage * 100, 'f', 2));
  QString height = QString("%1/%2").arg(QString::number(_current, 'f', 0)).arg(QString::number(_total, 'f', 0));
  Q_EMIT walletStateChangedSignal(status,height);
  Q_EMIT walletSynchronizationProgressUpdatedSignal(_current, _total);
}

void WalletAdapter::synchronizationCompleted(std::error_code _error) {
  if (!_error) {
    m_isSynchronized = true;
    Q_EMIT updateBlockStatusTextSignal();
    Q_EMIT walletSynchronizationCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
  }
}

void WalletAdapter::actualBalanceUpdated(uint64_t _actual_balance) {
  Q_EMIT walletActualBalanceUpdatedSignal(_actual_balance);
}

void WalletAdapter::pendingBalanceUpdated(uint64_t _pending_balance) {
  Q_EMIT walletPendingBalanceUpdatedSignal(_pending_balance);
}

void WalletAdapter::actualDepositBalanceUpdated(uint64_t _actualDepositBalance) {
Q_EMIT walletActualDepositBalanceUpdatedSignal(_actualDepositBalance);
}

void WalletAdapter::pendingDepositBalanceUpdated(uint64_t _pendingDepositBalance) {
  Q_EMIT walletPendingDepositBalanceUpdatedSignal(_pendingDepositBalance);
}

void WalletAdapter::actualInvestmentBalanceUpdated(uint64_t _actualInvestmentBalance) {
Q_EMIT walletActualInvestmentBalanceUpdatedSignal(_actualInvestmentBalance);
}

void WalletAdapter::pendingInvestmentBalanceUpdated(uint64_t _pendingInvestmentBalance) {
  Q_EMIT walletPendingInvestmentBalanceUpdatedSignal(_pendingInvestmentBalance);
}


void WalletAdapter::externalTransactionCreated(cn::TransactionId _transactionId) {
  if (!m_isSynchronized) {
    m_lastWalletTransactionId = _transactionId;
  } else {
    Q_EMIT walletTransactionCreatedSignal(_transactionId);
  }
}

void WalletAdapter::sendTransactionCompleted(cn::TransactionId _transactionId, std::error_code _error) {
  Q_ASSERT(_transactionId == m_sentTransactionId || _transactionId == m_sentMessageId ||
    _transactionId == m_depositId || _transactionId == m_depositWithdrawalId);
  Q_EMIT walletSendTransactionCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
  if (_transactionId == m_sentTransactionId) {
    m_sentTransactionId = cn::WALLET_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_sentMessageId) {
    Q_EMIT walletSendMessageCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_sentMessageId = cn::WALLET_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_depositId) {
    Q_EMIT walletCreateDepositCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_depositId = cn::WALLET_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_depositWithdrawalId) {
    Q_EMIT walletWithdrawDepositCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_depositWithdrawalId = cn::WALLET_INVALID_TRANSACTION_ID;
  }

  Q_EMIT updateBlockStatusTextWithDelaySignal();
}

void WalletAdapter::onWalletSendTransactionCompleted(cn::TransactionId _transactionId, int _error, const QString& _errorText) {
  cn::WalletTransaction transaction;
  if (!this->getTransaction(_transactionId, transaction)) {
    return;
  }

  Q_EMIT walletTransactionCreatedSignal(_transactionId);

  save(true, true);
}

void WalletAdapter::transactionUpdated(cn::TransactionId _transactionId) {
  Q_EMIT walletTransactionUpdatedSignal(_transactionId);
}

void WalletAdapter::depositUpdated(cn::DepositId depositId) {
  Q_EMIT walletDepositsUpdatedSignal(QVector<cn::DepositId>(1, depositId));
}

void WalletAdapter::depositsUpdated(const std::vector<cn::DepositId>& _depositIds) {
  Q_EMIT walletDepositsUpdatedSignal(QVector<cn::DepositId>::fromStdVector(_depositIds));
}

void WalletAdapter::notifyAboutLastTransaction() {
  if (m_lastWalletTransactionId != std::numeric_limits<quint64>::max()) {
    Q_EMIT walletTransactionCreatedSignal(m_lastWalletTransactionId);
    m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  }
}

void WalletAdapter::renameFile(const QString& _oldName, const QString& _newName) {
  Q_ASSERT(QFile::exists(_oldName));
  QFile::remove(_newName);
  QFile::rename(_oldName, _newName);
}

void WalletAdapter::updateBlockStatusText() {
  if (!m_wallet) {
    return;
  }

  QString walletSecurity = "";

  bool encrypted = Settings::instance().isEncrypted();
  if (!encrypted) {
    walletSecurity = tr("Unencrypted");
  } else
  {
    walletSecurity = tr("Encrypted");
  }

  const QDateTime currentTime = QDateTime::currentDateTimeUtc();
  const QDateTime blockTime = NodeAdapter::instance().getLastLocalBlockTimestamp();
  quint64 blockAge = blockTime.msecsTo(currentTime);
  const QString statusString = blockTime.msecsTo(currentTime) < LAST_BLOCK_INFO_WARNING_INTERVAL ? tr("Synchronized") : tr("Warning");
  const QString warningString = blockTime.msecsTo(currentTime) < LAST_BLOCK_INFO_WARNING_INTERVAL ? "" : QString("%1").arg(tr("There was a problem, please restart your wallet."));
  const QString blockHeightString = QString::number(NodeAdapter::instance().getLastLocalBlockHeight(),'f',0);

  Q_EMIT walletStateChangedSignal(statusString,blockHeightString);

  QTimer::singleShot(LAST_BLOCK_INFO_UPDATING_INTERVAL, this, SLOT(updateBlockStatusText()));
}

void WalletAdapter::updateBlockStatusTextWithDelay() {
  QTimer::singleShot(5000, this, SLOT(updateBlockStatusText()));
}

bool WalletAdapter::checkWalletPassword(const QString& _password) {
  try {
    std::string password = _password.toStdString();
    m_wallet->changePassword(password, password);
  } catch (std::system_error&) {
    return false;
  }
  return true;
}

crypto::SecretKey WalletAdapter::getTxKey(crypto::Hash& txid)
{
  return m_wallet->getTransactionDeterministicSecretKey(txid);
}

/* Check if the entered payment ID is valid */
bool WalletAdapter::isValidPaymentId(const QByteArray& _paymentIdString)
{
  if (_paymentIdString.isEmpty())
  {
    return true;
  }
  QByteArray paymentId = QByteArray::fromHex(_paymentIdString);
  return (paymentId.size() == sizeof(crypto::Hash)) &&
         (_paymentIdString.toUpper() == paymentId.toHex().toUpper());
}

}  // namespace WalletGui
