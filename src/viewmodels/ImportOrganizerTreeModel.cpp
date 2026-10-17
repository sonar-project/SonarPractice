#include "ImportOrganizerTreeModel.h"

ImportOrganizerTreeModel::ImportOrganizerTreeModel(QObject *parent) : QAbstractListModel(parent) {}

void ImportOrganizerTreeModel::setRows(QList<ImportOrganizerSession::FlatRow> rows) {
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

int ImportOrganizerTreeModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QVariant ImportOrganizerTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const ImportOrganizerSession::FlatRow &row = m_rows.at(index.row());
    switch (role) {
    case NodeIdRole:
        return static_cast<qint64>(row.nodeId);
    case ParentIdRole:
        return static_cast<qint64>(row.parentId);
    case DepthRole:
        return row.depth;
    case NameRole:
        return row.name;
    case IsFolderRole:
        return row.isFolder;
    case IsExistingManagedRole:
        return row.isExistingManaged;
    case CheckedRole:
        return row.checked;
    case ExpandedRole:
        return row.expanded;
    case HasChildrenRole:
        return row.hasChildren;
    case StatusRole:
        return static_cast<int>(row.status);
    case StatusLabelRole:
        return row.statusLabel;
    case AbsolutePathRole:
        return row.absolutePath;
    default:
        return {};
    }
}

QHash<int, QByteArray> ImportOrganizerTreeModel::roleNames() const {
    return {
        {NodeIdRole, "nodeId"},
        {ParentIdRole, "parentId"},
        {DepthRole, "depth"},
        {NameRole, "name"},
        {IsFolderRole, "isFolder"},
        {IsExistingManagedRole, "isExistingManaged"},
        {CheckedRole, "checked"},
        {ExpandedRole, "expanded"},
        {HasChildrenRole, "hasChildren"},
        {StatusRole, "status"},
        {StatusLabelRole, "statusLabel"},
        {AbsolutePathRole, "absolutePath"},
    };
}
