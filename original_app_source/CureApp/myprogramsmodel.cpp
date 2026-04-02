#include "myprogramsmodel.h"
#include <algorithm>
#include "myownprograms.h"

MyProgramsModel::MyProgramsModel(SettingsManager *settings_, MyOwnPrograms *myOwnPrograms_, QObject *parent /*= 0*/) : QAbstractListModel(parent) {
    settings=settings_;
    myOwnPrograms=myOwnPrograms_;


    connect(settings, SIGNAL(currentClientChanged()), this, SLOT(updateProgramData()));
    connect(myOwnPrograms, SIGNAL(UpdateProgramData()), this, SLOT(updateOwnProgramData()));

    myPrograms=settings->getProgramsFromSettings();

    std::sort(myPrograms.begin(), myPrograms.end(), [](const CureProgram &v1, const CureProgram &v2){ return (v1.programIndex) < (v2.programIndex);});
}


void MyProgramsModel::fillPlayList(CureProgramPlaylist &playlist) {
    //myPrograms is not necessarily sorted... however, the programIndex reflects the indendet order...
    QList<CureProgram> programs=myPrograms;
    std::sort(programs.begin(), programs.end(), [](const CureProgram &v1, const CureProgram &v2){ return (v1.programIndex) < (v2.programIndex);});

    playlist.clear();
    const int numProgs=programs.size();
    for (int i=0;i<numProgs;i++)
        playlist<<programs[i];
}
