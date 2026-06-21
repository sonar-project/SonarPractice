/**
 * @file JournalTableModel.cpp
 * @brief Practice journal table for QML (one asset, one day).
 */

#include "JournalTableModel.h"

#include "JournalEntry.h"
#include "PracticeTrackerService.h"
#include "interfaces/IPracticeJournalRepository.h"

#include <QLocale>
#include <qnamespace.h>

JournalTableModel::JournalTableModel(IPracticeJournalRepository &journalRepo, QObject *parent)
    : QAbstractTableModel(parent), m_journalRepo(journalRepo),
      m_selectedDate(QDate::currentDate()) {}

int JournalTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

int JournalTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return kColumnCount;
}

QVariant JournalTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const JournalRow &row = m_rows.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (static_cast<DisplayColumn>(index.column())) {
        case DisplayColumn::Date:
            return QLocale().toString(row.date, QLocale::ShortFormat);
        case DisplayColumn::StartBar:
            return row.startBar;
        case DisplayColumn::EndBar:
            return row.endBar;
        case DisplayColumn::Bpm:
            return row.bpm;
        case DisplayColumn::Streak:
            return row.streak;
        case DisplayColumn::DurationMinutes:
            return row.durationMinutes;
        }
        return {};
    }

    switch (static_cast<Roles>(role)) {
    case Roles::EntryIdRole:
        return row.id;
    case Roles::DateRole:
        return row.date;
    case Roles::StartBarRole:
        return row.startBar;
    case Roles::EndBarRole:
        return row.endBar;
    case Roles::BpmRole:
        return row.bpm;
    case Roles::StreakRole:
        return row.streak;
    case Roles::DurationMinutesRole:
        return row.durationMinutes;
    default:
        return {};
    }
}

bool JournalTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return false;
    }

    if (role != Qt::EditRole && role != static_cast<int>(Roles::StreakRole)) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    if (index.column() != static_cast<int>(DisplayColumn::Streak)) {
        return false;
    }

    JournalRow row = m_rows.at(index.row());
    row.streak = value.toInt();

    JournalEntry updatedEntry;
    updatedEntry.id = row.id;
    updatedEntry.assetId = m_assetId;
    updatedEntry.practiceDate = QDateTime(row.date, QTime(0, 0));
    updatedEntry.startBar = row.startBar;
    updatedEntry.endBar = row.endBar;
    updatedEntry.practicedBpm = row.bpm;
    updatedEntry.successfulStreaks = row.streak;
    updatedEntry.durationSeconds = row.durationMinutes * 60;

    bool success = m_journalRepo.updateEntry(updatedEntry);
    if (success) {
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        reload();
        return success;
    }

    return QAbstractTableModel::setData(index, value, role);
}

QVariant JournalTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    static const QStringList headers = {
        tr("Day"), tr("Bar from"), tr("Bar to"), tr("Practice tempo"), tr("Success streak"),
        tr("Practice duration"),
    };

    static const QStringList headerTooltips = {
        tr("Date of the practice session"),
        tr("Start bar of the practiced section"),
        tr("End bar of the practiced section"),
        tr("Practiced tempo in BPM"),
        tr("Maximum consecutive error-free repetitions"),
        tr("Duration of the practice session in minutes"),
    };

    if (orientation != Qt::Horizontal || section < 0 ||
        section >= static_cast<int>(headers.size())) {
        return {};
    }

    if (role == Qt::ToolTipRole) {
        return headerTooltips.at(section);
    }

    if (role == Qt::DisplayRole) {
        return headers.at(section);
    }

    return {};
}

QHash<int, QByteArray> JournalTableModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
    roles[static_cast<int>(Roles::EntryIdRole)] = "entryId";
    roles[static_cast<int>(Roles::DateRole)] = "practiceDate";
    roles[static_cast<int>(Roles::StartBarRole)] = "startBar";
    roles[static_cast<int>(Roles::EndBarRole)] = "endBar";
    roles[static_cast<int>(Roles::BpmRole)] = "bpm";
    roles[static_cast<int>(Roles::StreakRole)] = "streak";
    roles[static_cast<int>(Roles::DurationMinutesRole)] = "durationMinutes";
    return roles;
}

qlonglong JournalTableModel::assetId() const { return m_assetId; }

void JournalTableModel::setAssetId(qlonglong assetId) {
    if (m_assetId == assetId) {
        return;
    }
    m_assetId = assetId;
    emit assetIdChanged();
    reload();
}

QDate JournalTableModel::selectedDate() const { return m_selectedDate; }

void JournalTableModel::setSelectedDate(const QDate &date) {
    if (m_selectedDate == date) {
        return;
    }
    m_selectedDate = date;
    emit selectedDateChanged();
    reload();
}

void JournalTableModel::reload() {
    beginResetModel();
    m_rows.clear();

    if (m_assetId > 0 && m_selectedDate.isValid()) {
        const QList<JournalEntry> entries =
            m_journalRepo.listForAssetAndDate(m_assetId, m_selectedDate);
        m_rows.reserve(entries.size());

        for (const JournalEntry &entry : entries) {
            JournalRow row;
            row.id = entry.id;
            row.date = entry.practiceDate.date();
            row.startBar = entry.startBar;
            row.endBar = entry.endBar;
            row.bpm = entry.practicedBpm;
            row.streak = entry.successfulStreaks;
            row.durationMinutes =
                PracticeTrackerService::durationMinutesFromSeconds(entry.durationSeconds);
            m_rows.append(row);
        }
    }

    endResetModel();
}

Qt::ItemFlags JournalTableModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    if (index.isValid() && index.column() == static_cast<int>(DisplayColumn::Streak)) {
        return defaultFlags | Qt::ItemIsEditable;
    }

    return defaultFlags;
}