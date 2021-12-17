// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportSeedDialog.h"

#include <CryptoNoteCore/Account.h>
#include <Mnemonics/electrum-words.h>

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <boost/algorithm/string.hpp>

#include "Settings.h"
#include "WalletAdapter.h"
#include "ui_importseeddialog.h"

namespace WalletGui
{
  ImportSeedDialog::ImportSeedDialog(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::ImportSeedDialog)
  {
    m_ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint);
    if (_parent != nullptr)
    {
      move((_parent->width() - width()) / 2, (_parent->height() - height()) / 2);
    }
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ImportSeedDialog::~ImportSeedDialog() { }

  QString ImportSeedDialog::getSeed() const { return m_ui->m_seed->toPlainText().simplified(); }

  QString ImportSeedDialog::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportSeedDialog::selectPathClicked()
  {
    QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"),
                                                    Settings::instance().getDefaultWalletPath(),
                                                    tr("Wallets (*.wallet)"));
    if (!filePath.isEmpty() && !filePath.endsWith(".wallet"))
    {
      filePath.append(".wallet");
    }
    m_ui->m_pathEdit->setText(filePath);
  }

  void ImportSeedDialog::setErrorMessage(QString message) { m_ui->m_errorLabel->setText(message); }

  void ImportSeedDialog::clearErrorMessage() { m_ui->m_errorLabel->setText(""); }

  void ImportSeedDialog::importButtonClicked()
  {
    static std::string language = "English";
    static const int mnemonicPhraseLength = 25;
    QString seed = getSeed();
    QString filePath = getFilePath();
    int wordCount = seed.split(" ").size();
    if (wordCount != mnemonicPhraseLength)
    {
      setErrorMessage(tr("Invalid seed. Seed phrase is not 25 words! Please try again."));
      return;
    }

    if (QFile::exists(filePath))
    {
      setErrorMessage(
          tr("The wallet file already exists. Please change the wallet path and try again."));
      return;
    }

    std::string mnemonicPhrase = seed.toStdString();

    std::vector<std::string> words;
    boost::split(words, mnemonicPhrase, ::isspace);

    crypto::SecretKey privateSpendKey;
    crypto::SecretKey privateViewKey;

    bool created = crypto::electrum_words::words_to_bytes(mnemonicPhrase, privateSpendKey, language);

    if (!created)
    {
      setErrorMessage(tr("Invalid seed. Please check your seed and try again."));
      return;
    }

    crypto::PublicKey unused;

    cn::AccountBase::generateViewFromSpend(privateSpendKey, privateViewKey, unused);

    crypto::PublicKey spendPublicKey;
    crypto::PublicKey viewPublicKey;
    crypto::secret_key_to_public_key(privateSpendKey, spendPublicKey);
    crypto::secret_key_to_public_key(privateViewKey, viewPublicKey);

    cn::AccountPublicAddress publicKeys;
    publicKeys.spendPublicKey = spendPublicKey;
    publicKeys.viewPublicKey = viewPublicKey;

    cn::AccountKeys keys;
    keys.address = publicKeys;
    keys.spendSecretKey = privateSpendKey;
    keys.viewSecretKey = privateViewKey;

    if (WalletAdapter::instance().isOpen())
    {
      WalletAdapter::instance().close();
    }

    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createWithKeys(keys);
    accept();
  }

  QList<QWidget *> ImportSeedDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportSeedDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportSeedDialog::getLabels() { return m_ui->groupBox->findChildren<QLabel *>(); }

  void ImportSeedDialog::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
