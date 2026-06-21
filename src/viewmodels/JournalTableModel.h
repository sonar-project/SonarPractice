#ifndef JOURNALTABLEMODEL_H
#define JOURNALTABLEMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QtQml/qqmlregistration.h>

class IPracticeJournalRepository;

/**
 * @brief Practice journal entries for one asset and calendar day.
 *
 * Only the success-streak column is editable in the table view.
 */
class JournalTableModel : public QAbstractTableModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Instances are provided via practiceTracker.journalModel.")

    Q_PROPERTY(qlonglong assetId READ assetId WRITE setAssetId NOTIFY assetIdChanged)
    Q_PROPERTY(
        QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectedDateChanged)

  public:
    enum class Roles : quint16 {
        EntryIdRole = Qt::UserRole + 1,
        DateRole,
        StartBarRole,
        EndBarRole,
        BpmRole,
        StreakRole,
        DurationMinutesRole
    };
    Q_ENUM(Roles)

    enum class DisplayColumn : quint8 {
        Date = 0,
        StartBar = 1,
        EndBar = 2,
        Bpm = 3,
        Streak = 4,
        DurationMinutes = 5
    };

    static constexpr int kColumnCount = 6;

    explicit JournalTableModel(IPracticeJournalRepository &journalRepo, QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index,
                                int role = Qt::DisplayRole) const override;

    [[nodiscard]] Q_INVOKABLE bool setData(const QModelIndex &index, const QVariant &value,
                                           int role) override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] qlonglong assetId() const;
    void setAssetId(qlonglong assetId);

    [[nodiscard]] QDate selectedDate() const;
    void setSelectedDate(const QDate &date);

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

  public slots:
    void reload();

  signals:
    void assetIdChanged();
    void selectedDateChanged();

  private:
    struct JournalRow {
        qlonglong id{0};
        QDate date;
        int startBar{0};
        int endBar{0};
        int bpm{0};
        int streak{0};
        int durationMinutes{0};
    };

    IPracticeJournalRepository &m_journalRepo;
    qlonglong m_assetId{0};
    QDate m_selectedDate;
    QList<JournalRow> m_rows;
};

#endif // JOURNALTABLEMODEL_H
