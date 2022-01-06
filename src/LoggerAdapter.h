// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "Logging/LoggerManager.h"

namespace WalletGui
{
  class LoggerAdapter
  {
  public:
    static LoggerAdapter& instance();
    void init();
    logging::LoggerManager& getLoggerManager();
    void log(std::string message);

  private:
    logging::LoggerManager m_logManager;

    LoggerAdapter();
    ~LoggerAdapter();
  };

}  // namespace WalletGui
