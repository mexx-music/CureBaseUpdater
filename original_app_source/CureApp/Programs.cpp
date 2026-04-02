#include "Programs.h"
#include <QFile>
#include "aes/aes.hpp"
#include "qlocale.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>

#include <QtCore/QDataStream>
#include <QtCore/QSettings>
#include <QLocale>
#include <QCoreApplication>
#include <QMap>

#include "myownprograms.h"

QHash<QString, CureProgram> CureProgram::availablePrograms;
QList<CureProgram> CureProgram::availableProgramsList;


void CureProgram::initAvailablePrograms() {
//QHash<QString, CureProgram> CureProgram::availablePrograms() {
    availablePrograms.clear();

    QFile file(":/Programs");
    file.open(QIODevice::ReadOnly);

    uint8_t key[32];
    key[0]=229;
    key[1]=107;
    key[2]=96;
    key[3]=204;
    key[4]=77;
    key[5]=182;
    key[6]=17;
    key[7]=175;
    key[8]=49;
    key[9]=141;
    key[10]=166;
    key[11]=201;
    key[12]=151;
    key[13]=204;
    key[14]=227;
    key[15]=133;
    key[16]=10;
    key[17]=58;
    key[18]=71;
    key[19]=76;
    key[20]=76;
    key[21]=32;
    key[22]=43;
    key[23]=219;
    key[24]=192;
    key[25]=198;
    key[26]=29;
    key[27]=71;
    key[28]=250;
    key[29]=110;
    key[30]=249;
    key[31]=255;

    const qint64 filesize=file.size();
    QByteArray buffer=file.readAll();

    for (int i=filesize-1;i>0;i--) {
        buffer[i]=buffer[i]^(buffer[i-1]^(key[i%16]));
    }
    buffer[0]=buffer[0]^key[0];

  //  QString JSON=QString::fromUtf8(qUncompress(buffer));
    QString JSON=QString::fromUtf8(buffer);

    QJsonParseError error;
    QJsonDocument doc=QJsonDocument::fromJson(buffer, &error);
    qDebug()<<error.errorString();
/*    qDebug()<<"isObject()="<<doc.isObject();
    qDebug()<<"isArray()="<<doc.isArray();
    qDebug()<<".array().count()="<<doc.array().count();
*/
    //QHash<QString, CureProgram> _availablePrograms;

    QJsonArray::const_iterator it;
    QJsonArray array=doc.array();

//    QString currentLanguage=QLocale::system().name().split("_").first().toUpper();
    QLocale currentLocale=QLocale::system();
    qDebug()<<"Current Locale: "<<currentLocale.name();

    QString currentLanguage=currentLocale.name().split("_").first().toUpper();
    //qDebug()<<"Current currentLanguage: "<<currentLanguage;
    //qDebug()<<"num Items: "<<array.size();


    for (it=array.constBegin();it!=array.constEnd();it++) {
        QJsonObject obj=(*it).toObject();

        QVariantMap map=obj.toVariantMap();

        QUuid id=map["ProgramUUID"].toUuid();
        QUuid parentId=map["Parent"].toUuid();
        int level=map["level"].toInt();

        auto translations=map["Program"].toMap();

        QString name=translations["EN"].toString();
        //qDebug()<<"translating "<<name<<"...";
        if (translations.contains(currentLanguage)) {
            //qDebug()<<"translated "<<name<<" to "<<translations[currentLanguage].toString();
            name=translations[currentLanguage].toString();
        } else {
            //qDebug()<<"could not translate "<<name<<" to "<<currentLanguage;
        }

        QList<QVariant> variantFrequencies=map["Frequencies"].toList();
        QList<QVariant>::ConstIterator freqIt;
        QList<qreal> frequencies;

        for (freqIt=variantFrequencies.constBegin();freqIt!=variantFrequencies.constEnd();freqIt++) {
            frequencies.append((*freqIt).toReal());
        }

        availablePrograms[id.toString()]=CureProgram(name,frequencies, id, parentId, 1.0, 15,true, true, -1, WaveForm_Sine, WaveForm_Sine, level-1);
    }

    //return _availablePrograms;
}

void CureProgram::initAvailableProgramsList() {

    availableProgramsList.clear();

    QFile file(":/Programs");
    file.open(QIODevice::ReadOnly);

    uint8_t key[32];
    key[0]=229;
    key[1]=107;
    key[2]=96;
    key[3]=204;
    key[4]=77;
    key[5]=182;
    key[6]=17;
    key[7]=175;
    key[8]=49;
    key[9]=141;
    key[10]=166;
    key[11]=201;
    key[12]=151;
    key[13]=204;
    key[14]=227;
    key[15]=133;
    key[16]=10;
    key[17]=58;
    key[18]=71;
    key[19]=76;
    key[20]=76;
    key[21]=32;
    key[22]=43;
    key[23]=219;
    key[24]=192;
    key[25]=198;
    key[26]=29;
    key[27]=71;
    key[28]=250;
    key[29]=110;
    key[30]=249;
    key[31]=255;

    const qint64 filesize=file.size();
    QByteArray buffer=file.readAll();

    for (int i=filesize-1;i>0;i--) {
        buffer[i]=buffer[i]^(buffer[i-1]^(key[i%16]));
    }
    buffer[0]=buffer[0]^key[0];

  //  QString JSON=QString::fromUtf8(qUncompress(buffer));
    QString JSON=QString::fromUtf8(buffer);

    QJsonParseError error;
    QJsonDocument doc=QJsonDocument::fromJson(buffer, &error);
    qDebug()<<error.errorString();
/*    qDebug()<<"isObject()="<<doc.isObject();
    qDebug()<<"isArray()="<<doc.isArray();
    qDebug()<<".array().count()="<<doc.array().count();
*/
    //QList<CureProgram> _availablePrograms;


//    QString currentLanguage=QLocale::system().name().split("_").first().toUpper();

    QLocale currentLocale=QLocale::system();
    //qDebug()<<"Current Locale: "<<currentLocale.name();

    QString currentLanguage=currentLocale.name().split("_").first().toUpper();
    //qDebug()<<"Current currentLanguage: "<<currentLanguage;


    QJsonArray::const_iterator it;
    QJsonArray array=doc.array();

    qDebug()<<"num Items: "<<array.size();

    for (it=array.constBegin();it!=array.constEnd();it++) {
        QJsonObject obj=(*it).toObject();

        QVariantMap map=obj.toVariantMap();

        QUuid id=map["ProgramUUID"].toUuid();
/*
        if (id.toString()=="{c1be2eaa-d923-5466-b3fc-17883d61fc9e}") {
            qDebug()<<"AHH";
        }
        */
        QUuid parentId=map["Parent"].toUuid();
        int level=map["level"].toInt();

        auto translations=map["Program"].toMap();

        QString name=translations["EN"].toString();
        /*if (translations.contains(currentLanguage))
            translations[currentLanguage].toString();
*/
        //qDebug()<<"translating "<<name<<"...";
        if (translations.contains(currentLanguage)) {
            //qDebug()<<"translated "<<name<<" to "<<translations[currentLanguage].toString();
            name=translations[currentLanguage].toString();
        } else {
            //qDebug()<<"could not translate "<<name<<" to "<<currentLanguage;
        }

/*
        QMap<QString, QVariant> nameMap=map["Program"].toMap();

        QStringList::const_iterator languageIt;
        QString language="EN";
        for (languageIt=uiLanguages.constBegin(); languageIt!=uiLanguages.constEnd();languageIt++) {
            if (nameMap.contains((*languageIt).toUpper())) {
                language=(*languageIt).toUpper();
                break;
            }
            QStringList parts=(*languageIt).split("-");
            for (int i=0;i<parts.size();i++) {
                if (nameMap.contains((parts[i]).toUpper())) {
                    language=(parts[i]).toUpper();
                    break;
                }
            }
        }

        QString name=map["Program"].toMap()["DE"].toString();
*/
        QList<QVariant> variantFrequencies=map["Frequencies"].toList();
        QList<QVariant>::ConstIterator freqIt;
        QList<qreal> frequencies;

        for (freqIt=variantFrequencies.constBegin();freqIt!=variantFrequencies.constEnd();freqIt++) {
            frequencies.append((*freqIt).toReal());
        }

        availableProgramsList.append(CureProgram(name,frequencies, id, parentId, 1.0, 15,true, true, -1, WaveForm_Sine, WaveForm_Sine, level-1));
    }

   // return _availablePrograms;
}

OwnCureProgram OwnCureProgram::newOwnProgram() {
    OwnCureProgram temp;
    temp.name=tr("My New Cure Program");
    temp.frequencies<<963;
    temp.id=QUuid::createUuid();
    return temp;
}

QByteArray CureProgramPlaylist::compileProgram(MyOwnPrograms *myOwnPrograms) {
    QByteArray program;

    QUuid currentProgram;

    QList<CureProgramFrequency>::ConstIterator it;
    for(it=frequencies.constBegin();it!=frequencies.constEnd();it++) {
        const CureProgramFrequency thisFreq=*it;

        if (currentProgram!=thisFreq.programId) {
            currentProgram=thisFreq.programId;
            CureProgramCmd_program_t prog;
            uint8_t TenPercent=thisFreq.intensity*10.0;
            prog.Eintensity=TenPercent * ((thisFreq.enableE)?1.0:0.0);
            prog.Hintensity=TenPercent * ((thisFreq.enableH)?1.0:0.0);

            prog.cmd=INSTRUCTIN_programm;

            QByteArray uuid=currentProgram.toRfc4122();
            for (int i=0;i<16;i++)
                prog.uuid[i]=uuid[i];

            program.append(((const char *)&prog), sizeof(CureProgramCmd_program_t));

            CureProgramCmd_waveForm_t waveForm;
            waveForm.cmd=INSTRUCTIN_waveForm;
            waveForm.EwaveForm=thisFreq.eWaveForm;
            waveForm.HwaveForm=thisFreq.hWaveForm;

            program.append(((const char *)&waveForm), sizeof(CureProgramCmd_waveForm_t));

            //see CureProgramCmd_customName_t
            program.append((char)INSTRUCTIN_customName);

            QByteArray name=tr("Unknown Program").toUtf8();

            if (CureProgram::availablePrograms.contains(thisFreq.programId.toString()))
                name=CureProgram::availablePrograms[thisFreq.programId.toString()].name.toUtf8();
            else if (myOwnPrograms->hasProgram(thisFreq.programId))
                name=myOwnPrograms->programName(thisFreq.programId).toUtf8();

            //                QByteArray name=CureProgram::availablePrograms[thisFreq.programId.toString()].name.toUtf8();
            program.append((char)name.length());
            program.append(name);
        }

        CureProgramCmd_frequency_t freq;
        freq.frequency=(*it).frequency;
        freq.dwelltime=(*it).duration;
        freq.cmd=INSTRUCTIN_frequency;

        program.append(((const char *)&freq), sizeof(CureProgramCmd_frequency_t));
    }

    CureProgramCmd_end_t end;
    end.cmd=INSTRUCTIN_end;

    program.append(((const char *)&end), sizeof(CureProgramCmd_end_t));

    uint32_t progLen=program.size();

    uint32_t crc=0;
    crc=mz_crc32(crc,(const unsigned char *)(const char *)program, progLen);

    CureProgramCmd_id_t progId;
    progId.cmd=INSTRUCTIN_programId;
    progId.programLen=progLen;
    progId.programId=crc;

    QByteArray HashedProgram;
    HashedProgram.append(((const char *)&progId), sizeof(CureProgramCmd_id_t));

    HashedProgram.append(program);
    return HashedProgram;
}
