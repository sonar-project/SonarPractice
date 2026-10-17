#ifndef TST_REMINDERREPOSITORYTEST_H
#define TST_REMINDERREPOSITORYTEST_H

#include "DatabaseTestFixture.h"
#include "SqliteReminderConditionRepository.h"
#include "SqliteReminderRepository.h"

#include <QObject>

class TestReminderRepository : public QObject, protected DatabaseTestFixture {
    Q_OBJECT

  public:
    TestReminderRepository() : DatabaseTestFixture(QStringLiteral("ReminderRepositoryTest")) {}

  private slots:
    void init();
    void cleanup();

    void testCreateAndGetReminder();
    void testListForSong();
    void testListForPracticeAsset();
    void testListForSongExcludesPracticeAssetScopedReminders();
    void testListForDateDailyAndOnce();
    void testListForDateWithSongJoin();
    void testUpdateAndDeleteReminder();
    void testCreateConditionForReminder();

  private:
    [[nodiscard]] std::optional<qlonglong> createSong();
    [[nodiscard]] std::optional<qlonglong> createGuitarProFile(qlonglong songId, const QString &path);
    [[nodiscard]] std::optional<qlonglong> insertPracticeAssetWithGuitarPro(qlonglong songId,
                                                                            qlonglong guitarProId);

    SqliteReminderRepository m_reminderRepo{m_connector};
    SqliteReminderConditionRepository m_conditionRepo{m_connector};
};

#endif // TST_REMINDERREPOSITORYTEST_H
