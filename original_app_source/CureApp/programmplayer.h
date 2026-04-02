#ifndef PROGRAMMPLAYER_H
#define PROGRAMMPLAYER_H

#include <QObject>

class ProgrammPlayer : public QObject
{
    Q_OBJECT
public:
    explicit ProgrammPlayer(QObject *parent = nullptr);

signals:

private:


};

#endif // PROGRAMMPLAYER_H
