#include <QGuiApplication>
//#include <QApplication>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QLocale>
#include <QTranslator>

#include "Programs.h"
#include "myprogramsmodel.h"
#include "availableitemsmodel.h"
#include "availablecuredevices.h"
#include "bleuart.h"
#include "curebasestatemachine.h"
#include "settingsmanager.h"
#include <QIcon>
#include "Programs.h"

#include "qmlhelpers.h"
#include "dataprovider.h"

#include "myownprograms.h"

#include "utility.h"

QDataStream& operator<<(QDataStream& out, const CureProgram& v) {
    out<<(int)1; //Version;
    out<<v.id;
    out<<v.intensity;
    out<<v.duration;
    out<<v.enableE;
    out<<v.enableH;
    out<<v.programIndex;
    out<<v.EwaveForm;
    out<<v.HwaveForm;

    return out;
}

QDataStream& operator>>(QDataStream& in, CureProgram& v) {
    int version;
    in>>version;

    in>>v.id;
    in>>v.intensity;
    in>>v.duration;
    in>>v.enableE;
    in>>v.enableH;
    in>>v.programIndex;
    in>>v.EwaveForm;
    in>>v.HwaveForm;
    return in;
}

QDataStream& operator<<(QDataStream& out, const OwnCureProgram& v) {
    out<<(int)1; //Version;
    out<<v.id;
    out<<v.name;
    out<<v.intensity;
    out<<v.duration;
    out<<v.enableE;
    out<<v.enableH;
    out<<v.EwaveForm;
    out<<v.HwaveForm;
    out<<v.frequencies;
    return out;
}

QDataStream& operator>>(QDataStream& in, OwnCureProgram& v) {
    int version;
    in>>version;
    in>>v.id;
    in>>v.name;
    in>>v.intensity;
    in>>v.duration;
    in>>v.enableE;
    in>>v.enableH;
    in>>v.EwaveForm;
    in>>v.HwaveForm;
    in>>v.frequencies;

    return in;
}


int main(int argc, char *argv[]) {

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv); //does not work with QT Charts!!!
    //   QApplication app(argc, argv);

    qRegisterMetaType<CureProgram>();
    qRegisterMetaType<OwnCureProgram>();
    qRegisterMetaType<QList<CureProgram> >();
    qRegisterMetaType<QList<OwnCureProgram> >();
//todo: not needed in QT6
    qRegisterMetaTypeStreamOperators<CureProgram>();
    qRegisterMetaTypeStreamOperators<QList<CureProgram> >();
    qRegisterMetaTypeStreamOperators<QList<OwnCureProgram> >();

    QIcon::setThemeName("mytheme");

/*
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "CureApp_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }
*/
    QTranslator translator;
    // look up e.g. :/i18n/myapp_de.qm
    QLocale currentLocal=QLocale::system();

    if (translator.load(currentLocal, QLatin1String("CureApp"), QLatin1String("_"), QLatin1String(":/i18n")))
        QCoreApplication::installTranslator(&translator);

#ifdef Q_OS_ANDROID
    QTranslator AndroidTranslator;
    if (AndroidTranslator.load(currentLocal, QLatin1String("CureAppAndroidQuirks"), QLatin1String("_"), QLatin1String(":/i18n")))
        QCoreApplication::installTranslator(&AndroidTranslator);
#endif

    if (getPermissions()) {
        if (getPermissions())
            qDebug()<<"could not get all permissions!";
    }
    {

    QFile f("/storage/emulated/0/Documents/HealingAndBalanceCureAppConfig.txt");
    f.open((QIODevice::ReadWrite));
    qDebug()<<QString::fromLatin1(f.readAll());
    }
    CureProgram::initAvailablePrograms();
    CureProgram::initAvailableProgramsList();
    SettingsManager settings;
    BleUart uart(&settings);
    uart.initBLE();

    MyOwnPrograms myOwnPrograms(&settings);

    MyProgramsModel myPrograms(&settings, &myOwnPrograms);

    AvailableCureDevices availableCureDevicesMode(&settings, &uart);
    CureBaseStateMachine stateMachine(&uart, &settings, &myPrograms, &myOwnPrograms);
    AvailableItemsModel availablePrograms;


    int appRet;


    //be sure the QML engine gets destoryed well before our custom objects
    {

    QQmlApplicationEngine engine;
//        QQmlApplicationEngine engine("qrc:/main.qml");

    auto qmlHelpers = new QmlHelpers(&engine);
    engine.rootContext()->setContextProperty("qmlHelpers", qmlHelpers);
    engine.addImportPath("qrc:/mcharts");
    engine.rootContext()->setContextProperty("Settings", &settings);
    engine.rootContext()->setContextProperty("Uart", &uart);
    engine.rootContext()->setContextProperty("dataProvider", &stateMachine.dataprovider);
    engine.rootContext()->setContextProperty("Statemachine", &stateMachine);
    engine.rootContext()->setContextProperty("AvailablePrograms", &availablePrograms);

    engine.rootContext()->setContextProperty("MyOwnPrograms", &myOwnPrograms);




    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);


    engine.load(url);





    if (engine.rootObjects().isEmpty())
        return -1;

    QObject *listViewObject=engine.rootObjects().first()->findChild<QObject *>("MyProgramsDelegateModel");
    if (listViewObject)
        listViewObject->setProperty("model", QVariant::fromValue(&myPrograms));


    listViewObject=engine.rootObjects().first()->findChild<QObject *>("AvailableProgramsDelegateModel");
    if (listViewObject)
        listViewObject->setProperty("model", QVariant::fromValue(&availablePrograms));

    settings.connect(&settings, &SettingsManager::programFilterChanged, &availablePrograms, &AvailableItemsModel::setProgramsFilter);
    availablePrograms.setProgramsFilter(settings.getProgramFilter());

    availablePrograms.connect(&availablePrograms, SIGNAL(addProgram(QString)), &myPrograms, SLOT(addProgram(QString)));
    availablePrograms.connect(&myOwnPrograms, SIGNAL(addProgram(QString)), &myPrograms, SLOT(addProgram(QString)));


    QObject *cureDevicesView=engine.rootObjects().first()->findChild<QObject *>("AvailableCureDevices");
    if (cureDevicesView)
        cureDevicesView->setProperty("model", QVariant::fromValue(&availableCureDevicesMode));



    qputenv("QT_DEFAULT_CENTRAL_SERVICES", "1");
    availableCureDevicesMode.connect(&uart, SIGNAL(scanDone(QVariantMap, bool)), &availableCureDevicesMode, SLOT(scanDone(QVariantMap, bool)));

    stateMachine.connect(&stateMachine, SIGNAL(dataTx(QByteArray)), &uart, SLOT(writeData(QByteArray)));
    stateMachine.connect(&uart, SIGNAL(dataRx(QByteArray)), &stateMachine, SLOT(dataRx(QByteArray)));

    stateMachine.connect(&uart, SIGNAL(connecting()), &availableCureDevicesMode, SLOT(sendReset()));
    stateMachine.connect(&uart, SIGNAL(connected()), &availableCureDevicesMode, SLOT(sendReset()));
    stateMachine.connect(&uart, SIGNAL(disconnected()), &availableCureDevicesMode, SLOT(sendReset()));
    stateMachine.connect(&uart, SIGNAL(disconnected()), &stateMachine, SLOT(Reset()));

    stateMachine.connect(&uart, SIGNAL(connecting()), &myPrograms, SLOT(sendReset()));
    stateMachine.connect(&uart, SIGNAL(connected()), &myPrograms, SLOT(sendReset()));
    stateMachine.connect(&uart, SIGNAL(disconnected()), &myPrograms, SLOT(sendReset()));

    stateMachine.connect(&stateMachine, SIGNAL(StateChange()), &myPrograms, SLOT(sendReset()));

    stateMachine.connect(&uart, SIGNAL(scanStarted()), &availableCureDevicesMode, SLOT(clearDevices()));
    stateMachine.connect(&uart, SIGNAL(scanStarted()), &availableCureDevicesMode, SLOT(clearDevices()));


    stateMachine.connect(&uart, SIGNAL(connected()), &stateMachine, SLOT(Connected()));
    stateMachine.connect(&availableCureDevicesMode, SIGNAL(rescan()), &uart, SLOT(startScan()));

    uart.startScan();

    QIcon icon=QIcon(":/AppIcon");
    app.setWindowIcon(icon);


    appRet=app.exec();
    engine.quit();
    }
    return appRet;
}
