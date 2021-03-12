// Copyright (c) 2021 Conceal Network & Conceal Devs
//
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
    int startingFontSize = Settings::instance().getFontSize();
    setStyles(startingFontSize);
    hide();
    setWindowFlags(Qt::FramelessWindowHint);
    timer = new QTimer();
  }

  Notification::Notification(const Notification& notification)
  {
    timer = new QTimer();
    timer = notification.timer;
    parent = notification.parent;
    m_ui = new Ui::Notification;
    *m_ui = *notification.m_ui;
  }

  Notification& Notification::operator=(const Notification& notification)
  {
    if (this != &notification)
    {
      timer = new QTimer();
      timer = notification.timer;
      parent = notification.parent;
      m_ui = new Ui::Notification;
      *m_ui = *notification.m_ui;
    }
    return *this;
  }

  Notification::~Notification()
  {
    delete m_ui;
    delete timer;
  }

  void Notification::fadeOut()
  {
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
    setGraphicsEffect(effect);
    animation->setDuration(500);
    animation->setStartValue(1);
    animation->setEndValue(0);
    animation->setEasingCurve(QEasingCurve::OutBack);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    connect(animation, SIGNAL(finished()), this, SLOT(hide()));
  }

  void Notification::notify(const QString& message)
  {
    int nLines = message.split("\n").size();
    int minimumSize = nLines * Settings::instance().getFontSize();
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
    connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
    timer->start(2000 * nLines);
  }

  void Notification::applyStyles() { m_ui->notification->setFont(EditableStyle::font); }
}  // namespace WalletGui