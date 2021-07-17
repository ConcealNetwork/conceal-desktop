// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "AddressBookDialog.h"
#include "AddressBookModel.h"
#include "AddressProvider.h"
#include "Common/CommandLine.h"
#include "Common/DnsTools.h"
#include "Common/PathTools.h"
#include "Common/SignalHandler.h"
#include "Common/StringTools.h"
#include "Common/Util.h"
#include "Common/Base58.h"
#include "Common/Util.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Account.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "CurrencyAdapter.h"
#include "DepositDetailsDialog.h"
#include "DepositListModel.h"
#include "DepositModel.h"
#include "ExchangeProvider.h"
#include "MainWindow.h"
#include "MainPasswordDialog.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "ContactDialog.h"
#include "NodeAdapter.h"
#include "OverviewFrame.h"
#include "PasswordDialog.h"
#include "PriceProvider.h"
#include "QRLabel.h"
#include "RecentTransactionsModel.h"
#include "Settings.h"
#include "SortedMessagesModel.h"
#include "SortedTransactionsModel.h"
#include "TransactionDetailsDialog.h"
#include "TransactionFrame.h"
#include "TransactionsListModel.h"
#include "TransactionsModel.h"
#include "VisibleMessagesModel.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "WalletLegacy/WalletHelper.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCompleter>
#include <QDesktopServices>
#include <QFont>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QStringList>
#include <QtCore>
#include <QUrl>
#include <QFileDialog>

#include "ui_overviewframe.h"

namespace WalletGui
{

  Q_DECL_CONSTEXPR quint64 MESSAGE_AMOUNT = 100;
  Q_DECL_CONSTEXPR quint64 MESSAGE_CHAR_PRICE = 10;
  Q_DECL_CONSTEXPR quint64 MINIMAL_MESSAGE_FEE = 10;
  Q_DECL_CONSTEXPR int DEFAULT_MESSAGE_MIXIN = 4;
  Q_DECL_CONSTEXPR quint32 MINUTE_SECONDS = 60;
  Q_DECL_CONSTEXPR quint32 HOUR_SECONDS = 60 * MINUTE_SECONDS;
  Q_DECL_CONSTEXPR int MIN_TTL = 5 * MINUTE_SECONDS;
  Q_DECL_CONSTEXPR int MAX_TTL = 14 * HOUR_SECONDS;
  Q_DECL_CONSTEXPR int TTL_STEP = 5 * MINUTE_SECONDS;
  Q_DECL_CONSTEXPR int BASE_FEE = 1000;
  Q_DECL_CONSTEXPR int REMOTE_FEE = 100000;

  /* Convert months to the number of blocks */
  QString monthsToBlocks(int _months)
  {

    int maxPeriod = 13;
    uint32_t blocksPerDeposit = 21900;

    QString resTempate("%1 %2");
    if (_months < maxPeriod)
    {
      return resTempate.arg(_months * blocksPerDeposit).arg(QObject::tr("blocks"));
    }
    return QString();
  }

  class RecentTransactionsDelegate : public QStyledItemDelegate
  {
    Q_OBJECT

  public:
    explicit RecentTransactionsDelegate(QObject *_parent) : QStyledItemDelegate(_parent)
    {
    }
    ~RecentTransactionsDelegate()
    {
    }

    QWidget *createEditor(QWidget *_parent, const QStyleOptionViewItem &_option, const QModelIndex &_index) const Q_DECL_OVERRIDE
    {
      if (!_index.isValid())
      {
        return nullptr;
      }
      return new TransactionFrame(_index, _parent);
    }

    QSize sizeHint(const QStyleOptionViewItem &_option, const QModelIndex &_index) const Q_DECL_OVERRIDE
    {
      return QSize(346, 32);
    }
  };

  OverviewFrame::OverviewFrame(QWidget *_parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame),
                                                   m_priceProvider(new PriceProvider(this)),
                                                   m_transactionModel(new RecentTransactionsModel),
                                                   m_transactionsModel(new TransactionsListModel),
                                                   m_depositModel(new DepositListModel),
                                                   m_visibleMessagesModel(new VisibleMessagesModel),
                                                   m_exchangeProvider(new ExchangeProvider(this)),
                                                   m_addressProvider(new AddressProvider(this))
  {
    m_ui->setupUi(this);

    m_ui->m_addressBookView->setModel(&AddressBookModel::instance());
    m_ui->m_addressBookView->sortByColumn(AddressBookModel::COLUMN_LABEL, Qt::AscendingOrder);
    m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_ADDRESS, 1000);
    m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_PAYMENTID, 200);

    connect(m_ui->m_addressBookView->selectionModel(), &QItemSelectionModel::currentChanged, this, &OverviewFrame::currentAddressChanged);

    m_ui->m_addressBookView->setContextMenuPolicy(Qt::CustomContextMenu);

    /* Don't show the LOCK button if the wallet is not encrypted */
    if (!Settings::instance().isEncrypted())
    {
      m_ui->lm_lockWalletButton->hide();
    }
    else
    {
      m_ui->lm_lockWalletButton->show();
    }


    m_ui->m_size->addItem("12");
    m_ui->m_size->addItem("13");
    m_ui->m_size->addItem("14");
    m_ui->m_size->addItem("15");
    m_ui->m_size->addItem("16");
    m_ui->m_size->addItem("17");
    m_ui->m_size->addItem("18");
    m_ui->m_size->addItem("19");

    m_ui->m_font->addItem("Poppins");
    m_ui->m_font->addItem("Roboto");
    m_ui->m_font->addItem("Lekton");
    m_ui->m_font->addItem("Montserrat");
    m_ui->m_font->addItem("Open Sans");
    m_ui->m_font->addItem("Lato");
    m_ui->m_font->addItem("Oswald");

    /* Add the currencies we support to the combobox */
    m_ui->m_language->addItem("USD");
    m_ui->m_language->addItem("EUR");
    m_ui->m_language->addItem("GBP");
    m_ui->m_language->addItem("RUB");
    m_ui->m_language->addItem("TRY");
    m_ui->m_language->addItem("CNY");
    m_ui->m_language->addItem("AUD");
    m_ui->m_language->addItem("NZD");
    m_ui->m_language->addItem("SGD");
    m_ui->m_language->addItem("LKR");
    m_ui->m_language->addItem("VND");
    m_ui->m_language->addItem("ISK");
    m_ui->m_language->addItem("INR");
    m_ui->m_language->addItem("IDR");
    m_ui->m_language->addItem("ISK");
    m_ui->m_language->addItem("HKD");
    m_ui->m_language->addItem("PAB");
    m_ui->m_language->addItem("ZAR");
    m_ui->m_language->addItem("KRW");
    m_ui->m_language->addItem("BRL");
    m_ui->m_language->addItem("BYN");
    m_ui->m_language->addItem("VEF");
    m_ui->m_language->addItem("MUR");
    m_ui->m_language->addItem("IRR");
    m_ui->m_language->addItem("SAR");
    m_ui->m_language->addItem("AED");
    m_ui->m_language->addItem("PKR");
    m_ui->m_language->addItem("EGP");
    m_ui->m_language->addItem("ILS");
    m_ui->m_language->addItem("SEK");
    m_ui->m_language->addItem("NOK");
    m_ui->m_language->addItem("LSL");
    m_ui->m_language->addItem("UAH");
    m_ui->m_language->addItem("RON");
    m_ui->m_language->addItem("KZT");
    m_ui->m_language->addItem("MYR");
    m_ui->m_language->addItem("RON");
    m_ui->m_language->addItem("MXN");

    m_ui->m_transactionsView->setModel(m_transactionsModel.data());
    m_ui->m_transactionsView->header()->setSectionResizeMode(TransactionsModel::COLUMN_STATE, QHeaderView::Fixed);
    m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_STATE, 15);
    m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_DATE, 140);
    m_ui->m_transactionsView->header()->moveSection(3, 5);
    m_ui->m_transactionsView->header()->moveSection(0, 1);
    m_ui->m_transactionsView->header()->resizeSection(TransactionsModel::COLUMN_HASH, 300);

    m_ui->m_depositView->setModel(m_depositModel.data());
    m_ui->m_messagesView->setModel(m_visibleMessagesModel.data());

    m_ui->m_timeSpin->setSuffix(QString(" %1").arg(tr("Month(s)")));
    m_ui->m_timeSpin->setMaximum(12);
    timeChanged(1);

    m_ui->darkness->hide();
    m_ui->lockBox->hide();
    m_ui->darkness->setStyleSheet("background-color: rgba(0,0,0,160);");

    m_ui->m_ttlSlider->setVisible(true);
    m_ui->m_ttlLabel->setVisible(true);
    m_ui->m_ttlSlider->setMinimum(1);
    m_ui->m_ttlSlider->setMaximum(MAX_TTL / MIN_TTL);
    ttlValueChanged(m_ui->m_ttlSlider->value());

    m_ui->m_amountSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());

    m_ui->m_messagesView->header()->resizeSection(MessagesModel::COLUMN_DATE, 180);

    m_ui->m_depositView->header()->resizeSection(DepositModel::COLUMN_STATE, 75);
    m_ui->m_depositView->header()->resizeSection(1, 100);
    m_ui->m_depositView->header()->resizeSection(2, 200);
    m_ui->m_depositView->header()->resizeSection(3, 50);

#ifdef HAVE_CHART
    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    m_ui->verticalLayout_9->addWidget(m_chartView);
#else
    m_chart = new QLabel();
    m_ui->verticalLayout_9->addWidget(m_chart);
#endif

    /* Connect signals */
    connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &OverviewFrame::sendTransactionCompleted, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletSendMessageCompletedSignal, this, &OverviewFrame::sendMessageCompleted, Qt::QueuedConnection);

    connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &OverviewFrame::actualBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal, this, &OverviewFrame::pendingBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletActualDepositBalanceUpdatedSignal, this, &OverviewFrame::actualDepositBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingDepositBalanceUpdatedSignal, this, &OverviewFrame::pendingDepositBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletActualInvestmentBalanceUpdatedSignal, this, &OverviewFrame::actualInvestmentBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingInvestmentBalanceUpdatedSignal, this, &OverviewFrame::pendingInvestmentBalanceUpdated, Qt::QueuedConnection);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &OverviewFrame::reset, Qt::QueuedConnection);
    connect(m_transactionModel.data(), &QAbstractItemModel::rowsInserted, this, &OverviewFrame::transactionsInserted);
    connect(m_transactionModel.data(), &QAbstractItemModel::layoutChanged, this, &OverviewFrame::layoutChanged);

    connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &OverviewFrame::updateWalletAddress);
    connect(m_priceProvider, &PriceProvider::priceFoundSignal, this, &OverviewFrame::onPriceFound);
    connect(m_addressProvider, &AddressProvider::addressFoundSignal, this, &OverviewFrame::onAddressFound, Qt::QueuedConnection);
    connect(m_exchangeProvider, &ExchangeProvider::exchangeFoundSignal, this, &OverviewFrame::onExchangeFound);

    connect(&WalletAdapter::instance(), &WalletAdapter::walletStateChangedSignal, this,
            &OverviewFrame::setStatusBarText);
    connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this,
            &OverviewFrame::walletSynchronized, Qt::QueuedConnection);
    connect(&refreshDataTimer, &QTimer::timeout, this, &OverviewFrame::refreshDataClicked);

    /* Initialize basic ui elements */
    m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
    m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
    m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
    m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
    m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());
    disableAddressBookButtons();

    walletSynced = false;

    /** Get current currency from the config file and update 
     * the dropdown in settings to show the correct currency */
    QString currentFont = Settings::instance().getFont();
    int index = m_ui->m_font->findText(currentFont, Qt::MatchExactly);
    m_ui->m_font->setCurrentIndex(index);

    QString currency = Settings::instance().getCurrentCurrency();
    int index2 = m_ui->m_language->findText(currency, Qt::MatchExactly);
    m_ui->m_language->setCurrentIndex(index2);

    QString size = QString::number(Settings::instance().getFontSize());
    int index3 = m_ui->m_size->findText(size, Qt::MatchExactly);
    m_ui->m_size->setCurrentIndex(index3);

    /* Check saved sizes and ensure they are within the acceptable parameters */
    int startingFontSize = Settings::instance().getFontSize();
    if ((startingFontSize < 12) || (startingFontSize > 19)){
      startingFontSize = 13;
      Settings::instance().setFontSize(startingFontSize);
    }

    /* Apply the default or saved style */
    setStyles(startingFontSize);

    /* Load the price chart */
    loadChart();

    QString connection = Settings::instance().getConnection();

    /* Get current language */
    QString language = Settings::instance().getLanguage();

    if (language.compare("tr") == 0)
    {
      m_ui->m_turkish->setChecked(true);
    }
    else if (language.compare("ru") == 0)
    {
      m_ui->m_russian->setChecked(true);
    }
    else if (language.compare("cn") == 0)
    {
      m_ui->m_chinese->setChecked(true);
    }
    else
    {
      m_ui->m_english->setChecked(true);
    }

    /* Set current connection options */
    QString remoteHost = Settings::instance().getCurrentRemoteNode();
    m_ui->m_hostEdit->setText(remoteHost);

    /* If the connection is a remote node, then load the current (or default)
      remote node into the text field. */
    if (connection.compare("remote") == 0)
    {
      m_ui->radioButton->setChecked(true);
    }

    if (connection.compare("autoremote") == 0)
    {
      m_ui->radioButton_3->setChecked(true);
    }

    /* It is an embedded node, so let only check that */
    else if (connection.compare("embedded") == 0)
    {
      m_ui->radioButton_2->setChecked(true);
    }

    if (Settings::instance().getMaximizedStatus() == "enabled")
    {
      m_ui->b2_startMaximized->setText(tr("CLICK TO DISABLE"));
    }
    else
    {
      m_ui->b2_startMaximized->setText(tr("CLICK TO ENABLE"));
    }



    if (Settings::instance().getAutoOptimizationStatus() == "enabled")
    {
      m_ui->b2_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));
    }
    else
    {
      m_ui->b2_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));
    }

#ifndef QT_NO_SYSTEMTRAYICON
    /* Set minimize to tray button status */
    if (!Settings::instance().isMinimizeToTrayEnabled())
    {
      m_ui->b2_minToTrayButton->setText(tr("CLICK TO ENABLE"));
    }
    else
    {
      m_ui->b2_minToTrayButton->setText(tr("CLICK TO DISABLE"));
    }

    /* Set close to tray button status */
    if (!Settings::instance().isCloseToTrayEnabled())
    {
      m_ui->b2_closeToTrayButton->setText(tr("CLICK TO ENABLE"));
    }
    else
    {
      m_ui->b2_closeToTrayButton->setText(tr("CLICK TO DISABLE"));
    }
#endif

    /* Set auto refresh data button status */
    if (!Settings::instance().isAutoRefreshData())
    {
      m_ui->b2_autoRefreshDataButton->setText(tr("CLICK TO ENABLE"));
    }
    else
    {
      m_ui->b2_autoRefreshDataButton->setText(tr("CLICK TO DISABLE"));
    }

    dashboardClicked();
    depositParamsChanged();
    reset();
    showCurrentWalletName();
    calculateFee();

    /* Add addresses suggestions from the address book*/
    QCompleter* completer = new QCompleter(&AddressBookModel::instance(), this);
    completer->setCompletionRole(AddressBookModel::ROLE_ADDRESS);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    QTreeView* popup = new QTreeView;
    completer->setPopup(popup);
    popup->setStyleSheet("background-color: #282d31; color: #aaa");
    popup->setIndentation(0);
    popup->header()->setStretchLastSection(false);
    popup->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->m_addressEdit->setCompleter(completer);
#ifndef Q_OS_WIN
    m_ui->openSSL_layout->hide();
#endif
    if (Settings::instance().isAutoRefreshData())
    {
      refreshDataTimer.start(REFRESH_INTERVAL);
    }
  }

  OverviewFrame::~OverviewFrame()
  {
  }

  void OverviewFrame::walletSynchronized(int _error, const QString &_error_text)
  {
    showCurrentWalletName();
    walletSynced = true;

    /* Check saved sizes and ensure they are within the acceptable parameters */
    int startingFontSize = Settings::instance().getFontSize();
    if ((startingFontSize < 12) || (startingFontSize > 19))
    {
      startingFontSize = 13;
      Settings::instance().setFontSize(startingFontSize);
    }

    /* Apply the default or saved style */
    setStyles(startingFontSize);

    /* Show total portfolio */
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;

    quint64 numUnlockedOutputs;
    numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
    if (numUnlockedOutputs >= 100)
    {
      m_ui->m_optimizationMessage->setText("Recommended [" + QString::number(numUnlockedOutputs) + "]");
    }
    else
    {
      m_ui->m_optimizationMessage->setText("Not required [" + QString::number(numUnlockedOutputs) + "]");
    }

    if (!Settings::instance().isEncrypted())
    {
      m_ui->lm_lockWalletButton->hide();
    }

    updatePortfolio();
  }

  void OverviewFrame::transactionsInserted(const QModelIndex &_parent, int _first, int _last)
  {
    for (quint32 i = _first; i <= _last; ++i)
    {
      QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
      m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
    }
  }

  void OverviewFrame::updateWalletAddress(const QString &_address)
  {
    OverviewFrame::wallet_address = _address;
    std::string theAddress = _address.toStdString();
    std::string start = theAddress.substr(0, 6);
    std::string end = theAddress.substr(92, 6);
    m_ui->m_copyAddressButton_3->setText("Address: " + QString::fromStdString(start) + "......" + QString::fromStdString(end));

    /* Show/hide the encrypt wallet button */
    if (!Settings::instance().isEncrypted())
    {
      m_ui->b2_encryptWalletButton->setText("ENCRYPT WALLET");
    }
    else
    {
      m_ui->b2_encryptWalletButton->setText("CHANGE PASSWORD");
    }

    /* Don't show the LOCK button if the wallet is not encrypted */
    if (!Settings::instance().isEncrypted())
    {
      m_ui->lm_lockWalletButton->hide();
    }
    else
    {
      m_ui->lm_lockWalletButton->show();
    }
  }

  QList<QWidget *> OverviewFrame::getWidgets() { return m_ui->groupBox->findChildren<QWidget *>(); }

  QList<QPushButton *> OverviewFrame::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> OverviewFrame::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void OverviewFrame::applyStyles()
  {
    QList<QTreeView *> treeViews = m_ui->groupBox->findChildren<QTreeView *>();
    foreach (QTreeView *treeView, treeViews)
    {
      treeView->setStyleSheet(tableStyle);
    }
    m_ui->groupBox->update();
  }

  void OverviewFrame::refreshDataClicked()
  {
    m_priceProvider->getPrice();
    loadChart();
    Q_EMIT notifySignal(tr("Data updated"));
  }

  /* Load the price chart on the overview screen */
  void OverviewFrame::loadChart()
  {
    /* Get current currency */
    QString currency = Settings::instance().getCurrentCurrency().toLower();

    /* Pull the chart */
    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    connect(nam, &QNetworkAccessManager::finished, this, &OverviewFrame::downloadFinished);

    QUrl url;
    QString link;

#ifdef HAVE_CHART

    link = QString(
               "https://api.coingecko.com/api/v3/coins/conceal/market_chart?vs_currency=%1&days=30")
               .arg(currency);
#else

    QSize size = m_ui->groupBox->size();
    int width = size.width();
    int height = size.height();

    link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
           "&days=30&priceDecimals=2&xPoints=24&width=1170&height=560&dateFormat=MM-DD";

    /** 1280 x 720 or smaller is the default */
    if (width < 1363)
    {
      link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
             "&days=7&priceDecimals=2&xPoints=12&width=526&height=273&dateFormat=MM-DD";
    }

    /** 1365 x 768 */
    if ((width == 1363) && (height == 750))
    {
      link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
             "&days=7&priceDecimals=2&xPoints=12&width=618&height=297&dateFormat=MM-DD";
    }

    /** 1440 x 900 */
    if ((width == 1438) && (height == 868))
    {
      link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
             "&days=7&priceDecimals=2&xPoints=12&width=695&height=416&dateFormat=MM-DD";
    }

    /** 1680 x 1050 */
    if ((width == 1678) && (height == 1008))
    {
      link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
             "&days=14&priceDecimals=2&xPoints=12&width=927&height=555&dateFormat=MM-DD";
    }

    /** This should cover 1920 and above */
    if (width > 1599)
    {
      link = "https://explorer.conceal.network/services/charts/price.png?vsCurrency=" + currency +
             "&days=30&priceDecimals=2&xPoints=24&width=1170&height=560&dateFormat=MM-DD";
    }
#endif

    url = QUrl::fromUserInput(link);

    QNetworkRequest request(url);
    nam->get(request);
  }

  /* Show the name of the opened wallet */
  void OverviewFrame::showCurrentWalletName()
  {

    QString walletFile = Settings::instance().getWalletName();
    m_ui->m_currentWalletTitle->setText(walletFile);
  }

  /* Download is done, set the chart as the pixmap */
  void OverviewFrame::downloadFinished(QNetworkReply *reply)
  {
#ifdef HAVE_CHART
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());

    // We need to have the object contained in the document to ensure compatibility with Qt < 5.10
    QJsonObject documentAsObject = document.object();

    QLineSeries *series = new QLineSeries();

    QPen seriesPen;
    seriesPen.setWidth(2);
    seriesPen.setColor("orange");
    series->setPen(seriesPen);

    if (!documentAsObject["prices"].isArray())
    {
      return;
    }

    QJsonArray priceList = documentAsObject["prices"].toArray();

    foreach (const QJsonValue info, priceList)
    {
      if (!info.isArray())
      {
        return;
      }
      QJsonArray array = info.toArray();
      series->append(array.at(0).toDouble(), array.at(1).toDouble());
    }

    QChart *chart = new QChart();
    chart->setBackgroundVisible(false);
    QMargins margins(0, 0, 0, 0);
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setMargins(margins);
    chart->addSeries(series);
    chart->legend()->hide();

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("MM/dd");
    axisX->setLabelsFont(currentFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%.2f " + Settings::instance().getCurrentCurrency());
    axisY->setLabelsFont(currentFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QColor chartColor = QRgb(0x999999);

    axisX->setLabelsColor(chartColor);
    axisX->setLinePenColor(chartColor);
    axisX->setGridLineColor(chartColor);

    axisY->setLabelsColor(chartColor);
    axisY->setLinePenColor(chartColor);
    axisY->setGridLineColor(chartColor);

    m_ui->m_dataSource->setText(
        tr("Market data from coingecko.com - Last updated: %1")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));

    m_chartView->setChart(chart);
#else
    QPixmap pm;
    pm.loadFromData(reply->readAll());
    m_chart->setPixmap(pm);
    m_chart->setScaledContents(true);
#endif

    int startingFontSize = Settings::instance().getFontSize();
    setStyles(startingFontSize);
  }

  void OverviewFrame::calculateFee()
  {
    m_actualFee = BASE_FEE;
    QString connection = Settings::instance().getConnection();
    if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
    {
      if (!OverviewFrame::remote_node_fee_address.isEmpty())
      {
        m_actualFee = m_actualFee + REMOTE_FEE;
      }
    }
  }

  void OverviewFrame::layoutChanged()
  {
    for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i)
    {
      QModelIndex recent_index = m_transactionModel->index(i, 0);
      m_ui->m_recentTransactionsView->openPersistentEditor(recent_index);
    }
    showCurrentWalletName();
  }

  /* What happens when the available balance changes */
  void OverviewFrame::actualBalanceUpdated(quint64 _balance)
  {
    m_ui->m_actualBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));                   // Overview screen
    m_ui->m_balanceLabel->setText("Available Balance: " + CurrencyAdapter::instance().formatAmount(_balance) + " CCX"); // Send funds screen
    m_actualBalance = _balance;
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    updatePortfolio();
  }

  /* What happens when the pending(locked) balance changes */
  void OverviewFrame::pendingBalanceUpdated(quint64 _balance)
  {
    m_ui->m_lockedBalance->setText(CurrencyAdapter::instance().formatAmount(_balance));
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    updatePortfolio();
  }

  /* What happens when the unlocked deposit balance changes */
  void OverviewFrame::actualDepositBalanceUpdated(quint64 _balance)
  {
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    m_ui->m_unlockedDeposits->setText(CurrencyAdapter::instance().formatAmount(actualDepositBalance + actualInvestmentBalance));
    updatePortfolio();
    quint64 unlockedFunds = actualDepositBalance + actualInvestmentBalance;
    if (walletSynced == true)
    {
      if (unlockedFunds > 0)
      {
        m_ui->m_unlockedDeposits->setStyleSheet(
            "color: orange; background: transparent; border: none;");
      }
      else
      {
        m_ui->m_unlockedDeposits->setStyleSheet(
            "color: #ddd; background: transparent; border: none;");
      }
    }
  }

  /* What happens when the unlocked investment balance changes */
  void OverviewFrame::actualInvestmentBalanceUpdated(quint64 _balance)
  {
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    m_ui->m_unlockedDeposits->setText(CurrencyAdapter::instance().formatAmount(actualDepositBalance + actualInvestmentBalance));
    updatePortfolio();
    quint64 unlockedFunds = actualDepositBalance + actualInvestmentBalance;
    if (walletSynced == true)
    {
      if (unlockedFunds > 0)
      {
        m_ui->m_unlockedDeposits->setStyleSheet(
            "color: orange; background: transparent; border: none;");
      }
      else
      {
        m_ui->m_unlockedDeposits->setStyleSheet(
            "color: #ddd; background: transparent; border: none;");
      }
    }
  }

  /* What happens when the locked deposit balance changes */
  void OverviewFrame::pendingDepositBalanceUpdated(quint64 _balance)
  {
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    m_ui->m_lockedDeposits->setText(CurrencyAdapter::instance().formatAmount(pendingDepositBalance + pendingInvestmentBalance));
    updatePortfolio();
  }

  void OverviewFrame::resizeEvent(QResizeEvent *event)
  {
#ifndef HAVE_CHART
    loadChart();
#endif
  }

  /* What happens when the locked investment balance changes */
  void OverviewFrame::pendingInvestmentBalanceUpdated(quint64 _balance)
  {
    quint64 actualBalance = WalletAdapter::instance().getActualBalance();
    quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
    quint64 actualDepositBalance = WalletAdapter::instance().getActualDepositBalance();
    quint64 pendingDepositBalance = WalletAdapter::instance().getPendingDepositBalance();
    quint64 actualInvestmentBalance = WalletAdapter::instance().getActualInvestmentBalance();
    quint64 pendingInvestmentBalance = WalletAdapter::instance().getPendingInvestmentBalance();
    OverviewFrame::totalBalance = pendingDepositBalance + actualDepositBalance + actualBalance + pendingBalance + pendingInvestmentBalance + actualInvestmentBalance;
    m_ui->m_lockedDeposits->setText(CurrencyAdapter::instance().formatAmount(pendingDepositBalance + pendingInvestmentBalance));
    updatePortfolio();
  }

  /* Price data download complete */
  void OverviewFrame::onPriceFound(QJsonObject &result)
  {
    QString currentCurrency = Settings::instance().getCurrentCurrency();

    QString selectedCurrency = currentCurrency.toLower();
    QString marketCapCurrency = selectedCurrency + "_market_cap";
    QString volumeCurrency = selectedCurrency + "_24h_vol";
    QString changeCurrency = selectedCurrency + "_24h_change";

    double currency = result[selectedCurrency].toDouble();
    ccxfiat = (float)currency;
    QString ccx = QLocale(QLocale::system()).toString(currency, 'f', 2);
    double market_cap = result[marketCapCurrency].toDouble();
    QString ccx_market_cap = QLocale(QLocale::system()).toString(market_cap, 'f', 2);
    double c24h_volume = result[volumeCurrency].toDouble();
    QString ccx_24h_volume = QLocale(QLocale::system()).toString(c24h_volume, 'f', 2);
    double c24h_change = result[changeCurrency].toDouble();
    QString ccx_24h_change = QLocale(QLocale::system()).toString(c24h_change, 'f', 2);
    m_ui->m_ccx->setText(ccx + " " + currentCurrency);
    m_ui->m_24hChange->setText(ccx_24h_change + "%");
    m_ui->m_marketCap->setText(ccx_market_cap + " " + currentCurrency);
    m_ui->m_volume->setText(ccx_24h_volume + " " + currentCurrency);

    updatePortfolio();
  }

  /* Exchange address check complete */
  void OverviewFrame::onExchangeFound(QString &_exchange)
  {
    exchangeName = _exchange;
  }

  /* Update the total portfolio in CCX and Fiat on the top left hand corner */
  void OverviewFrame::updatePortfolio()
  {
    QString currentCurrency = Settings::instance().getCurrentCurrency();
    float total = 0;
    total = ccxfiat * (float)OverviewFrame::totalBalance;
    m_ui->ccxTotal->setText(CurrencyAdapter::instance().formatAmount(OverviewFrame::totalBalance) + " CCX ");
    m_ui->fiatTotal->setText(CurrencyAdapter::instance().formatCurrencyAmount(total / 10000) + " " + currentCurrency);
    m_ui->fiatLabel->setText("Portfolio (" + currentCurrency + ")");
  }

  /* Banking menu button clicked */
  void OverviewFrame::bankingClicked()
  {
    m_ui->darkness->hide();
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    if (walletSynced == true)
    {
      m_ui->m_myConcealWalletTitle->setText(tr("BANKING"));
      m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-banking"));
      m_ui->bankingBox->raise();
    }
    else
    {
      syncInProgressMessage();
    }
  }

  void OverviewFrame::transactionHistoryClicked()
  {
    m_ui->darkness->hide();
    m_ui->m_myConcealWalletTitle->setText(tr("TRANSACTIONS"));
    m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-transactions"));
    m_ui->transactionsBox->raise();
  }

  void OverviewFrame::dashboardClicked()
  {
    m_ui->darkness->hide();
    m_ui->m_myConcealWalletTitle->setText(tr("CONCEAL.NETWORK"));
    m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-home"));
    m_ui->overviewBox->raise();
    m_ui->lm_newTransferButton->show();
    m_ui->lm_newMessageButton->show();
    m_ui->leftMenu->raise();
    m_ui->leftMenu->show();
    m_ui->statusBox->raise();
    m_ui->statusBox->show();
    m_ui->headerBox->raise();
    m_ui->headerBox->show();
    setStyles(Settings::instance().getFontSize());
  }

  void OverviewFrame::aboutClicked()
  {
    m_ui->darkness->hide();
    m_ui->m_myConcealWalletTitle->setText(tr("ABOUT"));
    m_ui->m_titleIcon->setText("?");
    m_ui->aboutBox->raise();
    m_ui->lm_newTransferButton->show();
    m_ui->lm_newMessageButton->show();
  }

  void OverviewFrame::settingsClicked()
  {
    m_ui->darkness->hide();
    m_ui->m_myConcealWalletTitle->setText(tr("WALLET SETTINGS"));
    m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/settings").scaled(36, 36));
    m_ui->settingsBox->raise();
  }

  void OverviewFrame::qrCodeClicked()
  {
    Q_EMIT qrSignal(OverviewFrame::wallet_address);
  }

  void OverviewFrame::inboxClicked()
  {
    m_ui->darkness->hide();
    m_ui->m_myConcealWalletTitle->setText(tr("INBOX"));
    m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-messages"));
    m_ui->messageBox->raise();
  }

  void OverviewFrame::newWalletClicked()
  {
    Q_EMIT newWalletSignal();
  }

  void OverviewFrame::closeWalletClicked()
  {
    Q_EMIT closeWalletSignal();
  }

  void OverviewFrame::newTransferClicked()
  {
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    if (walletSynced == true)
    {
      m_ui->m_myConcealWalletTitle->setText(tr("SEND FUNDS"));
      m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-send"));
      m_ui->sendBox->raise();
      OverviewFrame::fromPay = true;
    }
    else
    {
      syncInProgressMessage();
    }
  }

  void OverviewFrame::addressEditTextChanged(QString text)
  {
    /* Small trick for the completer to show all the rows in the popup. Without this, one row is missing due to the header */
    int rowCount = m_ui->m_addressEdit->completer()->popup()->model()->rowCount();
    m_ui->m_addressEdit->completer()->popup()->window()->setMinimumHeight(
      20 * (rowCount + 1));
  }

  void OverviewFrame::newMessageClicked()
  {
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    if (walletSynced == true)
    {
      m_ui->m_myConcealWalletTitle->setText(tr("NEW MESSAGE"));
      m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-send-message"));
      m_ui->newMessageBox->raise();
      OverviewFrame::fromPay = false;
    }
    else
    {
      syncInProgressMessage();
    }
  }

  void OverviewFrame::reset()
  {
    actualBalanceUpdated(0);
    pendingBalanceUpdated(0);
    actualDepositBalanceUpdated(0);
    pendingDepositBalanceUpdated(0);
    actualInvestmentBalanceUpdated(0);
    pendingInvestmentBalanceUpdated(0);
    layoutChanged();
    m_priceProvider->getPrice();
    m_addressProvider->getAddress();
    Q_EMIT resetWalletSignal();
  }

  void OverviewFrame::setStatusBarText(const QString& _text, const QString& _height)
  {
    m_ui->m_statusBox->setText(_text);
    m_ui->statusHeight->setText(_height);
    showCurrentWalletName();
  }

  void OverviewFrame::copyClicked()
  {
    QApplication::clipboard()->setText(OverviewFrame::wallet_address);
    Q_EMIT notifySignal(tr("Address copied to clipboard"));
  }

  void OverviewFrame::syncInProgressMessage()
  {
    Q_EMIT notifySignal(
        tr("Synchronization is in progress.\nThis option is not available "
           "until your wallet is synchronized with the network."));
  }

  // TRANSACTION HISTORY

  void OverviewFrame::showTransactionDetails(const QModelIndex &_index)
  {
    if (!_index.isValid())
    {
      return;
    }
    m_ui->darkness->show();
    m_ui->darkness->raise();
    TransactionDetailsDialog dlg(_index, this);
    dlg.exec();
    m_ui->darkness->hide();
  }

  // MESSAGE LIST

  void OverviewFrame::showMessageDetails(const QModelIndex &_index)
  {
    /* Darken the background */
    m_ui->darkness->show();
    m_ui->darkness->raise();
    if (!_index.isValid())
    {
      return;
    }

    MessageDetailsDialog dlg(_index, this);
    dlg.exec();

    /* Restore the background */
    m_ui->darkness->hide();
  }

  // SEND FUNDS

  /* incoming data from address book frame */
  void OverviewFrame::setAddress(const QString &_address)
  {
    if (OverviewFrame::fromPay == true)
    {
      m_ui->m_addressEdit->setText(_address);
      m_ui->m_myConcealWalletTitle->setText(tr("SEND FUNDS"));
      m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-send"));
      m_ui->sendBox->raise();
    }
    else
    {
      m_ui->m_addressMessageEdit->setText(_address);
      m_ui->m_myConcealWalletTitle->setText(tr("SEND MESSAGE"));
      m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-send-message"));
      m_ui->newMessageBox->raise();
    }
  }

  /* incoming data from address book frame */
  void OverviewFrame::setPaymentId(const QString &_paymentId)
  {
    m_ui->m_paymentIdEdit->setText(_paymentId);
  }

  /* Set the variable to the fee address, save the address in settings so
   other functions can use, and show the fee if a fee address is found */
  void OverviewFrame::onAddressFound(const QString &_address)
  {
    QString connection = Settings::instance().getConnection();
    if ((!_address.isEmpty()) && (connection != "embedded"))
    {
      OverviewFrame::remote_node_fee_address = _address;
      Settings::instance().setCurrentFeeAddress(_address);
      calculateFee();
      QString fee =
          QString("Fee: %1 CCX").arg(CurrencyAdapter::instance().formatAmount(m_actualFee));
      m_ui->m_sendFee->setText(fee);
      m_ui->m_messageFee->setText(fee);
      m_ui->m_depositFeeLabel->setText(QString::number(m_actualFee / (double)1000000, 'f', 6) + " CCX");
    }
  }

  /* clear all fields */
  void OverviewFrame::clearAllClicked()
  {
    m_ui->m_paymentIdEdit->clear();
    m_ui->m_addressEdit->clear();
    m_ui->m_addressLabel->clear();
    m_ui->m_messageEdit->clear();
    m_ui->m_amountEdit->setText("0.000000");
  }

  /* Set the amount to 25% of available funds */
  void OverviewFrame::setPercentage25()
  {
    calculateFee();
    uint64_t amount = (m_actualBalance - m_actualFee) * 0.25;
    m_ui->m_amountEdit->setText(CurrencyAdapter::instance().formatAmount(amount));
  }

  /* Set the amount to 50% of available funds */
  void OverviewFrame::setPercentage50()
  {
    calculateFee();
    uint64_t amount = (m_actualBalance - m_actualFee) * 0.5;
    m_ui->m_amountEdit->setText(CurrencyAdapter::instance().formatAmount(amount));
  }

  /* Set the amount to total available funds */
  void OverviewFrame::setPercentage100()
  {
    calculateFee();
    uint64_t maxAmount = WalletAdapter::instance().getWalletMaximum();    
    uint64_t amount = maxAmount - m_actualFee;
    m_ui->m_amountEdit->setText(CurrencyAdapter::instance().formatAmount(amount));
  }

  void OverviewFrame::sendFundsClicked()
  {
    /* Check if its a tracking wallet */
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    /* Prepare the transfers */
    QVector<CryptoNote::WalletLegacyTransfer> walletTransfers;
    CryptoNote::WalletLegacyTransfer walletTransfer;
    QVector<CryptoNote::TransactionMessage> walletMessages;
    bool isIntegrated = false;
    std::string paymentID;
    std::string spendPublicKey;
    std::string viewPublicKey;
    QByteArray paymentIdString;

    QString address = m_ui->m_addressEdit->text().toUtf8();
    QString int_address = m_ui->m_addressEdit->text().toUtf8();

    /* Is it an Integrated address? */
    if (address.toStdString().length() == 186)
    {
      isIntegrated = true;
      const uint64_t paymentIDLen = 64;

      /* Extract and commit the payment id to extra */
      std::string decoded;
      uint64_t prefix;
      if (Tools::Base58::decode_addr(address.toStdString(), prefix, decoded))
      {
        paymentID = decoded.substr(0, paymentIDLen);
      }

      /* Create the address from the public keys */
      std::string keys = decoded.substr(paymentIDLen, std::string::npos);
      CryptoNote::AccountPublicAddress addr;
      CryptoNote::BinaryArray ba = Common::asBinaryArray(keys);

      CryptoNote::fromBinaryArray(addr, ba);

      std::string address_string = CryptoNote::getAccountAddressAsStr(CryptoNote::parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, addr);
      address = QString::fromStdString(address_string);
    }

    /* If its an integrated address, lets copy the payment ID to the field */
    if (isIntegrated == true)
    {
      m_ui->m_paymentIdEdit->setText(QString::fromStdString(paymentID));
    }

    try
    {
      /* Is it a Conceal ID? */
      if (CurrencyAdapter::instance().isValidOpenAliasAddress(address))
      {
        /* Parse the record and set address to the actual CCX address */
        std::vector<std::string> records;
        if (!Common::fetch_dns_txt(address.toStdString(), records))
        {
          QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Failed to lookup Conceal ID"), QtCriticalMsg));
        }
        std::string realAddress;
        for (const auto &record : records)
        {
          if (CurrencyAdapter::instance().processServerAliasResponse(record, realAddress))
          {
            address = QString::fromStdString(realAddress);
            m_ui->m_addressEdit->setText(address);
          }
        }
      }
    }

    catch (std::exception &)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Could not check Conceal ID"), QtCriticalMsg));
      return;
    }

    /* Check address validity */
    if (!CurrencyAdapter::instance().validateAddress(address))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
      return;
    }

    /* Start building the transaction */
    walletTransfer.address = address.toStdString();
    uint64_t amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountEdit->text());
    walletTransfer.amount = amount;
    walletTransfers.push_back(walletTransfer);
    QString label = m_ui->m_addressLabel->text();

    /* Check payment id validity */
    paymentIdString = m_ui->m_paymentIdEdit->text().toUtf8();
    if (!WalletAdapter::isValidPaymentId(paymentIdString))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
      return;
    }

    /* Warn the user if there is no payment id */
    if (paymentIdString.toStdString().length() < 64)
    {
      /* Is it an exchange address? */
      if (!exchangeName.isEmpty())
      {
        Q_EMIT notifySignal("This address belongs to " + exchangeName +
                            " and requires a Payment ID.\nPlease enter the "
                            "Payment ID provided by the exchange to proceed.");
        return;
      }
    }

    /* Add the comment to the transaction */
    QString comment = m_ui->m_messageEdit->text();
    if (!comment.isEmpty())
    {
      walletMessages.append(CryptoNote::TransactionMessage{comment.toStdString(), address.toStdString()});
    }

    quint64 actualFee = BASE_FEE;
    quint64 totalFee = BASE_FEE;

    /* Remote node fee */
    QString connection = Settings::instance().getConnection();
    if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
    {
      if (!OverviewFrame::remote_node_fee_address.isEmpty())
      {
        CryptoNote::WalletLegacyTransfer walletTransfer;
        walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
        walletTransfer.amount = REMOTE_FEE;
        walletTransfers.push_back(walletTransfer);
        totalFee = totalFee + REMOTE_FEE;
      }
    }

    /* Check if there are enough funds for the amount plus total fees */
    if (m_actualBalance < (amount + totalFee))
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Insufficient funds. Please ensure that you have enough funds for the amount plus fees."), QtCriticalMsg));
      return;
    }

    if (!checkWalletPassword())
    {
      return;
    }
    delay();

    /* If the wallet is open we proceed */
    if (WalletAdapter::instance().isOpen())
    {
      /* Send the transaction */
      WalletAdapter::instance().sendTransaction(walletTransfers, actualFee, paymentIdString, 4, walletMessages);
      /* Add to the address book if a label is given */
      if ((!label.isEmpty()) && (m_ui->m_saveAddress->isChecked()))
      {
        if (isIntegrated == true)
        {
          AddressBookModel::instance().addAddress(label, int_address, "");
        }
        else
        {
          AddressBookModel::instance().addAddress(label, address, paymentIdString);
        }
      }
    }
  }

  /* Once we complete a transaction, we either show the error or clear all fields and move back to the dashboard */
  void OverviewFrame::sendTransactionCompleted(CryptoNote::TransactionId _id, bool _error, const QString &_errorText)
  {
    Q_UNUSED(_id);
    if (_error)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(_errorText, QtCriticalMsg));
    }
    else
    {
      clearAllClicked();
      dashboardClicked();
    }
  }

  /* Open address book */
  void OverviewFrame::addressBookClicked()
  {
    m_ui->m_myConcealWalletTitle->setText(tr("ADDRESS BOOK"));
    m_ui->m_titleIcon->setPixmap(QPixmap(":/icons/icon-address"));
    m_ui->addressBookBox->raise();
  }

  /* Once we send a message, we either show the error or clear all fields and move back to the dashboard */
  void OverviewFrame::sendMessageCompleted(CryptoNote::TransactionId _id, bool _error, const QString &_errorText)
  {
    Q_UNUSED(_id);
    if (_error)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(_errorText, QtCriticalMsg));
    }
    else
    {
      clearMessageClicked();
      dashboardClicked();
    }
  }

  /* Clear all fields in the Send Message screen */
  void OverviewFrame::clearMessageClicked()
  {
    m_ui->m_messageTextEdit->clear();
    m_ui->m_addressMessageEdit->clear();
  }

  /* Generate the time display for the TTL change */
  void OverviewFrame::ttlValueChanged(int _ttlValue)
  {
    quint32 value = _ttlValue * MIN_TTL;
    quint32 hours = value / HOUR_SECONDS;
    quint32 minutes = value % HOUR_SECONDS / MINUTE_SECONDS;
    m_ui->m_ttlLabel->setText(QString("%1h %2m").arg(hours).arg(minutes));
  }

  /* When the address changes in the Send field, check if its from an exchange */
  void OverviewFrame::addressChanged(QString _address)
  {
    exchangeName = "";
    m_exchangeProvider->getExchange(_address);
  }

  /* Prevent users from sending message over 260 characters */
  void OverviewFrame::recalculateMessageLength()
  {

    if (m_ui->m_messageTextEdit->toPlainText().length() > 261)
    {
      m_ui->m_messageTextEdit->setPlainText(m_ui->m_messageTextEdit->toPlainText().left(m_ui->m_messageTextEdit->toPlainText().length() - 1));
      m_ui->m_messageTextEdit->moveCursor(QTextCursor::End);
      Q_EMIT notifySignal(tr("Warning: you have reached the maximum message size of 260 characters."));
    }

    QString messageText = m_ui->m_messageTextEdit->toPlainText();
    quint32 messageSize = messageText.length();

    if (messageSize > 0)
    {
      --messageSize;
    }

    m_ui->m_messageLength->setText(QString::number(messageSize));
  }

  void OverviewFrame::messageTextChanged()
  {
    recalculateMessageLength();
  }

  void OverviewFrame::sendMessageClicked()
  {

    if (!checkWalletPassword())
    {
      return;
    }
    delay();

    /* Exit if the wallet is not open */
    if (!WalletAdapter::instance().isOpen())
    {
      return;
    }

    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    QVector<CryptoNote::WalletLegacyTransfer> transfers;
    QVector<CryptoNote::WalletLegacyTransfer> feeTransfer;
    CryptoNote::WalletLegacyTransfer walletTransfer;
    QVector<CryptoNote::TransactionMessage> messages;
    QVector<CryptoNote::TransactionMessage> feeMessage;
    QString address = m_ui->m_addressMessageEdit->text().toUtf8();
    QString messageString = m_ui->m_messageTextEdit->toPlainText();

    try
    {
      /* Is it a Conceal ID? */
      if (CurrencyAdapter::instance().isValidOpenAliasAddress(address))
      {
        /* Parse the record and set address to the actual CCX address */
        std::vector<std::string> records;
        if (!Common::fetch_dns_txt(address.toStdString(), records))
        {
          QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Failed to lookup Conceal ID"), QtCriticalMsg));
        }
        std::string realAddress;
        for (const auto &record : records)
        {
          if (CurrencyAdapter::instance().processServerAliasResponse(record, realAddress))
          {
            address = QString::fromStdString(realAddress);
            m_ui->m_addressMessageEdit->setText(address);
          }
        }
      }
    }

    catch (std::exception &)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Could not check Conceal ID"), QtCriticalMsg));
      return;
    }

    /* Start building the transaction */
    walletTransfer.address = address.toStdString();
    uint64_t amount = 100;
    walletTransfer.amount = amount;
    transfers.push_back(walletTransfer);
    messages.append({messageString.toStdString(), address.toStdString()});

    /* Set fee */
    quint64 fee = BASE_FEE;

    /* Check if this is a self destructive message */
    bool selfDestructiveMessage = false;
    quint64 ttl = 0;
    if (m_ui->m_ttlCheck->checkState() == Qt::Checked)
    {
      ttl = QDateTime::currentDateTimeUtc().toTime_t() + m_ui->m_ttlSlider->value() * MIN_TTL;
      fee = 0;
      selfDestructiveMessage = true;
    }

    /* Add the remote node fee transfer to the transaction if the connection
     is a remote node with an address and this is not a self-destructive message */
    if ((!OverviewFrame::remote_node_fee_address.isEmpty()) && (selfDestructiveMessage == false))
    {
      QString connection = Settings::instance().getConnection();
      if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
      {
        CryptoNote::WalletLegacyTransfer walletTransfer;
        walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
        walletTransfer.amount = REMOTE_FEE;
        transfers.push_back(walletTransfer);
      }
    }

    QString messageText = m_ui->m_messageTextEdit->toPlainText();
    quint32 messageSize = messageText.length();
    if (messageSize > 0)
    {
      --messageSize;
    }

    if (messageSize > 260)
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Message too long. Please ensure that the message is less than 260 characters."), QtCriticalMsg));
      return;
    }

    /* Send the message. If it is a self-destructive message, send the fee transaction */
    if (WalletAdapter::instance().isOpen())
    {
      WalletAdapter::instance().sendMessage(transfers, fee, 4, messages, ttl);
    }
  }

  void OverviewFrame::addressBookMessageClicked()
  {
    AddressBookDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
      m_ui->m_addressMessageEdit->setText(dlg.getAddress());
    }
  }

  void OverviewFrame::aboutQTClicked()
  {
    Q_EMIT aboutQTSignal();
  }

  // DEPOSITS

  /* New deposit */
  void OverviewFrame::newDepositClicked()
  {

    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());

    /* Insufficient funds */
    if (amount == 0 || amount + CurrencyAdapter::instance().getMinimumFeeBanking() > WalletAdapter::instance().getActualBalance())
    {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("You don't have enough balance in your account!"), QtCriticalMsg));
      return;
    }

    if (!checkWalletPassword())
    {
      return;
    }

    uint32_t blocksPerDeposit = 21900;
    quint32 term = m_ui->m_timeSpin->value() * blocksPerDeposit;

    /* Warn the user */
    if (QMessageBox::warning(&MainWindow::instance(), tr("Deposit Confirmation"),
                             tr("Please note that once funds are locked in a deposit, you will not have access until maturity. Are you sure you want to proceed?"),
                             QMessageBox::Cancel,
                             QMessageBox::Ok) != QMessageBox::Ok)
    {
      return;
    }

    /* Initiate the desposit */
    WalletAdapter::instance().deposit(term, amount, BASE_FEE, 4);

    /* Remote node fee */
    QVector<CryptoNote::WalletLegacyTransfer> walletTransfers;
    QString connection = Settings::instance().getConnection();
    if ((connection.compare("remote") == 0) || (connection.compare("autoremote") == 0))
    {
      if (!OverviewFrame::remote_node_fee_address.isEmpty())
      {
        QVector<CryptoNote::TransactionMessage> walletMessages;
        CryptoNote::WalletLegacyTransfer walletTransfer;
        walletTransfer.address = OverviewFrame::remote_node_fee_address.toStdString();
        walletTransfer.amount = REMOTE_FEE;
        walletTransfers.push_back(walletTransfer);
        /* If the wallet is open we proceed */
        if (WalletAdapter::instance().isOpen())
        {
          /* Send the transaction */
          WalletAdapter::instance().sendTransaction(walletTransfers, BASE_FEE, "", 4, walletMessages);
        }
      }
    }
  }

  void OverviewFrame::showDepositDetails(const QModelIndex &_index)
  {
    if (!_index.isValid())
    {
      return;
    }
    m_ui->darkness->show();
    m_ui->darkness->raise();
    DepositDetailsDialog dlg(_index, this);
    dlg.exec();
    m_ui->darkness->hide();
  }

  void OverviewFrame::depositParamsChanged()
  {
    uint32_t blocksPerDeposit = 21900;
    quint64 amount = CurrencyAdapter::instance().parseAmount(m_ui->m_amountSpin->cleanText());
    quint32 term = m_ui->m_timeSpin->value() * blocksPerDeposit;
    quint64 interest = CurrencyAdapter::instance().calculateInterest(amount, term, NodeAdapter::instance().getLastKnownBlockHeight());
    qreal termRate = DepositModel::calculateRate(amount, interest);
    m_ui->m_interestEarnedLabel->setText(QString("%1 %2").arg(CurrencyAdapter::instance().formatAmount(interest)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
    m_ui->m_interestRateLabel->setText(QString("%3 %").arg(QString::number(termRate * 100, 'f', 4)));
  }

  void OverviewFrame::timeChanged(int _value)
  {
    m_ui->m_timeLabel->setText(monthsToBlocks(m_ui->m_timeSpin->value()));
  }

  void OverviewFrame::withdrawClicked()
  {
    QModelIndexList unlockedDepositIndexList = DepositModel::instance().match(DepositModel::instance().index(0, 0), DepositModel::ROLE_STATE, DepositModel::STATE_UNLOCKED, -1);
    if (unlockedDepositIndexList.isEmpty())
    {
      return;
    }

    QVector<CryptoNote::DepositId> depositIds;
    Q_FOREACH (const QModelIndex &index, unlockedDepositIndexList)
    {
      depositIds.append(index.row());
    }

    WalletAdapter::instance().withdrawUnlockedDeposits(depositIds, CurrencyAdapter::instance().getMinimumFeeBanking());
    actualBalanceUpdated(0);
    pendingBalanceUpdated(0);
    actualDepositBalanceUpdated(0);
    pendingDepositBalanceUpdated(0);
    actualInvestmentBalanceUpdated(0);
    pendingInvestmentBalanceUpdated(0);
  }

  void OverviewFrame::importSeedButtonClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT importSeedSignal();
    dashboardClicked();
  }

  void OverviewFrame::openWalletButtonClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT openWalletSignal();
    dashboardClicked();
  }

  void OverviewFrame::importTrackingButtonClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT importTrackingKeySignal();
    dashboardClicked();
  }

  void OverviewFrame::importPrivateKeysButtonClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT importSecretKeysSignal();
    dashboardClicked();
  }

  void OverviewFrame::createNewWalletButtonClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT newWalletSignal();
    dashboardClicked();
  }

  void OverviewFrame::goToWelcomeFrame(){
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT welcomeFrameSignal();
    dashboardClicked();
  }

  void OverviewFrame::backupClicked()
  {
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
      return;
    }

    if (!checkWalletPassword())
    {
      return;
    }

    Q_EMIT backupSignal();
  }

  void OverviewFrame::backupFileClicked()
  {

    if (!checkWalletPassword())
    {
      return;
    }

    Q_EMIT backupFileSignal();
  }

  void OverviewFrame::optimizeClicked()
  {
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
    }
    else
    {
      quint64 numUnlockedOutputs;
      numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
      WalletAdapter::instance().optimizeWallet();
      while (WalletAdapter::instance().getNumUnlockedOutputs() > 100)
      {
        numUnlockedOutputs = WalletAdapter::instance().getNumUnlockedOutputs();
        if (numUnlockedOutputs == 0)
          break;
        WalletAdapter::instance().optimizeWallet();
        delay();
      }
      dashboardClicked();
    }
  }

  void OverviewFrame::autoOptimizeClicked()
  {
    if (Settings::instance().isTrackingMode())
    {
      Q_EMIT notifySignal(tr("This is a tracking wallet.\nThis action is not available."));
    }
    else
    {
      if (Settings::instance().getAutoOptimizationStatus() == "enabled")
      {
        Settings::instance().setAutoOptimizationStatus("disabled");
        m_ui->b2_autoOptimizeButton->setText(tr("CLICK TO ENABLE"));
        Q_EMIT notifySignal(tr("Auto optimization disabled"));
      }
      else
      {
        Settings::instance().setAutoOptimizationStatus("enabled");
        m_ui->b2_autoOptimizeButton->setText(tr("CLICK TO DISABLE"));
        Q_EMIT notifySignal(
            tr("Auto optimization enabled.\nYour wallet will be optimized "
               "automatically every 15 minutes."));
      }
    }
  }

  void OverviewFrame::startMaximizedClicked()
  {
      if (Settings::instance().getMaximizedStatus() == "enabled")
      {
        Settings::instance().setMaximizedStatus("disabled");
        m_ui->b2_startMaximized->setText(tr("CLICK TO ENABLE"));
      }
      else
      {
        Settings::instance().setMaximizedStatus("enabled");
        m_ui->b2_startMaximized->setText(tr("CLICK TO DISABLE"));
      }
  }

  void OverviewFrame::saveLanguageCurrencyClicked()
  {
    QString language;
    if (m_ui->m_russian->isChecked())
    {
      language = "ru";
    }
    else if (m_ui->m_turkish->isChecked())
    {
      language = "tr";
    }
    else if (m_ui->m_chinese->isChecked())
    {
      language = "cn";
    }
    else
    {
      language = "en";
    }
    Settings::instance().setLanguage(language);

    /* Save the selected currency */
    Settings::instance().setCurrentCurrency(m_ui->m_language->currentText());

    Settings::instance().setFont(m_ui->m_font->currentText());

    Settings::instance().setFontSize(m_ui->m_size->currentText().toInt());

    loadChart();
    m_priceProvider->getPrice();

    QMessageBox::information(&MainWindow::instance(),
                             tr("Language and Currency settings saved"),
                             tr("Please restart the wallet for the new settings to take effect."),
                             QMessageBox::Ok);
  }

  void OverviewFrame::saveConnectionClicked()
  {
    QString connectionMode;
    if (m_ui->radioButton->isChecked())
    {
      connectionMode = "remote";
    }
    else if (m_ui->radioButton_2->isChecked())
    {
      connectionMode = "embedded";
    }
    else if (m_ui->radioButton_3->isChecked())
    {
      connectionMode = "autoremote";
    }
    Settings::instance().setConnection(connectionMode);

    QString remoteHost;
    /* If it is a remote connection, commit the entered remote node. There is no validation of the 
     remote node. If the connection is embedded then take no action */
    if (m_ui->radioButton->isChecked())
    {
      remoteHost = m_ui->m_hostEdit->text();
    }
    if (m_ui->radioButton_3->isChecked())
    {
      remoteHost = m_ui->m_hostEdit->text();
    }
    Settings::instance().setCurrentRemoteNode(remoteHost);

    QMessageBox::information(&MainWindow::instance(),
                             tr("Connection settings saved"),
                             tr("Please restart the wallet for the new settings to take effect."),
                             QMessageBox::Ok);
  }

  void OverviewFrame::rescanClicked()
  {
    Q_EMIT rescanSignal();
  }

  void OverviewFrame::delay()
  {
    QTime dieTime = QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < dieTime)
      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }

  void OverviewFrame::addABClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    ContactDialog dlg(&MainWindow::instance());
    dlg.exec();
    m_ui->darkness->hide();
  }

  void OverviewFrame::editABClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    ContactDialog dlg(&MainWindow::instance());
    dlg.edit(m_ui->m_addressBookView->currentIndex());
    if (dlg.exec() == QDialog::Accepted)
    {
      deleteABClicked();
    }
    m_ui->darkness->hide();
  }

  void OverviewFrame::copyABClicked()
  {
    QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
    Q_EMIT notifySignal(tr("Address copied to clipboard"));
  }

  void OverviewFrame::copyABPaymentIdClicked()
  {
    QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_PAYMENTID).toString());
    Q_EMIT notifySignal(tr("Payment ID copied to clipboard"));
  }

  void OverviewFrame::deleteABClicked()
  {
    int row = m_ui->m_addressBookView->currentIndex().row();
    AddressBookModel::instance().removeAddress(row);
    currentAddressChanged(m_ui->m_addressBookView->currentIndex());
  }

  void OverviewFrame::payToABClicked()
  {
    Q_EMIT payToSignal(m_ui->m_addressBookView->currentIndex());
  }

  /* Send the address from the address book when double clicked to either a new transfer or new message */
  void OverviewFrame::addressDoubleClicked(const QModelIndex &_index)
  {
    if (!_index.isValid())
    {
      return;
    }

    Q_EMIT payToSignal(_index);
    m_ui->darkness->hide();
  }

  /* Toggle states of buttons in the address book when a user clicks on an address */
  void OverviewFrame::currentAddressChanged(const QModelIndex &_index)
  {
    m_ui->b1_copyContactAddressButton->setEnabled(_index.isValid());
    m_ui->b1_copyContactPaymentIdButton->setEnabled(
        !_index.data(AddressBookModel::ROLE_PAYMENTID).toString().isEmpty());
    m_ui->b1_payToButton->setEnabled(_index.isValid());
    m_ui->b1_editContactButton->setEnabled(_index.isValid());
    m_ui->b1_deleteContactButton->setEnabled(_index.isValid());
  }

  /* Disable buttons in the address book when no contact is selected */
  void OverviewFrame::disableAddressBookButtons()
  {
    m_ui->b1_copyContactAddressButton->setEnabled(false);
    m_ui->b1_copyContactPaymentIdButton->setEnabled(false);
    m_ui->b1_payToButton->setEnabled(false);
    m_ui->b1_editContactButton->setEnabled(false);
    m_ui->b1_deleteContactButton->setEnabled(false);
  }

  /* Open URL's when contact us / stay informed buttons are clicked */

  void OverviewFrame::discordClicked()
  {
    QDesktopServices::openUrl(QUrl("https://discord.conceal.network/", QUrl::TolerantMode));
  }

  void OverviewFrame::twitterClicked()
  {
    QDesktopServices::openUrl(QUrl("https://twitter.com/ConcealNetwork", QUrl::TolerantMode));
  }

  void OverviewFrame::telegramClicked()
  {
    QDesktopServices::openUrl(QUrl("https://t.co/55klBHKGUR", QUrl::TolerantMode));
  }

  void OverviewFrame::githubClicked()
  {
    QDesktopServices::openUrl(QUrl("https://github.com/ConcealNetwork", QUrl::TolerantMode));
  }

  void OverviewFrame::redditClicked()
  {
    QDesktopServices::openUrl(QUrl("https://www.reddit.com/r/ConcealNetwork/", QUrl::TolerantMode));
  }

  void OverviewFrame::mediumClicked()
  {
    QDesktopServices::openUrl(QUrl("https://medium.com/@ConcealNetwork", QUrl::TolerantMode));
  }

  void OverviewFrame::helpDeskClicked()
  {
    QDesktopServices::openUrl(QUrl("https://conceal.network/support/", QUrl::TolerantMode));
  }

  void OverviewFrame::stexClicked()
  {
    QDesktopServices::openUrl(QUrl("https://app.stex.com/en/basic-trade/pair/BTC/CCX", QUrl::TolerantMode));
  }

  void OverviewFrame::tradeogreClicked()
  {
    QDesktopServices::openUrl(QUrl("https://tradeogre.com/exchange/BTC-CCX", QUrl::TolerantMode));
  }

  void OverviewFrame::wikiClicked()
  {
    QDesktopServices::openUrl(QUrl("https://conceal.network/wiki/doku.php?id=start", QUrl::TolerantMode));
  }

  void OverviewFrame::helpClicked()
  {
    QDesktopServices::openUrl(QUrl("https://conceal.network/wiki/doku.php?id=start", QUrl::TolerantMode));
  }

  /* Initiate a password prompt meant for critical tasks like sending funds etc */
  bool OverviewFrame::checkWalletPassword(bool _error)
  {
    if (!Settings::instance().isEncrypted() && WalletAdapter::instance().checkWalletPassword(""))
      return true;

    m_ui->darkness->show();
    m_ui->darkness->raise();

    PasswordDialog dlg(_error, this);
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::FramelessWindowHint);
    dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);
    if (dlg.exec() == QDialog::Accepted)
    {
      QString password = dlg.getPassword();
      if (!WalletAdapter::instance().checkWalletPassword(password))
      {
        return checkWalletPassword(true);
      }
      else
      {
        m_ui->darkness->hide();
        return true;
      }
    }
    m_ui->darkness->hide();
    return false;
  }

  /* Lock the wallet after prompting for confirmation */
  void OverviewFrame::lockWallet()
  {

    /* Return if the wallet is not encrypted */
    if (!Settings::instance().isEncrypted() && WalletAdapter::instance().checkWalletPassword(""))
      return;

    if (QMessageBox::warning(&MainWindow::instance(), tr("Lock Wallet"),
                             tr("Would you like to lock your wallet? While your wallet is locked, it will continue to synchronize with the network. You will need to enter your wallet password to unlock it."),
                             QMessageBox::Cancel,
                             QMessageBox::Ok) != QMessageBox::Ok)
    {
      return;
    }
    m_ui->lockBox->show();
    m_ui->lockBox->raise();
  }

  void OverviewFrame::change() {
    m_ui->lockBox->hide();
    m_ui->darkness->show();
    m_ui->darkness->raise();
    welcomeFrameSignal();
  }

  bool OverviewFrame::askForWalletPassword(bool _error) {
    if (!Settings::instance().isEncrypted() && WalletAdapter::instance().checkWalletPassword(""))
      return true;

    m_ui->darkness->show();
    m_ui->darkness->raise();

    MainPasswordDialog dlg(_error, this);
    connect(&dlg, &MainPasswordDialog::changeSignal, this, &OverviewFrame::change);
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::FramelessWindowHint);
    dlg.move((this->width() - dlg.width()) / 2, (height() - dlg.height()) / 2);

    if (dlg.exec() == QDialog::Accepted) {
      QString password = dlg.getPassword();
      if (!WalletAdapter::instance().checkWalletPassword(password)) {
        return askForWalletPassword(true);
      } else {
        m_ui->darkness->hide();
        return true;
      }
    }
    m_ui->darkness->hide();
    return false;
  }

  /* Unlock the wallet with the password */
  void OverviewFrame::unlockWallet()
  {
    if (!askForWalletPassword())
    {
      return;
    }
    m_ui->lockBox->hide();
  }

  /* Load the wallet encryption dialog */
  void OverviewFrame::encryptWalletClicked()
  {
    m_ui->darkness->show();
    m_ui->darkness->raise();
    Q_EMIT encryptWalletSignal();
    dashboardClicked();
  }

  /* When a user clicks on one of the recent transactions, we redirect to the transaction history and the specific record */
  void OverviewFrame::scrollToTransaction(const QModelIndex &_index)
  {
    transactionHistoryClicked();
    QModelIndex sortedModelIndex = SortedTransactionsModel::instance().mapFromSource(_index);
    QModelIndex index = static_cast<QSortFilterProxyModel *>(m_ui->m_transactionsView->model())->mapFromSource(sortedModelIndex);
    m_ui->m_transactionsView->scrollTo(index);
    m_ui->m_transactionsView->setFocus();
    m_ui->m_transactionsView->setCurrentIndex(index);
  }

  /* Export the transaction history to a CSV file */
  void OverviewFrame::exportCSV()
  {
    QString file = QFileDialog::getSaveFileName(&MainWindow::instance(), tr("Select CSV file"), QDir::homePath(), "CSV (*.csv)");
    if (!file.isEmpty())
    {
      QByteArray csv = TransactionsModel::instance().toCsv();
      QFile f(file);
      if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
        f.write(csv);
        f.close();
      }
    }
  }

  void OverviewFrame::minToTrayClicked()
  {
#ifndef QT_NO_SYSTEMTRAYICON
    if (!Settings::instance().isMinimizeToTrayEnabled())
    {
      Settings::instance().setMinimizeToTrayEnabled(true);
      m_ui->b2_minToTrayButton->setText(tr("CLICK TO DISABLE"));
    }
    else
    {
      Settings::instance().setMinimizeToTrayEnabled(false);
      m_ui->b2_minToTrayButton->setText(tr("CLICK TO ENABLE"));
    }
#endif
  }

  void OverviewFrame::closeToTrayClicked()
  {
#ifndef QT_NO_SYSTEMTRAYICON
    if (!Settings::instance().isCloseToTrayEnabled())
    {
      Settings::instance().setCloseToTrayEnabled(true);
      m_ui->b2_closeToTrayButton->setText(tr("CLICK TO DISABLE"));
    }
    else
    {
      Settings::instance().setCloseToTrayEnabled(false);
      m_ui->b2_closeToTrayButton->setText(tr("CLICK TO ENABLE"));
    }
#endif
  }

  void OverviewFrame::qtChartsLicenseClicked()
  {
    QDesktopServices::openUrl(QUrl("https://github.com/qt/qtcharts/blob/dev/LICENSE.GPL3"));
  }

  void OverviewFrame::openSslLicenseClicked()
  {
    QDesktopServices::openUrl(
        QUrl("https://github.com/openssl/openssl/blob/OpenSSL_1_1_1-stable/LICENSE"));
  }

  void OverviewFrame::autoRefreshButtonClicked()
  {
    if (Settings::instance().isAutoRefreshData())
    {
      Settings::instance().setAutoRefreshData(false);
      refreshDataTimer.stop();
      m_ui->b2_autoRefreshDataButton->setText(tr("CLICK TO ENABLE"));
    }
    else
    {
      Settings::instance().setAutoRefreshData(true);
      refreshDataTimer.start(REFRESH_INTERVAL);
      m_ui->b2_autoRefreshDataButton->setText(tr("CLICK TO DISABLE"));
    }
  }

}  // namespace WalletGui

#include "OverviewFrame.moc"
