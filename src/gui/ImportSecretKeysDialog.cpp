// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportSecretKeysDialog.h"

#include <Common/StringTools.h>
#include <CryptoNoteCore/Account.h>

#include <QFileDialog>
#include <QFont>
#include <QRegExpValidator>

#include "Settings.h"
#include "WalletAdapter.h"
#include "ui_importsecretkeysdialog.h"

namespace WalletGui
{
  ImportSecretKeysDialog::ImportSecretKeysDialog(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::ImportSecretKeysDialog)
  {
    m_ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint);
    if (_parent != nullptr)
    {
      move((_parent->width() - width()) / 2, (_parent->height() - height()) / 2);
    }
    QRegExp hexMatcher("^[0-9A-F]{64}$", Qt::CaseInsensitive);
    QValidator *validator = new QRegExpValidator(hexMatcher, this);
    m_ui->m_spendKey->setValidator(validator);
    m_ui->m_viewKey->setValidator(validator);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ImportSecretKeysDialog::~ImportSecretKeysDialog() { }

  QString ImportSecretKeysDialog::getSpendKeyString() const
  {
    return m_ui->m_spendKey->text().trimmed();
  }

  QString ImportSecretKeysDialog::getViewKeyString() const
  {
    return m_ui->m_viewKey->text().trimmed();
  }

  QString ImportSecretKeysDialog::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportSecretKeysDialog::selectPathClicked()
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

  void ImportSecretKeysDialog::setErrorMessage(QString message)
  {
    m_ui->m_errorLabel->setText(message);
  }

  void ImportSecretKeysDialog::clearErrorMessage() { m_ui->m_errorLabel->setText(""); }

  void ImportSecretKeysDialog::importButtonClicked()
  {
    QString spendKey = getSpendKeyString();
    QString viewKey = getViewKeyString();
    QString filePath = getFilePath();

    if (spendKey.size() != 64)
    {
      setErrorMessage(
          tr("Private Spend Key is not valid. The private spend key you entered is not valid."));
      return;
    }

    if (viewKey.size() != 64)
    {
      setErrorMessage(
          tr("Private View Key is not valid. The private view key you entered is not valid."));
      return;
    }

    if (QFile::exists(filePath))
    {
      setErrorMessage(
          tr("The wallet file already exists. Please change the wallet path and try again."));
      return;
    }

    std::string privateSpendKeyString = spendKey.toStdString();
    std::string privateViewKeyString = viewKey.toStdString();

    crypto::Hash privateSpendKeyHash;
    crypto::Hash privateViewKeyHash;

    size_t size;
    if (!common::fromHex(privateSpendKeyString, &privateSpendKeyHash, sizeof(privateSpendKeyHash),
                         size) ||
        size != sizeof(privateSpendKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The private spend key you entered is not valid."));
      return;
    }
    if (!common::fromHex(privateViewKeyString, &privateViewKeyHash, sizeof(privateViewKeyHash),
                         size) ||
        size != sizeof(privateViewKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The private view key you entered is not valid."));
      return;
    }
    crypto::PublicKey publicSpendKey;
    crypto::PublicKey publicViewKey;
    crypto::SecretKey privateSpendKey = *(struct crypto::SecretKey *)&privateSpendKeyHash;
    crypto::SecretKey privateViewKey = *(struct crypto::SecretKey *)&privateViewKeyHash;

    crypto::secret_key_to_public_key(privateSpendKey, publicSpendKey);
    crypto::secret_key_to_public_key(privateViewKey, publicViewKey);

    cn::AccountPublicAddress publicKeys;
    publicKeys.spendPublicKey = publicSpendKey;
    publicKeys.viewPublicKey = publicViewKey;

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

  QList<QWidget *> ImportSecretKeysDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportSecretKeysDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportSecretKeysDialog::getLabels()
  {
    return m_ui->groupBox->findChildren<QLabel *>();
  }

  void ImportSecretKeysDialog::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui