#include "curebasestatemachine.h"
#include <QRandomGenerator>

#include <QDebug>
#include "kmackay-micro-ecc-24c60e2/uECC.h"

#include <QHash>
#include <QPointF>
#include <QVector>
#include "utility.h"
#include "Programs.h"

#include "miniz.h"


//uint8_t private_key_CureBase[32]={0xD3, 0xA6, 0xCC, 0xA0, 0x34, 0x37, 0x8E, 0x52, 0xDC, 0x59, 0x8C, 0x9E, 0x1A, 0xD5, 0xE9, 0xCC, 0x42, 0xBC, 0xE1, 0x07, 0x2E, 0xB3, 0xAB, 0x79, 0xBE, 0x66, 0xCE, 0x1C, 0xD3, 0x21, 0x91, 0x0A};
uint8_t public_key_CureBase[64]={0xD6, 0x0A, 0xDD, 0xC5, 0xDF, 0x7F, 0xDA, 0xE3, 0x79, 0x18, 0x34, 0x5C, 0x9B, 0x40, 0x6A, 0x86, 0x28, 0x61, 0x9B, 0xAF, 0x20, 0x14, 0x9B, 0x27, 0x8E, 0xA9, 0x04, 0xE3, 0x8E, 0x1E, 0x5A, 0x4D, 0x64, 0xD2, 0x40, 0x86, 0xAE, 0x93, 0x2E, 0x3F, 0xED, 0x16, 0x18, 0xBC, 0x34, 0x36, 0x53, 0x53, 0x52, 0xD3, 0x67, 0x79, 0x02, 0x4B, 0xBD, 0xED, 0xCF, 0x37, 0xA8, 0x3A, 0xEA, 0x0E, 0xAB, 0x17};

uint8_t private_key_CureApp[32]={0xE4, 0x07, 0x83, 0xF6, 0x81, 0xA5, 0xBB, 0x85, 0x2C, 0xAB, 0x1E, 0x10, 0x6B, 0x66, 0x41, 0xEF, 0xB4, 0x3C, 0x19, 0x23, 0xC1, 0xEB, 0xE2, 0x5C, 0xA3, 0x68, 0x65, 0xCD, 0xFA, 0xB0, 0x65, 0x48};
//uint8_t public_key_CureApp[64]={0x84, 0x99, 0x3D, 0x74, 0xCA, 0xCC, 0xB0, 0x90, 0x14, 0x06, 0x25, 0x5E, 0xCE, 0x42, 0x95, 0xE7, 0x27, 0xA3, 0xD6, 0x70, 0x91, 0x8D, 0xDE, 0xA4, 0x24, 0x6B, 0xBE, 0xF8, 0x9F, 0xBB, 0x7B, 0xD9, 0x4E, 0xE1, 0xA8, 0xED, 0xA4, 0xC0, 0x37, 0x93, 0x09, 0xD9, 0x68, 0x1B, 0xFE, 0x3D, 0x33, 0x2E, 0x48, 0x15, 0x78, 0x6B, 0x89, 0xF8, 0xF1, 0xF0, 0xF7, 0xF5, 0x7D, 0xAB, 0xED, 0x60, 0x81, 0x16};
uECC_Curve curve;


const char HEX[]="0123456789ABCDEF";

const int defaultStatusNotRunningReloadValue=3;

CureBaseStateMachine::CureBaseStateMachine(BleUart *uart_, SettingsManager *settings_, MyProgramsModel *myPrograms_, MyOwnPrograms *myOwnPrograms_) : myPrograms(myPrograms_), dataprovider(this) {
    settings=settings_;
    myOwnPrograms=myOwnPrograms_;
    uart=uart_;

    StatusNotRunningCounter=defaultStatusNotRunningReloadValue;

    curve=uECC_secp256k1();

    state=CureBaseResetState;
    programTimer.connect(&programTimer, SIGNAL(timeout()), this, SLOT(runProgramCallback()));
    programTimer.setSingleShot(true);
    programTimer.setTimerType(Qt::PreciseTimer);

    isRunning=false;
    isPaused=false;

    GuiUpdateTimer.setInterval(500);
    GuiUpdateTimer.setTimerType(Qt::PreciseTimer);
    GuiUpdateTimer.setSingleShot(false);

    connect(&GuiUpdateTimer, &QTimer::timeout, this, &CureBaseStateMachine::updateGuiTimerCallback);
}

void CureBaseStateMachine::updateGuiTimerCallback() {
//    emit PlaybackChanged();

    if (supportsRemoteProgramsFlag()) {

        emit dataTx("progStatus\n");
        if (state!=CureBaseRunCureProgramStart) {
            GuiUpdateTimer.stop();
        }
    } else {

        if ( (currentProgramIndex>=0) && (currentProgramIndex<playlistToRun.frequencies.count() ) ) {
            int prevFreq=-1;
            int thisFreq=-1;
            int nextFreq=-1;

            QList<double> pastPoints;
            QList<double> nextPoints;

            if (currentProgramIndex>0) {
                pastPoints.append(std::log10(playlistToRun.frequencies[currentProgramIndex-1].frequency));
                nextPoints.append(std::numeric_limits<double>::quiet_NaN());
            }
            CureProgramFrequency thisFrequency=playlistToRun.frequencies[currentProgramIndex];
            int total=thisFrequency.duration;
            int increment=total/100+1;
            int current=(programElapsedTimer.elapsed()+elapsedBeforePause)/1000;
/*
            if (thisFrequency.duration<100) {
                total=thisFrequency.duration*10;
                increment=total/10+1;
                current=programElapsedTimer.elapsed()+elapsedBeforePause)/100;
            }
          */
            int i=0;
            while (i<total) {
                if ( (i<=current) || (i==0) ) {
                    pastPoints.append(std::log10(thisFrequency.frequency));
                    nextPoints.append(std::numeric_limits<double>::quiet_NaN());
                } else {
                    nextPoints.append(std::log10(thisFrequency.frequency));
                    pastPoints.append(std::numeric_limits<double>::quiet_NaN());
                }

                i+=increment;
            }

            if ( currentProgramIndex+1<playlistToRun.frequencies.count() ) {
                nextPoints.append(std::log10(playlistToRun.frequencies[currentProgramIndex+1].frequency));
                pastPoints.append(std::numeric_limits<double>::quiet_NaN());
            }

            dataprovider.update(pastPoints, nextPoints);
            emit PlaybackChanged();

        }
    }
}

void CureBaseStateMachine::runAllPrograms() {
    isPaused=false;

    myPrograms->fillPlayList(playlistToRun);

    if (supportsRemoteProgramsFlag()) {
        dataprovider.clear();

        remoteProgramUUid=QUuid();

        remoteProgramDuration=0;
        remoteProgramElapsed=0;

        emit PlaybackChanged();

        CureProgram=playlistToRun.compileProgram(myOwnPrograms);
        qDebug()<<"CureProgram.size="<<CureProgram.size();
        if (CureProgram.size()>32768) {
            emit showProgrammTooLarge();
            return;
        }

        SwitchState(CureBaseTransferCureProgram);

    } else {

        currentProgramIndex=-1;
        programElapsedTimer.start();
        elapsedBeforePause=0;

        isRunning=true;
        isPaused=false;
        emit PlaybackChanged();

        #ifdef Q_OS_ANDROID
            keep_screen_on(true);
        #endif

        isRunning=true;
        isPaused=false;
        emit PlaybackChanged();
        GuiUpdateTimer.start();
        runProgramCallback();
    }
}

void CureBaseStateMachine::runProgram(int index) {
    isPaused=false;

    playlistToRun=myPrograms->getProgramFromIndex(index);

    if (supportsRemoteProgramsFlag()) {
        dataprovider.clear();

        remoteProgramUUid=QUuid();

        remoteProgramDuration=0;
        remoteProgramElapsed=0;

        emit PlaybackChanged();

        CureProgram=playlistToRun.compileProgram(myOwnPrograms);
        qDebug()<<"CureProgram.size="<<CureProgram.size();

        SwitchState(CureBaseTransferCureProgram);

    } else {

        currentProgramIndex=-1;
        programElapsedTimer.start();
        elapsedBeforePause=0;

        #ifdef Q_OS_ANDROID
        if (!supportsRemoteProgramsFlag()) {
            keep_screen_on(true);
        }
        #endif

        isRunning=true;
        isPaused=false;
        emit PlaybackChanged();
        GuiUpdateTimer.start();

        runProgramCallback();
    }
}

void CureBaseStateMachine::terminateProgram() {
    if (!supportsRemoteProgramsFlag()) {
        GuiUpdateTimer.stop();

        stopFrequency();
        isRunning=false;
        isPaused=false;

        emit programFinished();
        emit PlaybackChanged();

        if (supportsRemoteProgramsFlag()) {
            SwitchState(CureBaseUnlocked);
        }

    #ifdef Q_OS_ANDROID
            if (!supportsRemoteProgramsFlag()) {
                keep_screen_on(false);
            }
        #endif
    } else {
        emit dataTx("progStop\n"); //the next status report will switch the ui.

    }

    qDebug()<<"program terminated";
}

void CureBaseStateMachine::pauseProgram() {
    if (!supportsRemoteProgramsFlag()) {
        elapsedBeforePause+=programElapsedTimer.elapsed();

        isPaused=true;
        emit PlaybackChanged();

        stopFrequency();


        programRemainingBeforePause=programTimer.remainingTime();
        programTimer.stop();
        GuiUpdateTimer.stop();
    } else {
        emit dataTx("progPause\n"); //the next status report will switch the ui.
    }

}
void CureBaseStateMachine::resumeProgram() {
    if (!supportsRemoteProgramsFlag()) {
        programElapsedTimer.start();
        GuiUpdateTimer.start();

        isPaused=false;
        emit PlaybackChanged();

        const CureProgramFrequency currentFreq=playlistToRun.frequencies[currentProgramIndex];

        programTimer.setInterval(programRemainingBeforePause);
        programTimer.start();

        setFrequency(currentFreq.frequency, currentFreq.intensity, currentFreq.enableE, currentFreq.enableH);
    } else {
        emit dataTx("progResume\n"); //the next status report will switch the ui.
    }
}

void CureBaseStateMachine::runProgramCallback() {
    currentProgramIndex++;

    programElapsedTimer.restart();
    elapsedBeforePause=0;

    qDebug()<<"currentProgramIndex "<<currentProgramIndex;
    if (currentProgramIndex<playlistToRun.frequencies.count()) {
        const CureProgramFrequency currentFreq=playlistToRun.frequencies[currentProgramIndex];

        programTimer.setInterval(currentFreq.duration*1000);
        programTimer.start();

        setFrequency(currentFreq.frequency, currentFreq.intensity, currentFreq.enableE, currentFreq.enableH);
        qDebug()<<"setFrequency "<<currentFreq.frequency<<" ("<<currentFreq.intensity<<")";
    } else {
        stopFrequency();
        GuiUpdateTimer.stop();

        isRunning=false;
        isPaused=false;
        emit PlaybackChanged();

        emit programFinished();

        #ifdef Q_OS_ANDROID
            keep_screen_on(false);
        #endif

        qDebug()<<"program finished";
    }

}

qreal CureBaseStateMachine::currentProgramElapsed() {
    if (supportsRemoteProgramsFlag()) {
        return remoteProgramElapsed;
    }

    int previousFrequencyTimes=0;

    if ( (currentProgramIndex>0) && (currentProgramIndex<playlistToRun.frequencies.count()) ) {
        for (int i=0;i<currentProgramIndex;i++) {
            previousFrequencyTimes+=playlistToRun.frequencies[i].duration*1000;
        }
    }

    if (isPaused)
        return (qreal)(previousFrequencyTimes+elapsedBeforePause)/1000.0;

    return (qreal)(previousFrequencyTimes+elapsedBeforePause+programElapsedTimer.elapsed())/1000.0;
}

qreal CureBaseStateMachine::currentProgramDuration() {
    if (supportsRemoteProgramsFlag()) {
        return remoteProgramDuration;
    }

    return playlistToRun.duration();
}

QString CureBaseStateMachine::getCurrentProgramName() {
    if (supportsRemoteProgramsFlag()) {

        if (CureProgram::availablePrograms.contains(remoteProgramUUid.toString()))
            return CureProgram::availablePrograms[remoteProgramUUid.toString()].name;
        else if (myOwnPrograms->hasProgram(remoteProgramUUid))
            return myOwnPrograms->programName(remoteProgramUUid);
        //return QString("ERROR NO PROGRAM NAME");
        return tr("Unknown Program");
    }

    if (playlistToRun.frequencies.count()==0)
        return "";
    if (currentProgramIndex<0 || currentProgramIndex>=playlistToRun.frequencies.count())
        return "";

    const QString ProgId=playlistToRun.frequencies[currentProgramIndex].programId.toString();

    if (CureProgram::availablePrograms.contains(ProgId))
        return CureProgram::availablePrograms[ProgId].name;
    else if (myOwnPrograms->hasProgram(remoteProgramUUid))
        return myOwnPrograms->programName(remoteProgramUUid);

    return tr("Unknown Program");
}

QString CureBaseStateMachine::getCurrentParentName() {
    if (playlistToRun.frequencies.count()==0)
        return "";

    const QString ProgId=playlistToRun.frequencies[currentProgramIndex].programId.toString();

    if (CureProgram::availablePrograms.contains(ProgId)) {
        const QString ParentId=CureProgram::availablePrograms[ProgId].parentId.toString();
        if (CureProgram::availablePrograms.contains(ParentId))
            return CureProgram::availablePrograms[ParentId].name;
    } else if (myOwnPrograms->hasProgram(playlistToRun.frequencies[currentProgramIndex].programId)) {
        return QString("Own Cure Program");
    }

    return QString("ERROR NO PARENT NAME");
}

QList<qreal> CureBaseStateMachine::getChartValueArray() {
    QList<qreal> ret;
    const int numFreq=playlistToRun.frequencies.size();
    const int repeats=(1000.0/playlistToRun.duration())+1;

    for (int i=0;i<numFreq;i++) {
        const CureProgramFrequency currentFreq=playlistToRun.frequencies[i];
        const int freqRepeate=repeats*currentFreq.duration;
        for (int j=0;j<freqRepeate;j++)
            ret.append(currentFreq.frequency);
    }
    return ret;
}

void CureBaseStateMachine::setFrequency(qreal Frequency, qreal Intensity, bool enableH, bool enableE) {
    emit dataTx(QString("stop\r\n").toLatin1());

    if (enableE) {
        emit dataTx(QString("setEFreq=%1\r\n").arg(Frequency,0,'f', 2).toLatin1());
        emit dataTx(QString("setEAmp=%1\r\n").arg(Intensity,0,'f', 2).toLatin1());
    }

    if (enableH) {
        emit dataTx(QString("setHFreq=%1\r\n").arg(Frequency,0,'f', 2).toLatin1());
        emit dataTx(QString("setHAmp=%1\r\n").arg(Intensity,0,'f', 2).toLatin1());
    }
    emit dataTx(QString("start\r\n").toLatin1());
}

void CureBaseStateMachine::stopFrequency() {
    if (supportsRemoteProgramsFlag())
        emit dataTx(QString("progStop\r\n").toLatin1());
    else
        emit dataTx(QString("stop\r\n").toLatin1());
}

void CureBaseStateMachine::Reset() {
    SwitchState(CureBaseUnconnected);
}

void CureBaseStateMachine::Connected() {
    SwitchState(CureBaseResetState);
}


void CureBaseStateMachine::dataRx(QByteArray data) {
    for (int i=0;i<data.length();i++) {
       // qDebug()<<"RX: "<<(char)data[i];
        ProcessByte(data[i]);
    }
}

void CureBaseStateMachine::ProcessString(QString string) {
    qDebug()<<"State "<<state<<" Processing :"<<string;

    QString returnString;
    switch (state) {
        case CureBaseResetState: {
        if (ReturnData.isEmpty()) {
            if ((string.compare("ok",Qt::CaseInsensitive)==0) || (string.compare("error",Qt::CaseInsensitive)==0)) {
                //that should not happen!
                Reset();
            } else {
                ReturnData=string;
                qDebug()<<"Challenge:"<<ReturnData;
                qDebug()<<"Length:"<<ReturnData.length();
            }
        } else {
            if (string.compare("ok",Qt::CaseInsensitive)==0) {

                qDebug()<<"We got something to sign!";
                if (ReturnData.length()!=64) {
                    qDebug()<<"returnstring has incorrect length";
                    Reset();
                } else {

                }
                QByteArray hash=QByteArray::fromHex(ReturnData.toLatin1());
                uint8_t signatur[64];

                if (!uECC_sign(private_key_CureApp,(uint8_t *)((const char *)hash),32,signatur,curve)) {
                    qDebug()<<"Signing Failed!";
                    Reset();
                } else {
                    QString RequestString="response=";
                    for(int i=0;i<64;i++) {
                        uint8_t Tmp=signatur[i];
                        RequestString+=QString("%1").arg((uint32_t)Tmp, 2,16,QChar('0'));
                    }
                    RequestString+="\r\n";

                    ReturnData.clear();
                    emit dataTx(RequestString.toLatin1());
                    SwitchState(CureBaseResponseSent);
                }
            } else {
                //that should not happen!
                Reset();
            }
        }
        }
        break;
case CureBaseResponseSent:
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            SwitchState(CurebaseTrusted);
        } else {
            qDebug()<<"Resonse was not acked!";
            Reset();
        }
        break;
    case CurebaseTrusted:

        if (ReturnData.isEmpty()) {
            if ((string.compare("ok",Qt::CaseInsensitive)==0) || (string.compare("error",Qt::CaseInsensitive)==0)) {
                //that should not happen!
                qDebug()<<"Received OK but no signature was received!";
                Reset();
            } else {
                ReturnData=string;
                qDebug()<<"Signatur:"<<ReturnData;
                qDebug()<<"Length:"<<ReturnData.length();
            }
        } else {
            if (string.compare("ok",Qt::CaseInsensitive)==0) {

                qDebug()<<"We got something to verify!";
                if (ReturnData.length()!=128) {
                    qDebug()<<"returnstring has incorrect length";
                    Reset();
                } else {

                }
                QByteArray signatur=QByteArray::fromHex(ReturnData.toLatin1());
                if (!uECC_verify(public_key_CureBase,Challenge,32,(uint8_t *)((const char *)signatur),curve)) {
                    qDebug()<<"CureBase Verify Failed!";
                    Reset();
                } else {
                    qDebug()<<"CureBase Verify OK!";
                    SwitchState(CureBaseCheckHardwareVersion);
                }
            } else {
                //that should not happen!
                qDebug()<<"Expected OK but got :"<<string;
                Reset();
            }
        }
        break;


    case CureBaseCheckHardwareVersion: {
        if (string.compare("error", Qt::CaseInsensitive)!=0) {
            if (string.compare("ok",Qt::CaseInsensitive)==0) {
                if (HardwareString.isEmpty()) {
                    qDebug()<<"did not receive hardware version information!";
                    Reset();
                } else {
                    qDebug()<<"CureBase Hardware Version:"+HardwareString;

                    SwitchState(CureBaseCheckFirmwareVersion);
                }
            } else {
                HardwareString=string;
            }
        } else {
            Reset();
        }
    } break;
    case CureBaseCheckFirmwareVersion: {
        if (string.compare("error", Qt::CaseInsensitive)!=0) {
            if (string.compare("ok",Qt::CaseInsensitive)==0) {
                if (VersionString.isEmpty()) {
                    qDebug()<<"did not receive version information!";
                    Reset();
                } else {
                    qDebug()<<"CureBase Version:"+VersionString;

                    QStringList version=VersionString.split(".");
                    QList<int> bootloadVersion=QList<int>()<<0<<1<<2;
                    QList<int> bootloadexperimentalVersion=QList<int>()<<0<<1<<2;

                    bool versionOk=true;
                    bool experimentalVersionOk=true;

                    if (version.length()==3) {
                        for (int i=0;i<3;i++) {
                            if (version[i].toInt()>bootloadexperimentalVersion[i])
                                break;
                            if (version[i].toInt()<bootloadVersion[i]) {
                                versionOk=false;
                                break;
                            }
                        }
                    }

                    if (version.length()==3) {
                        for (int i=0;i<3;i++) {
                            if (version[i].toInt()>bootloadexperimentalVersion[i])
                                break;
                            if (version[i].toInt()<bootloadexperimentalVersion[i]) {
                                experimentalVersionOk=false;
                                break;
                            }
                        }
                    }

                    emit supportsRemoteProgramsStateChange();
//versionOk=false;

                    if (!experimentalVersionOk && settings->getFlashExperimentalFirmware()) {
                        uart->setConnectionParamsPowerLowLatency();
                        SwitchState(CureBaseSuggestUpdate);
                    } else if (!versionOk) {
                        uart->setConnectionParamsPowerLowLatency();
                        SwitchState(CureBaseSuggestUpdate);
                    } else {
                        uart->setConnectionParamsPowerSave();
                        if (supportsRemoteProgramsFlag())
                            SwitchState(CureBaseGetStatus);
                        else
                            SwitchState(CureBaseUnlocked);
                    }

                }
            } else {
                VersionString=string;
            }
        } else {
            Reset();
        }
    } break;

    case CureBaseSuggestUpdate:
        break;

    case CureBaseEnterBootload: {
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            uart->setConnectionParamsPowerLowLatency();

            if (bootloadFile.isOpen())
                bootloadFile.close();
            bootloadFile.setFileName(":/CureBaseFirmware");
            bootloadFile.open(QIODevice::ReadOnly);
            SwitchState(CureBaseBootloadSendBlock);
        } else {
            qDebug()<<"CureBaseEnterBootload failed!";
            if (bootloadFile.isOpen())
                bootloadFile.close();
            Reset();
        }
    } break;
      case  CureBaseBootloadSendBlock:
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            if (bootloadFile.atEnd())
                SwitchState(CureBaseExitBootload);
            else
                SwitchState(CureBaseBootloadSendBlock);
        } else {
            emit dataTx("\r\nabortOTA\r\n");
            qDebug()<<"CureBaseBootloadSendBlock failed!";
            if (bootloadFile.isOpen())
                bootloadFile.close();
            Reset();
        }
        break;

      case  CureBaseExitBootload:
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            qDebug()<<"Bootload finished...waiting for reconnect...";
            uart->setConnectionParamsPowerSave();
            Reset();
            uart->disconnectAndRescan();
            if (bootloadFile.isOpen())
                bootloadFile.close();
        } else {
            qDebug()<<"Exit Bootload failed!";
            if (bootloadFile.isOpen())
                bootloadFile.close();
            Reset();
        }
        break;

    case CureBaseTransferCureProgram:{
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            SwitchState(CureBaseTransferCureProgramPacket);
        } else {
            SwitchState(CureBaseTransferCureProgram);
        }
        }
        break;
    case CureBaseTransferCureProgramPacket:{
        if (string.compare("ok",Qt::CaseInsensitive)==0) {
            if (CureProgramTransferIndex<CureProgram.size())
                SwitchState(CureBaseTransferCureProgramPacket);
            else {
                SwitchState(CureBaseRunCureProgramStart);
                emit transferCureProgramInProgressChanged();
                emit PlaybackChanged();
            }
        } else {
            SwitchState(CureBaseTransferCureProgram);
        }
        }
        break;

    case CureBaseGetCureProgram:
        if (string.compare("error",Qt::CaseInsensitive)==0) {
            SwitchState(CureBaseTransferCureProgram);
        } else if (string.compare("ok",Qt::CaseInsensitive)==0) {
            if (!localCureProgramValid()) {
                emit dataTx("progStop\r\nstop\r\nprogClear\r\n");
                Reset();
            }
            SwitchState(CureBaseGetStatus);
        } else {
            CureProgram.append(QByteArray::fromHex(string.toLatin1()));
        }
        break;

    case CureBaseRunCureProgramStart:{
        if (string.compare("error",Qt::CaseInsensitive)==0) {
            SwitchState(CureBaseTransferCureProgram);
        } else if (string.compare("ok",Qt::CaseInsensitive)==0) {
            QStringList tmp=ReturnData.split((","));
            if (tmp.size()==7) {
                bool running=tmp[0].toInt();
                bool paused=tmp[1].toInt();
                int elapsed=tmp[2].toInt()/1000;
                int total=tmp[3].toInt()/1000;

                bool programValid=(tmp[4]!="-");

                uint32_t id=tmp[4].toUInt(nullptr,16);

                uint32_t PC=tmp[5].toUInt(nullptr,16);
                int32_t WaitTime=tmp[6].toUInt();

                remoteProgramDuration=total;
                remoteProgramElapsed=elapsed;

                isRunning=running;
                isPaused=paused;

                if (programValid && ( (!localCureProgramValid()) || (localCureProgramId()!=id) )) {
                    SwitchState(CureBaseGetCureProgram);
                } else {
                    updateGraphFromPcAndWaitTime(PC, WaitTime);
                }
                emit PlaybackChanged();

                if (!isRunning) {
                    if (StatusNotRunningCounter>0) {
                        StatusNotRunningCounter--;
                    } else
                        SwitchState(CureBaseUnlocked);
                } else
                    StatusNotRunningCounter=defaultStatusNotRunningReloadValue;

            } else {
                //prevent reset because of async statemachines...
                if (ReturnData!="")
                Reset();
            }
        } else {
            ReturnData=string;
        }
        break;
    }

    case CureBaseGetStatus: {
        if (string.compare("error",Qt::CaseInsensitive)==0) {
            SwitchState(CureBaseTransferCureProgram);
        } else if (string.compare("ok",Qt::CaseInsensitive)==0) {
            QStringList tmp=ReturnData.split((","));
            if (tmp.size()==7) {
                bool running=tmp[0].toInt();
                bool paused=tmp[1].toInt();
                int elapsed=tmp[2].toInt()/1000;
                int total=tmp[3].toInt()/1000;

                bool programValid=(tmp[4]!="-");

                uint32_t id=tmp[4].toUInt(nullptr,16);

                uint32_t PC=tmp[5].toUInt(nullptr,16);
                int32_t WaitTime=tmp[6].toUInt();

                remoteProgramDuration=total;
                remoteProgramElapsed=elapsed;

                isRunning=running;
                isPaused=paused;

                if (programValid && ( (!localCureProgramValid()) || (localCureProgramId()!=id) )) {
                    SwitchState(CureBaseGetCureProgram);
                } else {
                    if (running) {
                        StatusNotRunningCounter=defaultStatusNotRunningReloadValue;
                        if (!programValid) {
                            emit dataTx("progStop\r\nstop\r\nprogClear\r\n");
                            Reset();
                        } else
                            SwitchState(CureBaseRunCureProgramStart);
                    } else {
                        //there might be some race condition.. so wait for more than 1 status responses...
                        if (StatusNotRunningCounter>0) {
                            StatusNotRunningCounter--;
                            SwitchState(CureBaseGetStatus);
                        } else
                            SwitchState(CureBaseUnlocked);
                    }

                    emit PlaybackChanged();
                }
            } else {
                Reset();
            }
        } else {
            ReturnData=string;
        }
        break;
    }
    }
}

void CureBaseStateMachine::EnterState(CureBaseStates newstate, CureBaseStates oldstate) {
    //qDebug()<<"EnterState:"<<newstate;
    switch( newstate) {
    case CureBaseUnconnected:
            isRunning=false;
            isPaused=false;
            emit programFinished();
            emit PlaybackChanged();

            ReturnData.clear();
            rxString.clear();
        break;
    case CureBaseResetState: {
            isRunning=false;
            ReturnData.clear();
            rxString.clear();
            emit dataTx("\r\nchallenge\r\n");
        break;

    case CureBaseResponseSent:
            ReturnData.clear();
            rxString.clear();
            break;

    case CurebaseTrusted:
            ReturnData.clear();
            rxString.clear();

            QVector<quint32> vector;
            vector.resize(8);
            QRandomGenerator::global()->fillRange(vector.data(), vector.size());

            QString RequestString="sign=";
            for(int i=0;i<8;i++) {
                quint32 Tmp=vector[i];
                Challenge[i*4+0]=((uint8_t*)(&Tmp))[3];
                Challenge[i*4+1]=((uint8_t*)(&Tmp))[2];
                Challenge[i*4+2]=((uint8_t*)(&Tmp))[1];
                Challenge[i*4+3]=((uint8_t*)(&Tmp))[0];
                RequestString+=QString("%1").arg((uint32_t)Tmp, 8,16,QChar('0'));
            }

            RequestString+="\r\n";


            qDebug()<<"sending "<<RequestString.toLatin1();
            emit dataTx(RequestString.toLatin1());
            }
            break;

    case CureBaseSuggestUpdate:
        ReturnData.clear();
        rxString.clear();

        qDebug()<<"emit suggestUpdateChanged();";
        emit suggestUpdateChanged();

        break;

    case CureBaseCheckFirmwareVersion:
        ReturnData.clear();
        rxString.clear();
        VersionString.clear();
        qDebug()<<"sending getBuild";
        emit dataTx("getBuild\r\n");
        break;


    case CureBaseCheckHardwareVersion:
        ReturnData.clear();
        rxString.clear();
        HardwareString.clear();
        qDebug()<<"sending getHardware";
        emit dataTx("getHardware\r\n");

        break;
    case CureBaseEnterBootload:

        ReturnData.clear();
        rxString.clear();
        qDebug()<<"entering Bootload";
        emit dataTx("beginOTA\r\n");
        bootTimer.start();
        break;

    case CureBaseBootloadSendBlock: {
        ReturnData.clear();
        rxString.clear();
//        qDebug()<<"entering CureBaseBootloadSendBlock";

        //mz_ulong rawBufferSize=512;//185-10-2-1;
        mz_ulong rawBufferSize=512;
        uint8_t rawBuffer[rawBufferSize];
        rawBufferSize=bootloadFile.read((char *)rawBuffer, rawBufferSize);

        uint8_t buffer[rawBufferSize];

       // mz_ulong bufferSize=rawBufferSize;
       // int ret=mz_compress2(buffer, &bufferSize,rawBuffer, rawBufferSize, MZ_UBER_COMPRESSION);

 //       qDebug()<<"Progress: "<<(double)bootloadFile.pos()/(double)bootloadFile.size();
  //      qDebug()<<"Rate: "<<(1000.0*(double)bootloadFile.pos())/(double)bootTimer.elapsed();
       // qDebug()<<"Compression: "<<((double)rawBufferSize)/(double)bufferSize;


        //http://www.yenc.org/yenc-draft.1.3.txt
        /*A typical encoding process might look something like this:
         1. Fetch a character from the input stream.
         2. Increment the character's ASCII value by 42, modulo 256
         3. If the result is a critical character (as defined in the previous
            section), write the escape character to the output stream and increment
            character's ASCII value by 64, modulo 256.
         4. Output the character to the output stream.
         5. Repeat from start.
        */

        QByteArray packetString="packetOTA=";
        for (mz_ulong i=0;i<rawBufferSize;i++) {
            uint8_t byte=rawBuffer[i];
            byte+=42;

            if (byte==0 || byte=='\r' || byte=='\n' | byte=='=') {
                byte=rawBuffer[i];
                byte+=64;

                packetString+='=';
            }
            packetString+=byte;

            // packetString+=HEX[buffer[i]>>4];
           // packetString+=HEX[buffer[i]&0x0f];
        }

        packetString+="\r\n";
        emit dataTx(packetString/*.toLatin1()*/);

        }
        break;

    case CureBaseExitBootload:
        ReturnData.clear();
        rxString.clear();

        qDebug()<<"exit Bootload";
        emit dataTx("\r\n");
        emit dataTx("endOTA\r\n");

        break;

    case CureBaseGetCureProgram:
        ReturnData.clear();
        rxString.clear();
        qDebug()<<"CureBaseGetCureProgram";

        CureProgram.clear();
        emit dataTx("progRead\r\n");

        break;

    case CureBaseGetStatus:
        ReturnData.clear();
        rxString.clear();
        qDebug()<<"progStatus";
        emit dataTx("progStatus\r\n");

    case CureBaseUnlocked:
        ReturnData.clear();
        rxString.clear();
        settings->setLastDevice(uart->connectedTo());

        isRunning=false;
        isPaused=false;

        GuiUpdateTimer.stop();


        emit programFinished();
        emit PlaybackChanged();

        break;

    case CureBaseTransferCureProgram:{
        uart->setConnectionParamsPowerLowLatency();
        emit dataTx("progClear\n");
        CureProgramTransferIndex=0;
        CureProgramTransferProgress=0.0;
        emit transferCureProgramInProgressChanged();
        emit PlaybackChanged();
        }
        break;
    case CureBaseTransferCureProgramPacket:{
            QByteArray packetString="progAppend=";
            if (CureProgramTransferIndex<CureProgram.size()) {
                QByteArray tmp=CureProgram.mid(CureProgramTransferIndex, 256);
                CureProgramTransferIndex+=256;

                CureProgramTransferProgress=std::min((double)CureProgramTransferIndex/(double)CureProgram.size(), 1.0);
                emit transferCureProgramInProgressChanged();

                for (int j=0;j<tmp.size();j++) {
                    const uint8_t byte=tmp[j];
                    packetString.append(HEX[(byte>>4)&0x0f]);
                    packetString.append(HEX[byte&0x0f]);
                }
                packetString.append('\n');
                emit dataTx(packetString);
            } else {
                //we have finished already...

                SwitchState(CureBaseRunCureProgramStart);
                uart->setConnectionParamsPowerSave();
                emit transferCureProgramInProgressChanged();
                emit PlaybackChanged();
            }
        }
        break;

    case CureBaseRunCureProgramStart: {
        if (oldstate!=CureBaseGetStatus) { //when we enter from the GetStatus-State, there's already a cure program running...
            emit dataTx("progStart\n");
            StatusNotRunningCounter=defaultStatusNotRunningReloadValue;
        }
        GuiUpdateTimer.start();
        isRunning=true;
        isPaused=false;
        emit PlaybackChanged();
        }
        break;
    case CureBaseRunCureProgramPause:
        isPaused=true;
        emit PlaybackChanged();
        break;
    case CureBaseRunCureProgramResume:
        isPaused=false;
        emit PlaybackChanged();
        break;
    case CureBaseRunCureProgramStop:
        isRunning=false;
        isPaused=false;
        GuiUpdateTimer.stop();
        emit PlaybackChanged();
        break;
    }
}
void CureBaseStateMachine::LeaveState(CureBaseStates newstate, CureBaseStates oldstate) {
    if (oldstate==CureBaseUnlocked) {
        if (newstate!=CureBaseTransferCureProgram) {
            isRunning=false;
            isPaused=false;
            emit programFinished();
            emit PlaybackChanged();
            programTimer.stop();
            GuiUpdateTimer.stop();

            #ifdef Q_OS_ANDROID
                keep_screen_on(false);
            #endif
        }
    }
    if (oldstate==CureBaseRunCureProgramStart) {
        GuiUpdateTimer.stop();
    }

    if (oldstate==CureBaseSuggestUpdate) {
        emit suggestUpdateChanged();
    }


    if (oldstate==CureBaseEnterBootload) {
        emit updateInProgressChanged();
        emit updateProgressChanged();
    }
    if (oldstate==CureBaseBootloadSendBlock) {
        emit updateInProgressChanged();
        emit updateProgressChanged();
    }
    if (oldstate==CureBaseExitBootload) {
        emit updateInProgressChanged();
        emit updateProgressChanged();
    }
}

void CureBaseStateMachine::ProcessByte(uint8_t byte) {
    if (byte=='\r'|| byte=='\n'||byte==0) {
        if (rxString.length()!=0)
            ProcessString(rxString);
        rxString.clear();
    } else {
        if (QChar(byte).isPrint())
            rxString.append(QChar(byte));
    }
}

void CureBaseStateMachine::SwitchState(CureBaseStates newstate) {
    CureBaseStates oldState=state;
    state=newstate;

    LeaveState(newstate, oldState);
    EnterState(newstate, oldState);

    emit StateChange();
    emit supportsRemoteProgramsStateChange();
}

bool CureBaseStateMachine::localCureProgramValid() {
    if (CureProgram.size()<sizeof(CureProgramCmd_id_t))
        return false;

    if (CureProgram.at(0)!=INSTRUCTIN_programId)
        return false;

    const CureProgramCmd_id_t *prog=(const CureProgramCmd_id_t *)((const char *)CureProgram);

  //  qDebug()<<"CureProgramLength="<<prog->programLen<<" ("<<(CureProgram.size()-sizeof(CureProgramCmd_id_t))<<")";
//    qDebug()<<"sizeof(CureProgramCmd_id_t)="<<sizeof(CureProgramCmd_id_t);

    if (prog->programLen!=CureProgram.size()-sizeof(CureProgramCmd_id_t))
        return false;

    return true;
}

uint32_t CureBaseStateMachine::localCureProgramId() {
    if (!localCureProgramValid())
        return 0;

    const CureProgramCmd_id_t *prog=(const CureProgramCmd_id_t *)((const char *)CureProgram);
    return prog->programId;
}

void CureBaseStateMachine::updateGraphFromPcAndWaitTime(int PC, int WaitTime) {
    const char *prog=(const char *)CureProgram;
    const int progLen=CureProgram.size();

    int prevFreq=-1;
    int thisFreq=-1;
    int nextFreq=-1;
    int thisUuid=-1;

    int i=0;
    while ( (i<progLen) && (nextFreq==-1) ) {
        const uint8_t instr=prog[i];
        switch(instr) {
            case INSTRUCTIN_end:
                i+=1;
                break;
            case INSTRUCTIN_programm:
                if (i<PC) {
                    thisUuid=i;
                }

                i+=sizeof(CureProgramCmd_program_t);
                break;
            case INSTRUCTIN_frequency:
                if (i<PC) { //pc is the next instruction after wait-time is over... so thisInstruction is the instruction befor the PC
                    prevFreq=thisFreq;
                    thisFreq=i;
                } else {
                    nextFreq=i;
                    break;
                }
                i+=sizeof(CureProgramCmd_frequency_t);
                break;
            case INSTRUCTIN_programId:
                i+=sizeof(CureProgramCmd_id_t);
                break;
        case INSTRUCTIN_waveForm:
            i+=sizeof(CureProgramCmd_waveForm_t);
            break;
        case INSTRUCTIN_customName:
            i+=2+prog[i+1]; //1byte cmd, 1byte len, n-bytes actual data
            break;
        default:
            return;
        }
    }

    if (thisUuid>=0) {
        const CureProgramCmd_program_t *programInstruction=(const CureProgramCmd_program_t *)&prog[thisUuid];
        QByteArray uuidBytes;
        uuidBytes.append((const char *)(programInstruction->uuid),16);
        remoteProgramUUid=QUuid::fromRfc4122(uuidBytes);
    }

    CureProgramCmd_frequency_t *prevFrequency=nullptr;
    if (prevFreq>=0)
        prevFrequency=(CureProgramCmd_frequency_t *)&prog[prevFreq];

    CureProgramCmd_frequency_t *thisFrequency=nullptr;
    if (thisFreq>=0)
        thisFrequency=(CureProgramCmd_frequency_t *)&prog[thisFreq];


    CureProgramCmd_frequency_t *nextFrequency=nullptr;
    if (nextFreq>=0)
        nextFrequency=(CureProgramCmd_frequency_t *)&prog[nextFreq];

    if (thisFrequency==nullptr)
        return;

    QList<double> pastPoints;
    QList<double> nextPoints;

    if (prevFrequency!=nullptr) {
        pastPoints.append(std::log10(prevFrequency->frequency));
        nextPoints.append(std::numeric_limits<double>::quiet_NaN());
    }

    int total=thisFrequency->dwelltime;
    int increment=total/100+1;
    int current=total-WaitTime/1000;

    i=0;
    while (i<total) {
        if ( (i<=current) || (i==0) ) {
            pastPoints.append(std::log10(thisFrequency->frequency));
            nextPoints.append(std::numeric_limits<double>::quiet_NaN());
        } else {
            nextPoints.append(std::log10(thisFrequency->frequency));
            pastPoints.append(std::numeric_limits<double>::quiet_NaN());
        }

        i+=increment;
    }

    if (nextFrequency!=nullptr) {
        nextPoints.append(std::log10(nextFrequency->frequency));
        pastPoints.append(std::numeric_limits<double>::quiet_NaN());
    }

    dataprovider.update(pastPoints, nextPoints);
}

void CureBaseStateMachine::ignoreUpdate() {
    if (supportsRemoteProgramsFlag())
        SwitchState(CureBaseGetStatus);
    else
        SwitchState(CureBaseUnlocked);
}
void CureBaseStateMachine::acceptUpdate() {
    SwitchState(CureBaseEnterBootload);
}
