#include "availableitemsmodel.h"

#include <QColor>

AvailableItemsModel::AvailableItemsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    RootItem=new TreeItem();

    //QList<CureProgram> programs=CureProgram::availableProgramsList();
    QList<CureProgram>::const_iterator it;

    QHash<QString, TreeItem *> items;

    for (it=CureProgram::availableProgramsList.constBegin();it!=CureProgram::availableProgramsList.constEnd();it++) {
        CureProgram Program=*it;

        TreeItem *parentElement=RootItem;

        if (!Program.parentId.isNull())
            parentElement=items[Program.parentId.toString()];
       /* else
            qDebug()<<"AHHH";*/

        /*if (Program.name=="") {
            qDebug()<<"AHHH";
        }*/

        TreeItem *newItem=new TreeItem(Program.name,Program.id.toString(), parentElement, (Program.frequencies.count()>0), Program.level);
        parentElement->appendChild(newItem);
        items.insert(Program.id.toString(), newItem);
    }
}
QModelIndex AvailableItemsModel::index(int row, int column, const QModelIndex &parent) const
{
  //  if (!hasIndex(row, column, parent))
  //      return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = RootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if (parentItem) { //see rowcount... parentItem could be equal to a nullptr in the destructor
        if (row<parentItem->childCount()) {
            TreeItem *childItem = parentItem->child(row);
            if (childItem)
                return createIndex(row, column, childItem);
        }
    }
    return QModelIndex();
}

QModelIndex AvailableItemsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == RootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int AvailableItemsModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = RootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    //this would crash in the destructor otherwise... (RootItem==nullptr after the destructor was called)
    if (!parentItem)
        return 0;

    return parentItem->childCount();
}

int AvailableItemsModel::columnCount(const QModelIndex &parent) const
{
   if (!parent.isValid())
        return 1;
    return 1;
}

QVariant AvailableItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
           return QVariant();

    if (role==NameRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        return item->name();
    } else if (role==IdRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        return item->id();
    } else if (role == Qt::DisplayRole) {
       TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
       return item->name();
    } else if (role== HasFrequenciesRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        return item->hasFrequencies();
    } else if (role==VisibilityRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        return item->matchFilter(filterLevel, filterString);
    } else if (role==HasChildrenRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        return item->childCount()>0;

    } else if (role==LevelColorRole) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (filterLevel!=0) {
            switch (item->level()) {
            case 0:
                return QColor("limegreen");
            case 1:
                return QColor("gold");
            case 2:
                return QColor("tomato");
            }
        }

        return QColor("transparent");
    } else if (role==LevelColorVisibleRole) {
        return (filterLevel!=0);
    }

    return QVariant();
}

void AvailableItemsModel::setFilterString(QString filter) {
    beginResetModel();
    filterString=filter;
    endResetModel();
    emit filterChanged();
}

void AvailableItemsModel::setProgramsFilter(int level) {
    beginResetModel();
    filterLevel=level;
    endResetModel();
    emit filterChanged();
}
