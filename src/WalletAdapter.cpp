// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletAdapter.h"
#include "LoggerAdapter.h"

#include <CryptoNoteCore/Account.h>
#include <CryptoNoteProtocol/CryptoNoteProtocolHandler.h>
#include <Mnemonics/electrum-words.h>
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
  m_sentTransactionId(cn::WALLET_LEGACY_INVALID_TRANSACTION_ID), m_sentMessageId(cn::WALLET_LEGACY_INVALID_TRANSACTION_ID),
  m_depositId(cn::WALLET_LEGACY_INVALID_TRANSACTION_ID), m_depositWithdrawalId(cn::WALLET_LEGACY_INVALID_TRANSACTION_ID) {
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
    return m_wallet == nullptr ? QString() : QString::fromStdString(m_wallet->getAddress());
  } catch (std::system_error&) {
    return QString();
  }
}

quint64 WalletAdapter::getActualBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->actualBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getPendingBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->pendingBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getActualDepositBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->actualDepositBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getActualInvestmentBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->actualInvestmentBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

/* Get the current maximum we can send because of dust outputs without optimizing the wallet */
quint64 WalletAdapter::getWalletMaximum() const
{
  try
  {
    return m_wallet == nullptr ? 0 : m_wallet->getWalletMaximum();
  }
  catch (std::system_error&)
  {
    return 0;
  }
}

quint64 WalletAdapter::getPendingInvestmentBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->pendingInvestmentBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getPendingDepositBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->pendingDepositBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

void WalletAdapter::open(const QString& _password) {

  Q_ASSERT(m_wallet == nullptr);
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

    if (openFile(Settings::instance().getWalletFile(), true)) {
      try {
        m_wallet->initAndLoad(m_file, _password.toStdString());
      } catch (std::system_error&) {
        closeFile();
        delete m_wallet;
        m_wallet = nullptr;
      }
    }
  } else {}
}

void WalletAdapter::createWallet() {

  Q_ASSERT(m_wallet == nullptr);
  Settings::instance().setEncrypted(false);
  Q_EMIT walletStateChangedSignal(tr("Creating wallet"), "");

  m_wallet = NodeAdapter::instance().createWallet();

  try {
    m_wallet->initAndGenerate("");
  } catch (std::system_error&) {
    delete m_wallet;
    m_wallet = nullptr;
  }
}

void WalletAdapter::addObserver()
{
  Q_CHECK_PTR(m_wallet);
  m_wallet->addObserver(this);
}

void WalletAdapter::createWithKeys(const cn::AccountKeys& _keys) {
    m_wallet = NodeAdapter::instance().createWallet();
    m_wallet->addObserver(this);
    Settings::instance().setEncrypted(false);
    Q_EMIT walletStateChangedSignal(tr("Importing keys"),"");
    m_wallet->initWithKeys(_keys, "");
}

bool WalletAdapter::isOpen() const {
  return m_wallet != nullptr;
}

bool WalletAdapter::importLegacyWallet(const QString &_password) {
  QString fileName = Settings::instance().getWalletFile();
  Settings::instance().setEncrypted(!_password.isEmpty());
  try {
    fileName.replace(fileName.lastIndexOf(".keys"), 5, ".wallet");
    if (!openFile(fileName, false)) {
      delete m_wallet;
      m_wallet = nullptr;
      return false;
    }

    cn::importLegacyKeys(Settings::instance().getWalletFile().toStdString(), _password.toStdString(), m_file);
    closeFile();
    Settings::instance().setWalletFile(fileName);
    return true;
  } catch (std::system_error& _err) {
    closeFile();
    if (_err.code().value() == cn::error::WRONG_PASSWORD) {
      Settings::instance().setEncrypted(true);
      Q_EMIT openWalletWithPasswordSignal(!_password.isEmpty());
    }
  } catch (std::runtime_error&) {
    closeFile();
  }

  delete m_wallet;
  m_wallet = nullptr;
  return false;
}

void WalletAdapter::close() {
  Q_CHECK_PTR(m_wallet);
  save(true, true);
  lock();
  m_wallet->removeObserver(this);
  m_isSynchronized = false;
  m_newTransactionsNotificationTimer.stop();
  m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  Q_EMIT walletCloseCompletedSignal();
  QCoreApplication::processEvents();
  delete m_wallet;
  m_wallet = nullptr;
  unlock();
}

bool WalletAdapter::save(bool _details, bool _cache) {
  return save(Settings::instance().getWalletFile() + ".temp", _details, _cache);
}

bool WalletAdapter::save(const QString& _file, bool _details, bool _cache) {
  Q_CHECK_PTR(m_wallet);
  if (openFile(_file, false)) {
    try {
      m_wallet->save(m_file, _details, _cache);
    } catch (std::system_error&) {
      closeFile();
      return false;
    }
    Q_EMIT walletStateChangedSignal(tr("Saving data"),"");
  } else {
    return false;
  }

  return true;
}

void WalletAdapter::backup(const QString& _file) {
  if (save(_file.endsWith(".wallet") ? _file : _file + ".wallet", true, false)) {
    m_isBackupInProgress = true;
  }
}

void WalletAdapter::reset() {
  Q_CHECK_PTR(m_wallet);
  save(false, false);
  lock();
  m_wallet->removeObserver(this);
  m_isSynchronized = false;
  m_newTransactionsNotificationTimer.stop();
  m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  Q_EMIT walletCloseCompletedSignal();
  QCoreApplication::processEvents();
  delete m_wallet;
  m_wallet = nullptr;
  unlock();
}

quint64 WalletAdapter::getTransactionCount() const 
{
  Q_CHECK_PTR(m_wallet);
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
  Q_CHECK_PTR(m_wallet);
  try 
  {
    return m_wallet->getTransferCount();
  } 
  catch (std::system_error&) 
  {
    return 0;
  }
}

quint64 WalletAdapter::getDepositCount() const 
{
  Q_CHECK_PTR(m_wallet);
  try 
  {
    return m_wallet->getDepositCount();
  } 
  catch (std::system_error&) 
  {
    return 0;
  }
}

bool WalletAdapter::getTransaction(cn::TransactionId _id, cn::WalletLegacyTransaction& _transaction) 
{
  Q_CHECK_PTR(m_wallet);
  try 
  {
    return m_wallet->getTransaction(_id, _transaction);
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getTransfer(cn::TransferId _id, cn::WalletLegacyTransfer& _transfer) 
{
  Q_CHECK_PTR(m_wallet);
  try 
  {
    return m_wallet->getTransfer(_id, _transfer);
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getDeposit(cn::DepositId _id, cn::Deposit& _deposit) {
  Q_CHECK_PTR(m_wallet);
  try 
  {
    return m_wallet->getDeposit(_id, _deposit);
  } 
  catch (std::system_error&) 
  {
    return false;
  }
}

bool WalletAdapter::getAccountKeys(cn::AccountKeys& _keys) 
{
  Q_CHECK_PTR(m_wallet);
  try 
  {
    m_wallet->getAccountKeys(_keys);
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
    crypto::electrum_words::bytes_to_words(keys.spendSecretKey, _seed, "English");
    return true;
  }
  else {
    _seed = "Your wallet does not support the use of a mnemonic seed. Please create a new wallet.";
    return false;
  }
}

void WalletAdapter::sendTransaction(QVector<cn::WalletLegacyTransfer>& _transfers,
                                    quint64 _fee,
                                    const QString& _paymentId,
                                    const QVector<cn::TransactionMessage>& _messages,
                                    quint64 _mixin)
{
  Q_CHECK_PTR(m_wallet);
  try
  {
    LoggerAdapter::instance().log("lock");
    lock();
    LoggerAdapter::instance().log("locked");
    crypto::SecretKey _transactionsk;
    std::vector<cn::WalletLegacyTransfer> transfers = _transfers.toStdVector();
    LoggerAdapter::instance().log("Sending transaction to WalletLegacy");
    m_sentTransactionId =
        m_wallet->sendTransaction(_transactionsk,
                                  transfers,
                                  _fee,
                                  NodeAdapter::instance().convertPaymentId(_paymentId),
                                  _mixin,
                                  0,
                                  _messages.toStdVector());
    Q_EMIT walletStateChangedSignal(tr("Sending transaction"), "");
    LoggerAdapter::instance().log("Transaction sent by WalletLegacy");
  }
  catch (std::system_error&)
  {
    unlock();
    LoggerAdapter::instance().log("unlocked");
  }
}

quint64 WalletAdapter::getNumUnlockedOutputs() const {
  Q_CHECK_PTR(m_wallet);
  return m_wallet->getNumUnlockedOutputs();
}  

void WalletAdapter::optimizeWallet() {
  Q_CHECK_PTR(m_wallet);
  std::vector<cn::WalletLegacyTransfer> transfers;
  std::vector<cn::TransactionMessage> messages;
  std::string extraString;
  uint64_t fee = cn::parameters::MINIMUM_FEE;
  uint64_t mixIn = 0;
  uint64_t unlockTimestamp = 0;
  uint64_t ttl = 0;
  crypto::SecretKey transactionSK;
  try {
    lock();
    m_sentTransactionId = m_wallet->sendTransaction(transactionSK, transfers, fee, extraString, mixIn, unlockTimestamp, messages, ttl);
    Q_EMIT walletStateChangedSignal(tr("Optimizing wallet"), "");
  } catch (std::system_error&) {
    unlock();
  }
}

void WalletAdapter::sendMessage(QVector<cn::WalletLegacyTransfer>& _transfers,
                                quint64 _fee,
                                const QVector<cn::TransactionMessage>& _messages,
                                quint64 _ttl,
                                quint64 _mixin)
{
  Q_CHECK_PTR(m_wallet);
  crypto::SecretKey _transactionsk;
  try
  {
    lock();
    std::vector<cn::WalletLegacyTransfer> transfers = _transfers.toStdVector();
    m_sentMessageId = m_wallet->sendTransaction(
        _transactionsk, transfers, _fee, "", _mixin, 0, _messages.toStdVector(), _ttl);
    Q_EMIT walletStateChangedSignal(tr("Sending message"), "");
  }
  catch (std::system_error&)
  {
    unlock();
  }
}

void WalletAdapter::deposit(quint32 _term, quint64 _amount, quint64 _fee, quint64 _mixIn)
{
  Q_CHECK_PTR(m_wallet);
  try
  {
    lock();
    m_depositId = m_wallet->deposit(_term, _amount, _fee, _mixIn);
    Q_EMIT walletStateChangedSignal(tr("Creating deposit"), "");
  }
  catch (std::system_error&)
  {
    unlock();
  }
}

void WalletAdapter::withdrawUnlockedDeposits(QVector<cn::DepositId> _depositIds, quint64 _fee) {
  Q_CHECK_PTR(m_wallet);
  try {
    lock();
    m_depositWithdrawalId = m_wallet->withdrawDeposits(_depositIds.toStdVector(), _fee);
    Q_EMIT walletStateChangedSignal(tr("Withdrawing deposit"), "");
  } catch (std::system_error&) {
    unlock();
  }
}

bool WalletAdapter::changePassword(const QString& _oldPassword, const QString& _newPassword) {
  Q_CHECK_PTR(m_wallet);
  try {
    if (m_wallet->changePassword(_oldPassword.toStdString(), _newPassword.toStdString()).value() == cn::error::WRONG_PASSWORD) {
      return false;
    }
  } catch (std::system_error&) {
    return false;
  }

  Settings::instance().setEncrypted(!_newPassword.isEmpty());
  return save(true, true);
}

void WalletAdapter::setWalletFile(const QString& _path) {
  Q_ASSERT(m_wallet == nullptr);
  Settings::instance().setWalletFile(_path);
}

void WalletAdapter::initCompleted(std::error_code _error) {
  if (m_file.is_open()) {
    closeFile();
  }

  Q_EMIT walletInitCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
}

void WalletAdapter::onWalletInitCompleted(int _error, const QString& _errorText) {
  switch(_error) {
  case 0: {
    Q_EMIT walletActualBalanceUpdatedSignal(m_wallet->actualBalance());
    Q_EMIT walletPendingBalanceUpdatedSignal(m_wallet->pendingBalance());
    Q_EMIT walletActualDepositBalanceUpdatedSignal(m_wallet->actualDepositBalance());
    Q_EMIT walletPendingDepositBalanceUpdatedSignal(m_wallet->pendingDepositBalance());
    Q_EMIT walletActualInvestmentBalanceUpdatedSignal(m_wallet->actualInvestmentBalance());
    Q_EMIT walletPendingInvestmentBalanceUpdatedSignal(m_wallet->pendingInvestmentBalance());    
    Q_EMIT updateWalletAddressSignal(QString::fromStdString(m_wallet->getAddress()));
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
    delete m_wallet;
    m_wallet = nullptr;
    break;
  default: {
    delete m_wallet;
    m_wallet = nullptr;
    break;
  }
  }
}

void WalletAdapter::saveCompleted(std::error_code _error) {
  if (!_error && !m_isBackupInProgress) {
    closeFile();
    renameFile(Settings::instance().getWalletFile() + ".temp", Settings::instance().getWalletFile());
    Q_EMIT walletStateChangedSignal(tr("Ready"),"");
    Q_EMIT updateBlockStatusTextWithDelaySignal();
  } else if (m_isBackupInProgress) {
    m_isBackupInProgress = false;
    closeFile();
  } else {
    closeFile();
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
  unlock();
  Q_EMIT walletSendTransactionCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
  if (_transactionId == m_sentTransactionId) {
    m_sentTransactionId = cn::WALLET_LEGACY_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_sentMessageId) {
    Q_EMIT walletSendMessageCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_sentMessageId = cn::WALLET_LEGACY_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_depositId) {
    Q_EMIT walletCreateDepositCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_depositId = cn::WALLET_LEGACY_INVALID_TRANSACTION_ID;
  } else if (_transactionId == m_depositWithdrawalId) {
    Q_EMIT walletWithdrawDepositCompletedSignal(_transactionId, _error.value(), QString::fromStdString(_error.message()));
    m_depositWithdrawalId = cn::WALLET_LEGACY_INVALID_TRANSACTION_ID;
  }

  Q_EMIT updateBlockStatusTextWithDelaySignal();
}

void WalletAdapter::onWalletSendTransactionCompleted(cn::TransactionId _transactionId, int _error, const QString& _errorText) {
  cn::WalletLegacyTransaction transaction;
  if (!this->getTransaction(_transactionId, transaction)) {
    return;
  }

  Q_EMIT walletTransactionCreatedSignal(_transactionId);

  save(true, true);
}

void WalletAdapter::transactionUpdated(cn::TransactionId _transactionId) {
  Q_EMIT walletTransactionUpdatedSignal(_transactionId);
}

void WalletAdapter::depositsUpdated(const std::vector<cn::DepositId>& _depositIds) {
  Q_EMIT walletDepositsUpdatedSignal(QVector<cn::DepositId>::fromStdVector(_depositIds));
}

void WalletAdapter::lock() {
  m_mutex.lock();
}

void WalletAdapter::unlock() {
  m_mutex.unlock();
}

bool WalletAdapter::openFile(const QString& _file, bool _readOnly)
{
  lock();
#ifdef Q_OS_WIN
  const wchar_t* cwc = reinterpret_cast<const wchar_t*>(_file.utf16());
  m_file.open(cwc,
              std::ios::binary | (_readOnly ? std::ios::in : (std::ios::out | std::ios::trunc)));
#else
  m_file.open(_file.toStdString(),
              std::ios::binary | (_readOnly ? std::ios::in : (std::ios::out | std::ios::trunc)));
#endif
  if (!m_file.is_open())
  {
    unlock();
  }

  return m_file.is_open();
}

void WalletAdapter::closeFile() {
  m_file.close();
  unlock();
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
  if (m_wallet == nullptr) {
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
  Q_ASSERT(m_wallet != nullptr);
  if (Settings::instance().getWalletFile().endsWith(".wallet")) {
    if (openFile(Settings::instance().getWalletFile(), true)) {
      try {
        if (m_wallet->checkWalletPassword(m_file, _password.toStdString())) {
          closeFile();
          return true;
        }
        else {
          closeFile();
          return false;
        }
      }
      catch (std::system_error&) {
        closeFile();
        return false;
      }
    }
  }
  return false;
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
