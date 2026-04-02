#ifndef AVAILABLEITEMSMODEL_H
#define AVAILABLEITEMSMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QDebug>
#include <QCollator>

#include "Programs.h"

class TreeItem : public QObject
{
    Q_OBJECT
public:
    explicit TreeItem(const QString name_=tr("Available Programs"), const QString key="", TreeItem *parentItem_ = nullptr, bool hasFrequencies=false, int level=0) {
        Name=name_;
        Key=key;
        m_parentItem=parentItem_;
        m_hasFrequencies=hasFrequencies;
        ItemLevel=level;

    }

    virtual ~TreeItem() {
        qDeleteAll(m_childItems);
    }

    bool matchFilter(int level, QString filter) {

        if (level>=ItemLevel) {
            if (filter=="")
                return true;
            if (Name.contains(filter, Qt::CaseInsensitive))
                return true;
            for (int i=0;i<m_childItems.size();i++) {
                if (m_childItems[i]->matchFilter(level, filter))
                    return true;
            }
        }
        return false;
    }

    bool isBiggerStringNumeric(const QString &val1, const QString &val2) {
        QString::const_iterator val1it=val1.constBegin();
        QString::const_iterator val2it=val2.constBegin();

        do {
            if ( (val1it==val1.constEnd()) && (val2it==val2.constEnd()))
                return false;

            if (*val1it!=*val2it) {
                if (val1it+1==val1.constEnd() && val2it+1!=val2.constEnd())
                    return false;
                return (*val1it>*val2it);
            }

            val1it++;
            val2it++;
        } while (1);

        return false;
    }

    void appendChild(TreeItem *child) {
        for (int i=0;i<m_childItems.size();i++) {
            //if (m_childItems[i]->Name>child->Name) {
            //if (m_childItems[i]->Name.compare(child->Name)>0) {

            if (isBiggerStringNumeric(m_childItems[i]->Name,child->Name)>0) {
                m_childItems.insert(i,child);
                return;
            }
        }

        m_childItems.append(child);
    }

    TreeItem *child(int row) {
        if (row < 0 || row >= m_childItems.size())
            return nullptr;
        return m_childItems.at(row);
    }

    int childCount() const {return m_childItems.size();}

    int columnCount() const {return 1;}

    QVariant name() const {return Name;}

    int row() const {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

        return 0;
    }

    QString id() const {
        return Key;
    }

    TreeItem *parentItem() {
        return m_parentItem;
    }

    bool hasFrequencies() {
        return m_hasFrequencies;
    }

    int level() {
        if (m_parentItem!=nullptr) {
            return std::max(ItemLevel, m_parentItem->level());
        }
        return ItemLevel;
    }
private:
    QVector<TreeItem*> m_childItems;
    QString Name;
    QString Key;
    int ItemLevel;
    TreeItem *m_parentItem;
    bool m_hasFrequencies;
};

class AvailableItemsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ProgramDataRoles {
        NameRole = Qt::UserRole + 1,
        IdRole,
        HasFrequenciesRole,
        VisibilityRole,
        HasChildrenRole,
        LevelColorRole,
        LevelColorVisibleRole
     };

    QHash<int, QByteArray> roleNames() const {
        QHash<int, QByteArray> roles;
        roles[NameRole] = QString("ProgramName").toLatin1();
        roles[IdRole] = QString("ProgramId").toLatin1();
        roles[HasFrequenciesRole] = QString("ProgramHasFrequencies").toLatin1();
        roles[VisibilityRole] = QString("ProgramIsVisible").toLatin1();
        roles[HasChildrenRole]= QString("hasChildren").toLatin1();
        roles[LevelColorRole]= QString("ProgramLevelIndicatorColor").toLatin1();
        roles[LevelColorVisibleRole]= QString("ProgramLevelIndicatorVisible").toLatin1();


        return roles;
    }

    Q_PROPERTY(QString filterString READ getFilterString WRITE setFilterString NOTIFY filterChanged)

    explicit AvailableItemsModel(QObject *parent = nullptr);
    virtual ~AvailableItemsModel() {
        beginResetModel();
       delete RootItem;
       RootItem=nullptr;
       endResetModel();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void addProgram(const QModelIndex &index) {
        qDebug()<<"addProgrammed called for Program "<< data(index,IdRole);
        emit addProgram(data(index,IdRole).toString());
    }

    Q_INVOKABLE QString getFilterString() {
        return filterString;
    }
    Q_INVOKABLE void setFilterString(QString filter);


    Q_INVOKABLE void setProgramsFilter(int level);

signals:
    void addProgram(QString Id);
    void filterChanged();
private:
    QList<QModelIndex> Items;
    TreeItem  *RootItem;

    QString filterString;
    int filterLevel;
};

#endif // AVAILABLEITEMSMODEL_H
