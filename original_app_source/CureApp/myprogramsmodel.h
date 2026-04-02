#ifndef MYPROGRAMSMODEL_H
#define MYPROGRAMSMODEL_H
#include <QObject>
#include <QAbstractItemModel>
#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QDebug>

#include "Programs.h"
#include "settingsmanager.h"
#include "qmlhelpers.h"

class MyOwnPrograms;

class MyProgramsModel  : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ProgramDataRoles {
        NameRole = Qt::UserRole + 1,
        DurationRole,
        DurationTextRole,
        IntensityRole,
        EFieldRole,
        HFieldRole,
        ProgramIndexRole, //index is hardcoeded in function onReleased() for dragItemDelegate in MyProgramsForm.ui.qml!
        EWaveFormRole,
        HWaveFormRole,
        OwnProgramRole,
     };

    MyProgramsModel(SettingsManager *settings_, MyOwnPrograms *myOwnPrograms_, QObject *parent = 0);

    Q_INVOKABLE void sendReset() {beginResetModel(); endResetModel();}
    Q_INVOKABLE void moveItems(int from, int to) {
        if (from!=to) {
            if(from == to - 1) // Allow item moving to the bottom
                to = from++;
            beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
                qDebug()<<"moved from: "<<from<<"; to:"<<to;
                myPrograms.move(from, to);
                for (int i=0;i<myPrograms.count();i++) {
                    myPrograms[i].programIndex=i;
                }
                settings->updatePrograms(myPrograms);
            endMoveRows();

        }
        else{
        }

    }

    QList<CureProgram> myPrograms;

    CureProgram getProgramFromIndex(int index) {
        if (index<myPrograms.size())
            return myPrograms[index];
        return CureProgram();
    }

    QHash<int, QByteArray> roleNames() const {
        QHash<int, QByteArray> roles;
        roles[NameRole] = QString("ProgramName").toLatin1();
        roles[DurationRole] = QString("ProgramDuration").toLatin1();
        roles[IntensityRole] = QString("ProgramIntensity").toLatin1();
        roles[EFieldRole] = QString("ProgramUseEFields").toLatin1();
        roles[HFieldRole] = QString("ProgramUseHFields").toLatin1();
        roles[ProgramIndexRole] = QString("ProgramIndex").toLatin1();
        roles[DurationTextRole]= QString("ProgramDurationText").toLatin1();
        roles[EWaveFormRole] = QString("EWaveForm").toLatin1();
        roles[HWaveFormRole]= QString("HWaveForm").toLatin1();
        roles[OwnProgramRole]= QString("OwnProgram").toLatin1();

        return roles;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        QVariant value;
        if (index.isValid()) {
            if (role== Qt::DisplayRole) {
                if (index.row()<myPrograms.size()) {
                    if (index.column()==1) {
                        value=QVariant::fromValue(myPrograms[index.row()].name);
                    } else if (index.column()==2) {
                        value=QVariant::fromValue(myPrograms[index.row()].duration);
                    } else if (index.column()==0) {
                        value=QVariant::fromValue(myPrograms[index.row()].intensity);
                    }
                }
            } else if (role==NameRole) {
                value=QVariant::fromValue(myPrograms[index.row()].name);
            } else if (role==DurationRole) {
                value=QVariant::fromValue(myPrograms[index.row()].duration);
            } else if (role==OwnProgramRole) {
                value=QVariant::fromValue(myPrograms[index.row()].isOwnProgram);
            } else if (role==IntensityRole) {
                value=QVariant::fromValue(myPrograms[index.row()].intensity);
            } else if (role==EFieldRole) {
                value=QVariant::fromValue(myPrograms[index.row()].enableE);
            } else if (role==HFieldRole) {
                value=QVariant::fromValue(myPrograms[index.row()].enableH);
            } else if (role==ProgramIndexRole) {
                value=QVariant::fromValue(myPrograms[index.row()].programIndex);
            } else if (role==DurationTextRole) {
                value=QVariant::fromValue(QmlHelpers::formatDuration(myPrograms[index.row()].duration));
            } else if (role==EWaveFormRole) {
                value=QVariant::fromValue((int)myPrograms[index.row()].EwaveForm);
            } else if (role==HWaveFormRole) {
                value=QVariant::fromValue((int)myPrograms[index.row()].HwaveForm);
            }
        }
        return value;
    }

    Q_INVOKABLE virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        if (index.isValid()) {
            if (role== Qt::DisplayRole) {
                if (index.row()<myPrograms.size()) {
                    if (index.column()==1) {
                    } else if (index.column()==2) {
                        myPrograms[index.row()].duration=value.toInt();
                        emit dataChanged(index,index, QVector<int>()<<role);
                    } else if (index.column()==0) {
                        myPrograms[index.row()].intensity=value.toReal();
                        emit dataChanged(index,index, QVector<int>()<<role);
                    }
                }
            } else if (role==NameRole) {
            } else if (role==DurationRole) {
                myPrograms[index.row()].duration=value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role<<DurationTextRole);
            } else if (role==IntensityRole) {
                myPrograms[index.row()].intensity=value.toReal();
                emit dataChanged(index,index, QVector<int>()<<role<<Qt::DisplayRole);
            } else if (role==EFieldRole) {
                myPrograms[index.row()].enableE=value.toBool();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==HFieldRole) {
                myPrograms[index.row()].enableH=value.toBool();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==ProgramIndexRole) {
                myPrograms[index.row()].programIndex=value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==EWaveFormRole) {
                myPrograms[index.row()].EwaveForm=(CureProgram::WaveForm_t)value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==HWaveFormRole) {
                myPrograms[index.row()].HwaveForm=(CureProgram::WaveForm_t)value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            }

            settings->updatePrograms(myPrograms);
            return true;
        }

        return false;
    }

    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return myPrograms.size();
    }
    Q_INVOKABLE virtual int columnCount(const QModelIndex &parent = QModelIndex()) const {
        return 3;
    }

    void fillPlayList(CureProgramPlaylist &playlist);

    Q_INVOKABLE void removeProgram(int index) {
        if (index>=0) {
            if (index<myPrograms.size()) {
                beginRemoveRows(QModelIndex(), index,index);
                myPrograms.removeAt(index);

                for (int i=0;i<myPrograms.count();i++) {
                    myPrograms[i].programIndex=i;
                }

                settings->updatePrograms(myPrograms);
                endRemoveRows();

            }
        }
    }
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) {
        return true;
    }



public slots:
    void updateProgramData() {
        beginResetModel();
        myPrograms=settings->getProgramsFromSettings();

        for (int i=0;i<myPrograms.count();i++) {
            myPrograms[i].programIndex=i;
            settings->updatePrograms(myPrograms);
        }
        endResetModel();
    }

    void updateOwnProgramData() {
        updateProgramData();
    }

    void addProgram(QString ProgramKey) {
        beginInsertRows(QModelIndex(), myPrograms.size(),myPrograms.size());

        if (CureProgram::availablePrograms.contains(ProgramKey)) {
            myPrograms.append(CureProgram::availablePrograms[ProgramKey]);
        } else {
            QList<OwnCureProgram> ownPrograms=settings->getMyOwnProgramsFromSettings();
            QHash<QUuid, OwnCureProgram>ownProgramsHash;

            for (int i=0;i< ownPrograms.count();i++) {
                ownProgramsHash.insert(ownPrograms[i].id.toString(), ownPrograms[i]);
            }

            if (ownProgramsHash.contains(ProgramKey)) {
                myPrograms.append(ownProgramsHash[ProgramKey]);
            }
        }

        settings->updatePrograms(myPrograms);

        for (int i=0;i<myPrograms.count();i++) {
            myPrograms[i].programIndex=i;
            settings->updatePrograms(myPrograms);
        }
        endInsertRows();
    }
private:
    SettingsManager *settings;
    MyOwnPrograms *myOwnPrograms;
};

#endif // MYPROGRAMSMODEL_H
