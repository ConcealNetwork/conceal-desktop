#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class Notification;
}

namespace WalletGui {
class Notification : public QDialog {
  Q_OBJECT

 public:
  explicit Notification(QWidget* parent = nullptr);
  ~Notification();
  void notify(const QString& message);

 private:
  QWidget* parent;
  Ui::Notification* m_ui;
  QTimer* timer;
  void fadeOut();
  void setStyles(int change);
};
}  // namespace WalletGui
#endif  // NOTIFICATION_H
