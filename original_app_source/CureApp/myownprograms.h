#ifndef MYOWNPROGRAMS_H
#define MYOWNPROGRAMS_H

#include <QObject>
#include <QAbstractItemModel>
#include "qlocale.h"
#include "settingsmanager.h"
#include "qmlhelpers.h"
#include <QDebug>

class MyOwnPrograms  : public QAbstractListModel
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
        FrequencyRole,
        IdRole,
    };

    MyOwnPrograms(SettingsManager *settings_, QObject *parent = 0);

    QList<OwnCureProgram> myOwnPrograms;

    bool hasProgram(QUuid id) {
        for (int i=0;i<myOwnPrograms.count();i++) {
            if (myOwnPrograms[i].id==id)
                return true;
        }
        return false;
    }

    QString programName(QUuid id) {
        for (int i=0;i<myOwnPrograms.count();i++) {
            if (myOwnPrograms[i].id==id)
                return myOwnPrograms[i].name;
        }
        return tr("Unknown Program");
    }

    OwnCureProgram getProgramFromIndex(int index) {
        if (index<myOwnPrograms.size())
            return myOwnPrograms[index];
        return OwnCureProgram();
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
        roles[FrequencyRole] = QString("ProgramFrequency").toLatin1();
        roles[IdRole] = QString("ProgramId").toLatin1();

        return roles;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        QVariant value;
        if (index.isValid()) {
            if (role== Qt::DisplayRole) {
                if (index.row()<myOwnPrograms.size()) {
                    if (index.column()==1) {
                        value=QVariant::fromValue(myOwnPrograms[index.row()].name);
                    } else if (index.column()==2) {
                        value=QVariant::fromValue(myOwnPrograms[index.row()].duration);
                    } else if (index.column()==0) {
                        value=QVariant::fromValue(myOwnPrograms[index.row()].intensity);
                    }
                }
            } else if (role==NameRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].name);
            } else if (role==FrequencyRole) {
                QList<qreal> frequencies=myOwnPrograms[index.row()].frequencies;
                if (frequencies.count()>0) {
                    QLocale l;
                    value=QVariant::fromValue(l.toString(frequencies.first()));
                }
                else
                    value=963;
            } else if (role==IdRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].id);
            } else if (role==DurationRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].duration);
            } else if (role==IntensityRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].intensity);
            } else if (role==EFieldRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].enableE);
            } else if (role==HFieldRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].enableH);
            } else if (role==ProgramIndexRole) {
                value=QVariant::fromValue(myOwnPrograms[index.row()].programIndex);
            } else if (role==DurationTextRole) {
                value=QVariant::fromValue(QmlHelpers::formatDuration(myOwnPrograms[index.row()].duration));
            } else if (role==EWaveFormRole) {
                value=QVariant::fromValue((int)myOwnPrograms[index.row()].EwaveForm);
            } else if (role==HWaveFormRole) {
                value=QVariant::fromValue((int)myOwnPrograms[index.row()].HwaveForm);
            }
        }
        return value;
    }

    Q_INVOKABLE virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        bool resetModel=false;

        if (index.isValid()) {
            if (role== Qt::DisplayRole) {
                if (index.row()<myOwnPrograms.size()) {
                    if (index.column()==1) {
                    } else if (index.column()==2) {
                        myOwnPrograms[index.row()].duration=value.toInt();
                        emit dataChanged(index,index, QVector<int>()<<role);
                    } else if (index.column()==0) {
                        myOwnPrograms[index.row()].intensity=value.toReal();
                        emit dataChanged(index,index, QVector<int>()<<role);
                    }
                }
            } else if (role==NameRole) {
                myOwnPrograms[index.row()].name=value.toString();
                resetModel=true;
            } else if (role==FrequencyRole) {
                QLocale l;
                double frequency=l.toDouble(value.toString());
                myOwnPrograms[index.row()].frequencies=QList<qreal>()<<frequency;
            } else if (role==DurationRole) {
                myOwnPrograms[index.row()].duration=value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role<<DurationTextRole);
            } else if (role==IntensityRole) {
                myOwnPrograms[index.row()].intensity=value.toReal();
                emit dataChanged(index,index, QVector<int>()<<role<<Qt::DisplayRole);
            } else if (role==EFieldRole) {
                myOwnPrograms[index.row()].enableE=value.toBool();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==HFieldRole) {
                myOwnPrograms[index.row()].enableH=value.toBool();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==ProgramIndexRole) {
                myOwnPrograms[index.row()].programIndex=value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==EWaveFormRole) {
                myOwnPrograms[index.row()].EwaveForm=(OwnCureProgram::WaveForm_t)value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            } else if (role==HWaveFormRole) {
                myOwnPrograms[index.row()].HwaveForm=(OwnCureProgram::WaveForm_t)value.toInt();
                emit dataChanged(index,index, QVector<int>()<<role);
            }

            if (resetModel) {
                emit beginResetModel();
                qSort(myOwnPrograms.begin(), myOwnPrograms.end(),
                      [](OwnCureProgram a, OwnCureProgram b) -> bool { return a.name.compare(b.name)<0; });

            }

            settings->updateMyOwnPrograms(myOwnPrograms);

            if (resetModel) {
                emit endResetModel();
                emit UpdateProgramData();
            }

            return true;
        }

        return false;
    }

    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return myOwnPrograms.size();
    }
    Q_INVOKABLE virtual int columnCount(const QModelIndex &parent = QModelIndex()) const {
        return 3;
    }

    Q_INVOKABLE void removeProgram(int index) {
        if (index>=0) {
            if (index<myOwnPrograms.size()) {
                beginRemoveRows(QModelIndex(), index,index);
                myOwnPrograms.removeAt(index);

                settings->updateMyOwnPrograms(myOwnPrograms);
                endRemoveRows();

            }
        }

        emit UpdateProgramData();
    }

    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) {
        return true;
    }

    Q_INVOKABLE void addProgram(QUuid id) {

        qDebug()<<"addProgrammed called for OwnProgram "<<id;
        emit addProgram(id.toString());
    }

    Q_INVOKABLE void insertNewOwnProgram() {
        beginResetModel();
        myOwnPrograms.append(OwnCureProgram::newOwnProgram());

        qSort(myOwnPrograms.begin(), myOwnPrograms.end(),
              [](OwnCureProgram a, OwnCureProgram b) -> bool { return a.name.compare(b.name)<0; });

        settings->updateMyOwnPrograms(myOwnPrograms);
        endResetModel();

        emit UpdateProgramData();
    }
signals:
    void UpdateProgramData();
    void addProgram(QString Id);

private:
    SettingsManager *settings;
};

#endif // MYOWNPROGRAMS_H
