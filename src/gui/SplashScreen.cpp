// Copyright (c) 2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "SplashScreen.h"

#include <QtWidgets>

SplashScreen::SplashScreen(QWidget* parent) : QWidget(parent)
{
  QFont splashFont;
  splashFont.setFamily("Arial");
  splashFont.setBold(true);
  splashFont.setPixelSize(9);
  splashFont.setStretch(125);

  image = new QLabel();
  image->setPixmap(QPixmap(":/images/splash"));

  text = new QLabel();
  palette.setColor(QPalette::WindowText, Qt::darkGray);
  text->setAutoFillBackground(true);
  text->setPalette(palette);
  text->setText(QObject::tr("STARTING WALLET"));
  text->setAlignment(Qt::AlignCenter);
  text->setFont(splashFont);

  minimizeButton = new QPushButton();
  minimizeButton->setIcon(QIcon(":/icons/minimize"));
  minimizeButton->setStyleSheet("text-align: right; border: none;");

  connect(minimizeButton, SIGNAL(released()), this, SLOT(minimize()));

  layout = new QVBoxLayout();
  layout->addWidget(minimizeButton);
  layout->addWidget(image);
  layout->addWidget(text);
  setLayout(layout);

  setFixedSize(500, 325);

  setStyleSheet("background-color: #282d31;");
  setWindowIcon(QIcon(":/images/cryptonote"));
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

void SplashScreen::centerOnScreen(QApplication* app)
{
  setGeometry(QStyle::alignedRect(
      Qt::LeftToRight, Qt::AlignCenter, size(), app->desktop()->availableGeometry()));
}

void SplashScreen::showMessage(const QString& message, Qt::Alignment alignment, const QColor& color)
{
  palette.setColor(QPalette::WindowText, color);
  text->setAutoFillBackground(true);

  text->setPalette(palette);
  text->setText(message);
  text->setAlignment(alignment);
}

// Qt's waitForWindowExposed() implementation from QSplashScreen
// https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/widgets/qsplashscreen.cpp
inline static bool waitForWindowExposed(QWindow* window, int timeout = 1000)
{
  enum { TimeOutMs = 10 };
  QElapsedTimer timer;
  timer.start();
  while (!window->isExposed()) {
    const int remaining = timeout - int(timer.elapsed());
    if (remaining <= 0)
      break;
    QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
#if defined(Q_OS_WIN)
    Sleep(uint(TimeOutMs));
#else
    struct timespec ts = {TimeOutMs / 1000, (TimeOutMs % 1000) * 1000 * 1000};
    nanosleep(&ts, nullptr);
#endif
  }
  return window->isExposed();
}

/*!
    Makes the splash screen wait until the widget \a mainWin is displayed
    before calling close() on itself.
*/
// Qt's finish() implementation from QSplashScreen
// https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/widgets/qsplashscreen.cpp
void SplashScreen::finish(QWidget* mainWin)
{
  if (mainWin) {
    if (!mainWin->windowHandle())
      mainWin->createWinId();
    waitForWindowExposed(mainWin->windowHandle());
  }
  close();
}

void SplashScreen::minimize()
{
  showMinimized();
}
