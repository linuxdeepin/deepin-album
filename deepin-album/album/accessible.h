#include "accessibledefine.h"
#include "mainwindow.h"
#include "albumview.h"

#include <DImageButton>
#include <DSwitchButton>
#include <DPushButton>
DWIDGET_USE_NAMESPACE

// 添加accessible

SET_BUTTON_ACCESSIBLE(DPushButton, "dpushbutton");
SET_MENU_ACCESSIBLE(DMenu, "dmenu");
SET_WIDGET_ACCESSIBLE(DMainWindow, QAccessible::Form, "main");

QAccessibleInterface *accessibleFactory(const QString &classname, QObject *object)
{
    QAccessibleInterface *interface = nullptr;
    USE_ACCESSIBLE(classname, DPushButton);
    USE_ACCESSIBLE(classname, DMenu);

    return interface;
}
