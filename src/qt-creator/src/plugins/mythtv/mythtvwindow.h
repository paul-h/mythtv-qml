#ifndef MYTHTVWINDOW_H
#define MYTHTVWINDOW_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Mythtv {
namespace Internal {

class MythtvWindow : public QWidget
{
    Q_OBJECT

public:
    MythtvWindow(QWidget *parent = 0);
};

} // namespace Internal
} // namespace Mythtv

#endif // MYTHTVWINDOW_H
