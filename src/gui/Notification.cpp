// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Notification.h"

#include <QFontDatabase>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "Settings.h"
#include "ui_notification.h"

namespace WalletGui
{
  Notification::Notification(QWidget* parent) : QDialog(parent), m_ui(new Ui::Notification)
  {
    this->parent = parent;
    m_ui->setupUi(this);
    Settings& settings = Settings::instance();
    setStyles(settings.getFontSize());
    hide();
    setWindowFlags(Qt::FramelessWindowHint);
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
  }

  Notification::Notification(const Notification& notification)
  {
    parent = notification.parent;
    m_ui = new Ui::Notification;
    *m_ui = *notification.m_ui;
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
  }

  Notification& Notification::operator=(const Notification& notification)
  {
    if (this != &notification)
    {
      delete timer;
      timer = new QTimer(this);
      timer->setSingleShot(true);
      connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
      parent = notification.parent;
      delete m_ui;
      m_ui = new Ui::Notification;
      *m_ui = *notification.m_ui;
    }
    return *this;
  }

  Notification::~Notification()
  {
    delete m_ui;
  }

  void Notification::fadeOut()
  {
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity", this);
    setGraphicsEffect(effect);
    animation->setDuration(500);
    animation->setStartValue(1);
    animation->setEndValue(0);
    animation->setEasingCurve(QEasingCurve::OutBack);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
      setGraphicsEffect(nullptr);
      hide();
    });
  }

  void Notification::notify(const QString& message)
  {
    int nLines = message.count('\n') + 1;
    Settings& settings = Settings::instance();
    int minimumSize = nLines * settings.getFontSize();
    minimumSize = minimumSize > 25 ? minimumSize : 25;
    m_ui->notification->setText(message);
    m_ui->notification->setMinimumHeight(minimumSize);
    m_ui->notification->setMinimumWidth(minimumSize * 15);
    adjustSize();
    move((parent->width() - width()) / 2, parent->height() - 100);
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    setGraphicsEffect(effect);
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->setEasingCurve(QEasingCurve::InBack);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    show();
    timer->stop();
    timer->start(2000 * nLines);
  }

  void Notification::applyStyles() { m_ui->notification->setFont(EditableStyle::currentFont); }
}  // namespace WalletGui