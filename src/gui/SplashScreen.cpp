// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2017-2018 The Circle Foundation & Conceal Devs
// Copyright (c) 2018-2023 Conceal Network & Conceal Devs

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
  setWindowIcon(QIcon(":/images/conceal-logo"));
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

void SplashScreen::centerOnScreen(QApplication* app)
{
  // Initialize to false - only set to true if evaluation succeeds
  bool isGnomeMutter = false;
  
  // Safely evaluate environment to detect GNOME/Mutter
  try {
    QString desktopEnv = qgetenv("XDG_CURRENT_DESKTOP");
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    
    isGnomeMutter = (sessionType == "wayland" && 
                    (desktopEnv.contains("GNOME") || desktopEnv.contains("ubuntu:GNOME") || 
                     desktopEnv.contains("gnome") || desktopEnv.contains("Ubuntu"))); 
  } catch (...) {
    QRect availableGeometry = app->primaryScreen() ? app->primaryScreen()->availableGeometry() : QRect(0, 0, 1920, 1080);
    setGeometry(QStyle::alignedRect(
        Qt::LeftToRight, Qt::AlignCenter, size(), availableGeometry));
    return;
  }
  
  if (isGnomeMutter) {
    // GNOME/Mutter way: use move() with computed values
    QScreen *screen = app->primaryScreen();
    if (screen) {
      QRect availableGeometry = screen->availableGeometry();
      QSize splashSize = size();
      
      int x = (availableGeometry.width() - splashSize.width()) / 2;
      int y = (availableGeometry.height() - splashSize.height()) / 2;
      
      x = qMax(0, qMin(x, availableGeometry.width() - splashSize.width()));
      y = qMax(0, qMin(y, availableGeometry.height() - splashSize.height()));
      
      move(availableGeometry.x() + x, availableGeometry.y() + y);
      return;
    }
  }
    // Traditional way for non-GNOME/Mutter
    QRect availableGeometry = app->primaryScreen() ? app->primaryScreen()->availableGeometry() : QRect(0, 0, 1920, 1080);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), availableGeometry));
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
