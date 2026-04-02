/*******************************************************************************
Copyright (C) 2020 Milo Solutions
Contact: https://www.milosolutions.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/


#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

#include <QObject>
#include <QColor>

#include <QJsonArray>

class CureBaseStateMachine;
class DataProvider : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<qreal> elapsedData READ elapsedData NOTIFY countChanged)
    Q_PROPERTY(QList<qreal> remainingData READ remainingData NOTIFY countChanged)
    Q_PROPERTY(QList<int> labels READ labels NOTIFY countChanged)

public:
    explicit DataProvider(CureBaseStateMachine *statemachine_, QObject *parent = nullptr);

    QList<qreal> elapsedData() const;
    QList<qreal> remainingData() const;

    QList<int> labels() const;

    Q_INVOKABLE void update(QList<double> &prevData, QList<double> &nextData);
    Q_INVOKABLE void init();
    Q_INVOKABLE void clear();
signals:
    void valuesChanged(const QList<qreal> &values) const;
    void labelsChanged(const QList<int> &labels) const;
    void countChanged() const;

private:
    QList<qreal> values;
    QList<qreal> elapsedValues;
    QList<qreal> remainingValues;
    QList<int> labelsValues;
    CureBaseStateMachine *statemachine;
};

#endif // DATAPROVIDER_H
