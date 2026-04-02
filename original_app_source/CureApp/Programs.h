#ifndef PROGRAMS_H
#define PROGRAMS_H
#include <QObject>
#include <QHash>
#include <QList>
#include <QHash>
#include <QUuid>
#include "miniz.h"
#include "quuid.h"

class MyOwnPrograms;


class CureProgram : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int duration MEMBER duration)
    Q_PROPERTY(double intensity MEMBER intensity)
    Q_PROPERTY(int programIndex MEMBER programIndex)
    Q_PROPERTY(WaveForm_t EwaveForm MEMBER EwaveForm)
    Q_PROPERTY(WaveForm_t HwaveForm MEMBER HwaveForm)

public:
    enum WaveForm_t {
        WaveForm_Sine=0x00,
        WaveForm_Triangle=0x01,
        WaveForm_Rectangular=0x02,
        WaveForm_SawTooth=0x03
    };
    Q_ENUM(WaveForm_t)

    CureProgram () : QObject() {
        duration=15;
        intensity=1.0;
        enableE=true;
        enableH=true;
        programIndex=-1;
        EwaveForm=WaveForm_Sine;
        HwaveForm=WaveForm_Sine;
        level=0;
        isOwnProgram=false;
    }

    CureProgram (const CureProgram& rhs) : QObject() {
        name=rhs.name;
        frequencies=rhs.frequencies;
        intensity=rhs.intensity;
        duration=rhs.duration;
        enableH=rhs.enableH;
        enableE=rhs.enableE;
        id=rhs.id;
        parentId=rhs.parentId;
        programIndex=rhs.programIndex;
        EwaveForm=rhs.EwaveForm;
        HwaveForm=rhs.HwaveForm;
        level=rhs.level;
        isOwnProgram=rhs.isOwnProgram;
    }

    CureProgram (QString Name , QList<qreal> Frequencies, QUuid Id, QUuid ParentId, double Intensity=1.0, int Duration=15, bool EnableE=true, bool EnableH=true, int ProgramIndex=-1, WaveForm_t eWaveForm=WaveForm_Sine, WaveForm_t hWaveForm=WaveForm_Sine, int level_=0) : QObject() {
        name=Name;
        frequencies=Frequencies;
        intensity=Intensity;
        duration=Duration;
        enableH=EnableE;
        enableE=EnableH;
        id=Id;
        parentId=ParentId;
        programIndex=ProgramIndex;
        EwaveForm=eWaveForm;
        HwaveForm=hWaveForm;
        level=level_;
        isOwnProgram=false;
    }

    virtual ~CureProgram () {

    }

    CureProgram &operator=(const CureProgram &rhs) {
        name=rhs.name;
        frequencies=rhs.frequencies;
        intensity=rhs.intensity;
        duration=rhs.duration;
        enableH=rhs.enableH;
        enableE=rhs.enableE;
        id=rhs.id;
        parentId=rhs.parentId;
        programIndex=rhs.programIndex;
        EwaveForm=rhs.EwaveForm;
        HwaveForm=rhs.HwaveForm;
        level=rhs.level;
        isOwnProgram=rhs.isOwnProgram;
        return *this;
    }

    static void initAvailableProgramsList();
    static void initAvailablePrograms();

    bool ownProgram() {return isOwnProgram;}

    static QHash<QString, CureProgram> availablePrograms;
    static QList<CureProgram> availableProgramsList;

    int programIndex;

    bool enableH;
    bool enableE;
    int duration;
    double intensity;
    QList<qreal> frequencies;

    QString name;
    QUuid id;
    QUuid parentId;
    WaveForm_t EwaveForm;
    WaveForm_t HwaveForm;
    int level;
    bool isOwnProgram;
};

Q_DECLARE_METATYPE(CureProgram);


class OwnCureProgram : public CureProgram {
    Q_OBJECT
public:
    OwnCureProgram() : CureProgram () {
        isOwnProgram=true;
    }

    static OwnCureProgram newOwnProgram();
};


Q_DECLARE_METATYPE(OwnCureProgram);

class CureProgramFrequency {
public:
    CureProgramFrequency(QUuid programId_, qreal freq, qreal intensity_, bool E, bool H, double secs, int ewaveform, int hwaveform) {
        programId=programId_;frequency=freq; intensity=intensity_; enableH=H, enableE=E; duration=secs;
        eWaveForm=ewaveform;
        hWaveForm=hwaveform;
    }

    QUuid programId;
    qreal frequency;
    qreal intensity;
    bool enableH;
    bool enableE;
    double duration;
    int eWaveForm;
    int hWaveForm;
};

//1 byte
#define INSTRUCTIN_end 0x00

//18 byte: 1byte instructin; 16byte Uuid; 1 nibble E intensity 0-10; 1 nibble H intensity 0-10
#define INSTRUCTIN_programm 0x01

//5 byte: 1byte instructin; 4byte float frequency; 2byte duration
#define INSTRUCTIN_frequency 0x02

//5 byte 1byte instruction; 4byte id
#define INSTRUCTIN_programId 0x03

//2 byte: 1byte instruction; 1nibble H waveform 1nibble E waveform
#define INSTRUCTIN_waveForm 0x04

//n bye: 1byte instruction 1byte length; n-bytes data (base64)
#define INSTRUCTIN_customName 0x05
#pragma pack(push,1)
typedef struct {
    uint8_t cmd;
} CureProgramCmd_end_t;

typedef struct {
    uint8_t cmd;
    uint8_t uuid[16];
    uint8_t Eintensity:4;
    uint8_t Hintensity:4;
} CureProgramCmd_program_t;

typedef struct {
    uint8_t cmd;
    float frequency;
    uint16_t dwelltime;
} CureProgramCmd_frequency_t;
typedef struct {
    uint8_t cmd;
    uint32_t programLen;
    uint32_t programId;
} CureProgramCmd_id_t;

typedef struct {
    uint8_t cmd;
    uint8_t EwaveForm:4;
    uint8_t HwaveForm:4;
} CureProgramCmd_waveForm_t;

typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[];
} CureProgramCmd_customName_t;
#pragma pack(pop)

class CureProgramPlaylist : QObject {
    Q_OBJECT
public:
    void operator =(const CureProgram &other) {
        clear();
        operator<<(other);
    }

    void clear() {
        frequencies.clear();
    }

    QByteArray compileProgram(MyOwnPrograms *myOwnPrograms);

    void operator <<(const CureProgram &other) {
        const int numFrequencies=other.frequencies.size();
        int secsPerFreq=(60.0*(double)other.duration)/(double)numFrequencies;
        int delta=(60.0*(double)other.duration)-numFrequencies*secsPerFreq;

        for (int i=0;i<numFrequencies;i++) {
            frequencies.append(CureProgramFrequency(other.id, other.frequencies[i], other.intensity, other.enableE, other.enableH, secsPerFreq+(i==0?delta:0), other.EwaveForm, other.HwaveForm));
        }
    }

    qreal duration() {
        double secs=0;
        const int numFrequencies=frequencies.count();
        for (int i=0;i<numFrequencies;i++) {
            secs+=frequencies[i].duration;
        }
        return secs;
    }

    QList<CureProgramFrequency> frequencies;
};

#endif // PROGRAMS_H
