// Copyright (c) 2020 Conceal Network & Conceal Devs
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

class SplashScreen : public QWidget {
  Q_OBJECT

public:
  SplashScreen(QWidget* parent = 0);
  void centerOnScreen(QApplication* app);
  void showMessage(
      const QString& message,
      Qt::Alignment alignment = Qt::AlignLeft,
      const QColor& color = Qt::black);
  void finish(QWidget* mainWin);

private slots:
  void minimize();

private:
  QPushButton* minimizeButton;
  QLabel* text;
  QLabel* image;
  QVBoxLayout* layout;
  QPalette palette;
};

#endif
