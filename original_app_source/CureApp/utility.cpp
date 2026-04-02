/*
    Copyright 2017 - 2019 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */
//#include <QApplication>
#include <QGuiApplication>

//#include <QCoreApplication>
//#include <QJniObject>

#include "utility.h"


void Sleep1SecAndProcessEvents() {
    QTime dieTime= QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}


Utility::Utility(QObject *parent) : QObject(parent)
{

}


#ifdef Q_OS_ANDROID

#include <QtAndroid>
#include <QAndroidJniEnvironment>

int getPermissions() {
    QStringList neededPermissions;

    if (QtAndroid::checkPermission("android.permission.ACCESS_COARSE_LOCATION")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.ACCESS_COARSE_LOCATION";
    if (QtAndroid::checkPermission("android.permission.ACCESS_FINE_LOCATION")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.ACCESS_FINE_LOCATION";
    if (QtAndroid::checkPermission("android.permission.BLUETOOTH")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.BLUETOOTH";
    if (QtAndroid::checkPermission("android.permission.BLUETOOTH_ADMIN")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.BLUETOOTH_ADMIN";
    if (QtAndroid::checkPermission("android.permission.BLUETOOTH_SCAN")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.BLUETOOTH_SCAN";
    if (QtAndroid::checkPermission("android.permission.BLUETOOTH_CONNECT")==QtAndroid::PermissionResult::Denied)
        neededPermissions<<"android.permission.BLUETOOTH_CONNECT";


    if (neededPermissions.size()>0) {
        QtAndroid::requestPermissionsSync(neededPermissions);
    }

    return neededPermissions.size();
}


void keep_screen_on(bool on) {
  QtAndroid::runOnAndroidThread([on]{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (activity.isValid()) {
      QAndroidJniObject window =
          activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

      if (window.isValid()) {
        const int FLAG_KEEP_SCREEN_ON = 128;
        if (on) {
          window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        } else {
          window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
      }
    }
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
  });
}
/*

void keep_screen_on(bool on) {
QNativeInterface::QAndroidApplication::runOnAndroidMainThread([on]() {
    const int FLAG_KEEP_SCREEN_ON = 128;
   QJniObject activity = QNativeInterface::QAndroidApplication::context();
   // Hide system ui elements or go full screen
   if (on) {
   activity.callObjectMethod("getWindow", "()Landroid/view/Window;")
           .callObjectMethod("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
   } else {
       activity.callObjectMethod("getWindow", "()Landroid/view/Window;")
               .callObjectMethod("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
   }
});
}
*/
#else
int getPermissions() {
    return 0;
}

#endif
