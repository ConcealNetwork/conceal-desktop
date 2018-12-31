// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
// 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QString>

#include "CryptoNoteCore/Currency.h"

namespace WalletGui {

class CurrencyAdapter {

public:
  static CurrencyAdapter& instance();

  const CryptoNote::Currency& getCurrency();
  QString getCurrencyDisplayName() const;
  QString getCurrencyName() const;
  QString getCurrencyTicker() const;
  quint64 calculateInterest(quint64 _amount, quint32 _term, uint32_t height) const;
  quint64 getMinimumFee() const;
  quint64 getMinimumFeeV1() const;  
  quint64 getMinimumFeeBanking() const;  
  quint64 getAddressPrefix() const;
  quint64 getDepositMinAmount() const;
  quint32 getDepositMinTerm() const;
  quint32 getDepositMaxTerm() const;
  quint64 getDifficultyTarget() const;
  quintptr getNumberOfDecimalPlaces() const;
  QString formatAmount(quint64 _amount) const;
  quint64 parseAmount(const QString& _amountString) const;
  bool validateAddress(const QString& _address) const;

private:
  CryptoNote::Currency m_currency;

  CurrencyAdapter();
  ~CurrencyAdapter();
};

}
