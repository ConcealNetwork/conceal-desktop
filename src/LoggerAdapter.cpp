// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "LoggerAdapter.h"

#include "Logging/LoggerRef.h"
#include "Settings.h"

namespace WalletGui
{
  LoggerAdapter& LoggerAdapter::instance()
  {
    static LoggerAdapter inst;
    return inst;
  }

  void LoggerAdapter::init()
  {
    Common::JsonValue loggerConfiguration(Common::JsonValue::OBJECT);
    int64_t logLevel =
#ifdef DEBUG
        Logging::TRACE
#else
        Logging::INFO
#endif
        ;
    loggerConfiguration.insert("globalLevel", logLevel);
    Common::JsonValue& cfgLoggers = loggerConfiguration.insert("loggers", Common::JsonValue::ARRAY);
    Common::JsonValue& fileLogger = cfgLoggers.pushBack(Common::JsonValue::OBJECT);
    fileLogger.insert("type", "file");
    fileLogger.insert(
        "filename",
        Settings::instance().getDataDir().absoluteFilePath("Concealwallet.log").toStdString());
    fileLogger.insert("level", logLevel);
    m_logManager.configure(loggerConfiguration);
  }

  LoggerAdapter::LoggerAdapter() : m_logManager() { }

  LoggerAdapter::~LoggerAdapter() { }

  Logging::LoggerManager& LoggerAdapter::getLoggerManager() { return m_logManager; }

  void LoggerAdapter::log(std::string message)
  {
    Logging::LoggerRef logger(m_logManager, "desktop");
    logger(Logging::INFO) << message;
  }

}  // namespace WalletGui
