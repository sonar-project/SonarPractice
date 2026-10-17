#ifndef IMPORTORGANIZERTREEMODEL_H
#define IMPORTORGANIZERTREEMODEL_H

#include "ImportOrganizerSession.h"

#include <QAbstractListModel>
#include <QList>

class ImportOrganizerTreeModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Roles {
        NodeIdRole = Qt::UserRole + 1,
        ParentIdRole,
        DepthRole,
        NameRole,
        IsFolderRole,
        IsExistingManagedRole,
        CheckedRole,
        ExpandedRole,
        HasChildrenRole,
        StatusRole,
        StatusLabelRole,
        AbsolutePathRole,
    };

    explicit ImportOrganizerTreeModel(QObject *parent = nullptr);

    void setRows(QList<ImportOrganizerSession::FlatRow> rows);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  private:
    QList<ImportOrganizerSession::FlatRow> m_rows;
};

#endif // IMPORTORGANIZERTREEMODEL_H
