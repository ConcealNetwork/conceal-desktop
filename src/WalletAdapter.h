// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2017 Karbowanec developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <IWallet.h>
#include <System/Event.h>
#include <System/EventLock.h>

#include <QMutex>
#include <QObject>
#include <QTimer>
#include <atomic>
#include <fstream>
#include <memory>

namespace WalletGui {

class WalletAdapter : public QObject, public cn::IWalletObserver {
  Q_OBJECT
  Q_DISABLE_COPY(WalletAdapter)

public:
  static WalletAdapter& instance();

  void open(const QString& _password);
  void createWithKeys(const cn::AccountKeys& _keys);
  void close();
  bool save(bool _details, bool _cache);
  void backup(const QString& _file);
  void reset();
  void createWallet();
  void addObserver();

  QString getAddress() const;
  quint64 getActualBalance() const;
  quint64 getPendingBalance() const;
  quint64 getActualDepositBalance() const;
  quint64 getActualInvestmentBalance() const;  
  quint64 getPendingDepositBalance() const;
  quint64 getPendingInvestmentBalance() const;
  quint64 getWalletMaximum() const;
  quint64 getTransactionCount() const;
  quint64 getTransferCount() const;
  quint64 getDepositCount() const;
  quint64 getNumUnlockedOutputs() const;
  quint64 getTransferCount(cn::TransactionId id) const;
  bool getTransaction(cn::TransactionId _id, cn::WalletTransaction& _transaction) const;
  bool getTransfer(size_t transactionIndex, size_t transferIndex, cn::WalletTransfer& transfer) const;
  bool getDeposit(cn::DepositId _id, cn::Deposit& _deposit);
  bool getAccountKeys(cn::AccountKeys& _keys);
  bool getMnemonicSeed(std::string& _seed);
  bool isOpen() const;
  void sendTransaction(QVector<cn::WalletOrder>& _transfers,
                       quint64 _fee,
                       const QString& _payment_id,
                       const QVector<cn::WalletMessage>& _messages,
                       quint64 _mixin = cn::parameters::MINIMUM_MIXIN);
  void optimizeWallet();
  void sendMessage(QVector<cn::WalletOrder>& _transfers,
                   quint64 _fee,
                   const QVector<cn::WalletMessage>& _messages,
                   quint64 _ttl,
                   quint64 _mixin = cn::parameters::MINIMUM_MIXIN);
  void deposit(quint32 _term,
               quint64 _amount,
               quint64 _fee,
               quint64 _mixIn = cn::parameters::MINIMUM_MIXIN);
  void withdrawUnlockedDeposits(QVector<cn::DepositId> _depositIds, quint64 _fee);
  bool changePassword(const QString& _old_pass, const QString& _new_pass);
  void setWalletFile(const QString& _path);

  void initCompleted(std::error_code _result) override;
  void saveCompleted(std::error_code _result) override;
  void synchronizationProgressUpdated(uint32_t _current, uint32_t _total) override;
  void synchronizationCompleted(std::error_code _error) override;
  void actualBalanceUpdated(uint64_t _actualBalance) override;
  void pendingBalanceUpdated(uint64_t _pendingBalance) override;
  void actualDepositBalanceUpdated(uint64_t _actualDepositBalance) override;
  void pendingDepositBalanceUpdated(uint64_t _pendingDepositBalance) override;
  void actualInvestmentBalanceUpdated(uint64_t _actualDepositBalance) override;
  void pendingInvestmentBalanceUpdated(uint64_t _pendingDepositBalance) override;  
  void externalTransactionCreated(cn::TransactionId _transactionId) override;
  void sendTransactionCompleted(cn::TransactionId _transactionId, std::error_code _result) override;
  void transactionUpdated(cn::TransactionId _transactionId) override;
  void depositUpdated(cn::DepositId depositId) override;
  void depositsUpdated(const std::vector<cn::DepositId>& _depositIds) override;
  bool checkWalletPassword(const QString& _password);
  crypto::SecretKey getTxKey(crypto::Hash &txid);
  static bool isValidPaymentId(const QByteArray &_paymentIdString);
  
private:
  std::fstream m_file;
  std::unique_ptr<cn::IWallet> m_wallet;
  platform_system::Event m_readyEvent;
  QMutex m_mutex;
  std::atomic<bool> m_isBackupInProgress;
  std::atomic<bool> m_isSynchronized;
  std::atomic<quint64> m_lastWalletTransactionId;
  QTimer m_newTransactionsNotificationTimer;
  std::atomic<cn::TransactionId> m_sentTransactionId;
  std::atomic<cn::TransactionId> m_sentMessageId;
  std::atomic<cn::TransactionId> m_depositId;
  std::atomic<cn::TransactionId> m_depositWithdrawalId;


  WalletAdapter();
  ~WalletAdapter() override;

  void onWalletInitCompleted(int _error, const QString& _error_text);
  void onWalletSendTransactionCompleted(cn::TransactionId _transaction_id, int _error, const QString& _error_text);

  bool importLegacyWallet(const QString &_password);
  bool save(const QString& _file, bool _details, bool _cache);
  void notifyAboutLastTransaction();

  static void renameFile(const QString& _old_name, const QString& _new_name);
  Q_SLOT void updateBlockStatusText();
  Q_SLOT void updateBlockStatusTextWithDelay();

Q_SIGNALS:
  void walletInitCompletedSignal(int _error, const QString& _errorText);
  void walletCloseCompletedSignal();
  void walletSaveCompletedSignal(int _error, const QString& _errorText);
  void walletSynchronizationProgressUpdatedSignal(quint64 _current, quint64 _total);
  void walletSynchronizationCompletedSignal(int _error, const QString& _error_text);
  void walletActualBalanceUpdatedSignal(quint64 _actualBalance);
  void walletPendingBalanceUpdatedSignal(quint64 _pendingBalance);
  void walletActualDepositBalanceUpdatedSignal(quint64 _actualDepositBalance);
  void walletPendingDepositBalanceUpdatedSignal(quint64 _pendingDepositBalance);
  void walletActualInvestmentBalanceUpdatedSignal(quint64 _actualInvestmentBalance);
  void walletPendingInvestmentBalanceUpdatedSignal(quint64 _pendingInvestmentBalance);  
  void walletTransactionCreatedSignal(cn::TransactionId _transactionId);
  void walletSendTransactionCompletedSignal(cn::TransactionId _transactionId, int _error, const QString& _errorText);
  void walletSendMessageCompletedSignal(cn::TransactionId _transactionId, int _error, const QString& _errorText);
  void walletCreateDepositCompletedSignal(cn::TransactionId _transactionId, int _error, const QString& _errorText);
  void walletWithdrawDepositCompletedSignal(cn::TransactionId _transactionId, int _error, const QString& _errorText);
  void walletTransactionUpdatedSignal(cn::TransactionId _transactionId);
  void walletDepositsUpdatedSignal(const QVector<cn::DepositId>& _depositIds);
  void walletStateChangedSignal(const QString &_stateText, const QString &height);

  void openWalletWithPasswordSignal(bool _error);
  void changeWalletPasswordSignal();
  void updateWalletAddressSignal(const QString& _address);
  void reloadWalletTransactionsSignal();
  void updateBlockStatusTextSignal();
  void updateBlockStatusTextWithDelaySignal();
};



}
