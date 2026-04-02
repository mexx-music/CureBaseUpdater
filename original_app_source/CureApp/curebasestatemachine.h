#ifndef CUREBASESTATEMACHINE_H
#define CUREBASESTATEMACHINE_H

#include <QString>
#include <QObject>
#include <QTimer>
#include <QFile>
#include <QElapsedTimer>

#include "Programs.h"
#include "myownprograms.h"
#include "myprogramsmodel.h"
#include "settingsmanager.h"
#include "bleuart.h"

#include "dataprovider.h"

class CureBaseStateMachine : public QObject
{
    Q_OBJECT
public:
    CureBaseStateMachine(BleUart *uart_, SettingsManager *settings_, MyProgramsModel *myPrograms_, MyOwnPrograms *myOwnPrograms_);

    Q_PROPERTY(bool ready READ isReady NOTIFY StateChange)
    Q_PROPERTY(bool supportsRemotePrograms READ supportsRemoteProgramsFlag NOTIFY supportsRemoteProgramsStateChange)

    Q_PROPERTY(bool cureProgramRunning READ isPlaybackRunning NOTIFY PlaybackChanged)
    Q_PROPERTY(bool cureProgramPaused READ isPlaybackPaused NOTIFY PlaybackChanged)
    Q_PROPERTY(bool cureProgramIdle READ isPlaybackIdle NOTIFY PlaybackChanged)

    Q_PROPERTY(bool currentCureProgramPlaybackCanStart READ isPlaybackCanStart NOTIFY PlaybackChanged)
    Q_PROPERTY(bool currentCureProgramPlaybackCanPause READ isPlaybackCanPause NOTIFY PlaybackChanged)
    Q_PROPERTY(bool currentCureProgramPlaybackCanStop READ isPlaybackCanStop NOTIFY PlaybackChanged)

    Q_PROPERTY(QString currentCureProgramPlaybackProgress READ currentCureProgramPlaybackProgress NOTIFY PlaybackChanged)
    Q_PROPERTY(qreal currentCureProgramPlaybackProgressPercent READ currentCureProgramPlaybackProgressPercent NOTIFY PlaybackChanged)

    Q_PROPERTY(QString currentCureProgramPlayback READ currentCureProgramPlayback NOTIFY PlaybackChanged)

    Q_PROPERTY(bool suggestUpdate READ isSuggestUpdate NOTIFY suggestUpdateChanged)

    Q_PROPERTY(double UpdateProgress READ updateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(bool updateInProgress READ isUpdateInProgress NOTIFY updateInProgressChanged)

    Q_PROPERTY(bool transferCureProgramInProgress READ isTransferCureProgramInProgress NOTIFY transferCureProgramInProgressChanged)
    Q_PROPERTY(qreal transferCureProgramProgress READ getTransferCureProgramProgress NOTIFY transferCureProgramInProgressChanged)


    enum CureBaseStates {
        CureBaseUnconnected=0,
        CureBaseResetState,
        CureBaseResponseSent,
        CurebaseTrusted,
        CureBaseUnlocked,
        CureBaseCuring,
        CureBaseEnterBootload,
        CureBaseBootloadSendBlock,
        CureBaseExitBootload,
        CureBaseCheckFirmwareVersion,
        CureBaseCheckHardwareVersion,
        CureBaseTransferCureProgram,
        CureBaseTransferCureProgramPacket,
        CureBaseRunCureProgramStart,
        CureBaseRunCureProgramPause,
        CureBaseRunCureProgramResume,
        CureBaseRunCureProgramStop,
        CureBaseGetStatus,
        CureBaseGetCureProgram,
        CureBaseSuggestUpdate
    };

    Q_INVOKABLE bool isPlaybackRunning() {
        return isRunning;
    }

    Q_INVOKABLE bool isReady() {
        switch (state) {
            case CureBaseUnconnected:
            case CureBaseResetState:
            case CureBaseResponseSent:
            case CurebaseTrusted:
            case CureBaseEnterBootload:
            case CureBaseBootloadSendBlock:
            case CureBaseExitBootload:
            case CureBaseCheckHardwareVersion:
                return false;

            case CureBaseCheckFirmwareVersion: //needed to make supportsRemoteProgramsFlag() work.
            case CureBaseSuggestUpdate:

            case CureBaseUnlocked:
            case CureBaseCuring:
            case CureBaseTransferCureProgram:
            case CureBaseTransferCureProgramPacket:
            case CureBaseRunCureProgramStart:
            case CureBaseRunCureProgramPause:
            case CureBaseRunCureProgramResume:
            case CureBaseRunCureProgramStop:
            case CureBaseGetCureProgram:
            return true;
        }

        return (state==CureBaseUnlocked);
    }

    Q_INVOKABLE bool isSuggestUpdate() {
        return (state==CureBaseSuggestUpdate);
    }

    Q_INVOKABLE bool isUpdateInProgress() {
        if (state==CureBaseEnterBootload)
            return true;
        else if (state==CureBaseBootloadSendBlock)
            return true;
        else if (state==CureBaseExitBootload)
            return true;

        return false;
    }

    Q_INVOKABLE double updateProgress() {
        double ret=0.0;
        if (bootloadFile.isOpen()) {
            if (bootloadFile.size()>0) {
               ret=((double)bootloadFile.pos())/((double)bootloadFile.size());
            }
        }
        return ret;
    }


    Q_INVOKABLE bool isPlaybackIdle() {
        return (!isRunning) && (!isTransferCureProgramInProgress());
    }

    Q_INVOKABLE bool isPlaybackPaused() {
        return isPaused;
    }

    Q_INVOKABLE bool isPlaybackCanStart() {
        return (isPaused && isRunning) || !isRunning;
    }

    Q_INVOKABLE bool isPlaybackCanPause() {
        return (!isPaused && isRunning);
    }

    Q_INVOKABLE bool isPlaybackCanStop() {
        return isRunning;
    }

    Q_INVOKABLE bool supportsRemoteProgramsFlag() {
        return ( isReady() && (VersionString>="0.1.0") );
    }

    Q_INVOKABLE QString currentCureProgramPlaybackProgress() {
        return QString("%1 / %2").arg(QmlHelpers::formatDurationSeconds(currentProgramElapsed())).arg(QmlHelpers::formatDurationSeconds(currentProgramDuration()));
    }

    Q_INVOKABLE qreal currentCureProgramPlaybackProgressPercent() {
        return (qreal)currentProgramElapsed()/(qreal)currentProgramDuration();
    }

    Q_INVOKABLE QString currentCureProgramPlayback() {
        return QString("<b>%1</b>").arg(getCurrentProgramName());
    }

    Q_INVOKABLE bool isTransferCureProgramInProgress() {
        qDebug()<<"isTransferCureProgramInProgress() returns "<<( (state==CureBaseTransferCureProgram) || (state==CureBaseTransferCureProgramPacket));
        if ( (state==CureBaseTransferCureProgram) || (state==CureBaseTransferCureProgramPacket))
            return true;
        return false;
    }

    Q_INVOKABLE qreal getTransferCureProgramProgress() {
        qDebug()<<"getTransferCureProgramProgress() returned "<<CureProgramTransferProgress;
        if ( (state!=CureBaseTransferCureProgram) && (state!=CureBaseTransferCureProgramPacket))
            return 0;
        return CureProgramTransferProgress;
    }

    void ProcessString(QString string);
    void ProcessByte(uint8_t byte);

    void EnterState(CureBaseStates newstate, CureBaseStates oldstate);
    void LeaveState(CureBaseStates newstate, CureBaseStates oldstate);
    void SwitchState(CureBaseStates newstate);

    DataProvider dataprovider;

public slots:
    void runProgram(int index);
    void runAllPrograms();

public:
    Q_INVOKABLE void ignoreUpdate();
    Q_INVOKABLE void acceptUpdate();

    Q_INVOKABLE void terminateProgram();
    Q_INVOKABLE void pauseProgram();
    Q_INVOKABLE void resumeProgram();
    Q_INVOKABLE qreal currentProgramElapsed();
    Q_INVOKABLE qreal currentProgramDuration();

    QList<qreal> getChartValueArray();

    Q_INVOKABLE QString getCurrentProgramName();
    Q_INVOKABLE QString getCurrentParentName();

    void setFrequency(qreal Frequency, qreal Intensity, bool enableH, bool enableE);
    void stopFrequency();

    CureBaseStates state;
    QString ReturnData;
    uint8_t Challenge[32];
    QString rxString;

    QString VersionString;
    QString HardwareString;

    QTimer programTimer;
    QElapsedTimer programElapsedTimer;
    QTimer GuiUpdateTimer;
    int currentProgramIndex;

    int StatusNotRunningCounter;

    CureProgramPlaylist playlistToRun;

    MyProgramsModel *myPrograms;

public slots:
    void Reset();
    void Connected();
    void dataRx(QByteArray);

    void runProgramCallback();

    void updateGuiTimerCallback();
signals:
    void dataTx(QByteArray data);

    void programFinished();

    void StateChange();
    void supportsRemoteProgramsStateChange();
    void PlaybackChanged();
    void suggestUpdateChanged();

    void updateProgressChanged();
    void updateInProgressChanged();

    void transferCureProgramInProgressChanged();

    void showProgrammTooLarge();

private:
    bool localCureProgramValid();
    uint32_t localCureProgramId();

    void updateGraphFromPcAndWaitTime(int PC, int WaitTime);

    SettingsManager *settings;
    MyOwnPrograms *myOwnPrograms;

    BleUart *uart;

    QFile bootloadFile;
    QElapsedTimer bootTimer;

    qint64 elapsedBeforePause;
    qint64 programRemainingBeforePause;

    QByteArray CureProgram;
    int CureProgramTransferIndex;

    bool isPaused;
    bool isRunning;

    int remoteProgramDuration;
    QUuid remoteProgramUUid;
    int remoteProgramElapsed;

    QList<QPair<int, double> > GraphData;

    double CureProgramTransferProgress;

};

#endif // CUREBASESTATEMACHINE_H
