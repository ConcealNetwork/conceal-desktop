#include <QFontDatabase>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "Notification.h"
#include "Settings.h"

#include "ui_notification.h"

namespace WalletGui {

Notification::Notification(QWidget* parent)
    : QDialog(parent), m_ui(new Ui::Notification) {
  this->parent = parent;
  m_ui->setupUi(this);
  int startingFontSize = Settings::instance().getFontSize();
  setStyles(startingFontSize);
  hide();
  setWindowFlags(Qt::FramelessWindowHint);
  timer = new QTimer();
}

Notification::~Notification() {
  delete m_ui;
  delete timer;
}

void Notification::fadeOut() {
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

void Notification::notify(const QString& message) {
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

void Notification::setStyles(int change) {
  /** Set the base font sizes */
  int baseFontSize = change;
  int baseTitleSize = 7 + change;
  int baseSmallButtonSize = change - 3;
  int baseLargeButtonSize = change - 1;

  int id;

  QString currentFont = Settings::instance().getFont();
  if (currentFont == "Poppins") {
    id = QFontDatabase::addApplicationFont(":/fonts/Poppins-Regular.ttf");
  } else if (currentFont == "Lekton") {
    id = QFontDatabase::addApplicationFont(":/fonts/Lekton-Regular.ttf");
  } else if (currentFont == "Roboto") {
    id = QFontDatabase::addApplicationFont(":/fonts/RobotoSlab-Regular.ttf");
  } else if (currentFont == "Montserrat") {
    id = QFontDatabase::addApplicationFont(":/fonts/Montserrat-Regular.ttf");
  } else if (currentFont == "Open Sans") {
    id = QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
  } else if (currentFont == "Oswald") {
    id = QFontDatabase::addApplicationFont(":/fonts/Oswald-Regular.ttf");
  } else if (currentFont == "Lato") {
    id = QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
  }

  QFont font;
  font.setFamily(currentFont);
  font.setPixelSize(baseFontSize);
  font.setLetterSpacing(QFont::PercentageSpacing, 102);
  font.setHintingPreference(QFont::PreferFullHinting);
  font.setStyleStrategy(QFont::PreferAntialias);

  m_ui->notification->setFont(font);
}
}  // namespace WalletGui