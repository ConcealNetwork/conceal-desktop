// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ImportTrackingDialog.h"

#include <Common/StringTools.h>
#include <CryptoNoteCore/Account.h>

#include <QFileDialog>
#include <QFont>
#include <QRegExpValidator>

#include "Settings.h"
#include "WalletAdapter.h"
#include "ui_importtrackingdialog.h"

namespace WalletGui
{
  ImportTrackingDialog::ImportTrackingDialog(QWidget *_parent)
      : QDialog(_parent), m_ui(new Ui::ImportTrackingDialog)
  {
    m_ui->setupUi(this);
    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint);
    if (_parent != nullptr)
    {
      move((_parent->width() - width()) / 2, (_parent->height() - height()) / 2);
    }
    QRegExp hexMatcher("^[0-9A-F]{256}$", Qt::CaseInsensitive);
    QValidator *validator = new QRegExpValidator(hexMatcher, this);
    m_ui->m_trackingKey->setValidator(validator);
    m_ui->m_pathEdit->setText(Settings::instance().getDefaultWalletPath());
    EditableStyle::setStyles(Settings::instance().getFontSize());
  }

  ImportTrackingDialog::~ImportTrackingDialog() { }

  QString ImportTrackingDialog::getKeyString() const
  {
    return m_ui->m_trackingKey->text().trimmed();
  }

  QString ImportTrackingDialog::getFilePath() const { return m_ui->m_pathEdit->text().trimmed(); }

  void ImportTrackingDialog::selectPathClicked()
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

  void ImportTrackingDialog::setErrorMessage(QString message)
  {
    m_ui->m_errorLabel->setText(message);
  }

  void ImportTrackingDialog::clearErrorMessage() { m_ui->m_errorLabel->setText(""); }

  void ImportTrackingDialog::importButtonClicked()
  {
    QString keyString = getKeyString();
    QString filePath = getFilePath();

    if (keyString.size() != 256)
    {
      setErrorMessage(tr("Tracking key is not valid. The tracking key you entered is not valid."));
      return;
    }

    if (QFile::exists(filePath))
    {
      setErrorMessage(
          tr("The wallet file already exists. Please change the wallet path and try again."));
      return;
    }

    CryptoNote::AccountKeys keys;

    std::string publicSpendKeyString = keyString.mid(0, 64).toStdString();
    std::string publicViewKeyString = keyString.mid(64, 64).toStdString();
    std::string privateSpendKeyString = keyString.mid(128, 64).toStdString();
    std::string privateViewKeyString = keyString.mid(192, 64).toStdString();

    Crypto::Hash publicSpendKeyHash;
    Crypto::Hash publicViewKeyHash;
    Crypto::Hash privateSpendKeyHash;
    Crypto::Hash privateViewKeyHash;

    size_t size;
    if (!Common::fromHex(publicSpendKeyString, &publicSpendKeyHash, sizeof(publicSpendKeyHash),
                         size) ||
        size != sizeof(publicSpendKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The public spend key you entered is not valid."));
      return;
    }
    if (!Common::fromHex(publicViewKeyString, &publicViewKeyHash, sizeof(publicViewKeyHash),
                         size) ||
        size != sizeof(publicViewKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The public view key you entered is not valid."));
      return;
    }
    if (!Common::fromHex(privateSpendKeyString, &privateSpendKeyHash, sizeof(privateSpendKeyHash),
                         size) ||
        size != sizeof(privateSpendKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The private spend key you entered is not valid."));
      return;
    }
    if (!Common::fromHex(privateViewKeyString, &privateViewKeyHash, sizeof(privateViewKeyHash),
                         size) ||
        size != sizeof(privateViewKeyHash))
    {
      setErrorMessage(tr("Key is not valid. The private view key you entered is not valid."));
      return;
    }

    Crypto::PublicKey publicSpendKey = *(struct Crypto::PublicKey *)&publicSpendKeyHash;
    Crypto::PublicKey publicViewKey = *(struct Crypto::PublicKey *)&publicViewKeyHash;
    Crypto::SecretKey privateSpendKey = *(struct Crypto::SecretKey *)&privateSpendKeyHash;
    Crypto::SecretKey privateViewKey = *(struct Crypto::SecretKey *)&privateViewKeyHash;

    keys.address.spendPublicKey = publicSpendKey;
    keys.address.viewPublicKey = publicViewKey;
    keys.spendSecretKey = privateSpendKey;
    keys.viewSecretKey = privateViewKey;

    if (WalletAdapter::instance().isOpen())
    {
      WalletAdapter::instance().close();
    }
    Settings::instance().setTrackingMode(true);
    WalletAdapter::instance().setWalletFile(filePath);
    WalletAdapter::instance().createWithKeys(keys);
    accept();
  }

  QList<QWidget *> ImportTrackingDialog::getWidgets()
  {
    return m_ui->groupBox->findChildren<QWidget *>();
  }

  QList<QPushButton *> ImportTrackingDialog::getButtons()
  {
    return m_ui->groupBox->findChildren<QPushButton *>();
  }

  QList<QLabel *> ImportTrackingDialog::getLabels()
  {
    return m_ui->groupBox->findChildren<QLabel *>();
  }

  void ImportTrackingDialog::applyStyles() { m_ui->groupBox->update(); }
}  // namespace WalletGui
