#include "myownprograms.h"

MyOwnPrograms::MyOwnPrograms(SettingsManager *settings_, QObject *parent /*= 0*/) : QAbstractListModel(parent) {
    settings=settings_;

    myOwnPrograms=settings->getMyOwnProgramsFromSettings();

}
