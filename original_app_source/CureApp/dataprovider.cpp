
#include "dataprovider.h"
#include "curebasestatemachine.h"
#include <QJsonObject>

DataProvider::DataProvider(CureBaseStateMachine *statemachine_, QObject *parent) : QObject(parent) {
    statemachine=statemachine_;

}

void DataProvider::init() {
    values=statemachine->getChartValueArray();

    labelsValues.clear();
    const int numValues=values.count();
    for (int i=0;i<numValues;i++)
        labelsValues.append(0);

    QList<double> dummy;
    update(dummy, dummy);
    //emit countChanged();
}

void DataProvider::update(QList<double> &prevData, QList<double> &nextData) {
    elapsedValues.clear();
    remainingValues.clear();
    labelsValues.clear();

    QList<double>::ConstIterator it;
    int i=0;
    for (it=prevData.constBegin();it!=prevData.constEnd();it++) {
        elapsedValues.append(*it);
        labelsValues.append(i);
        i++;
    }

    for (it=nextData.constBegin();it!=nextData.constEnd();it++) {
        remainingValues.append(*it);
    }

    emit countChanged();
}

QList<qreal> DataProvider::elapsedData() const {
    return elapsedValues;
}

QList<qreal> DataProvider::remainingData() const {
    return remainingValues;
}

QList<int> DataProvider::labels() const {
    return labelsValues;
}

void DataProvider::clear() {
    elapsedValues.clear();
    remainingValues.clear();
    labelsValues.clear();
    emit countChanged();
}
