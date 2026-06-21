#ifndef IREMINDERREPOSITORY_H
#define IREMINDERREPOSITORY_H

#include <optional>

#include <QList>

#include "Reminder.h"
#include "ReminderDayEntry.h"

class IReminderRepository {
  public:
    virtual ~IReminderRepository() = default;

    IReminderRepository(const IReminderRepository &) = delete;
    IReminderRepository &operator=(const IReminderRepository &) = delete;
    IReminderRepository(IReminderRepository &&) = delete;
    IReminderRepository &operator=(IReminderRepository &&) = delete;

    virtual std::optional<qlonglong> createReminder(const Reminder &reminder) = 0;
    virtual std::optional<Reminder> getReminder(qlonglong id) = 0;
    virtual bool updateReminder(const Reminder &reminder) = 0;
    virtual bool deleteReminder(qlonglong id) = 0;
    virtual QList<Reminder> listForSong(qlonglong songId) = 0;
    /**
     * @brief Lists reminders linked to one practice asset.
     *
     * A practice asset can aggregate multiple media files. All reminders
     * whose practice_asset_id matches are returned regardless of the
     * individual media file they were originally created for.
     */
    virtual QList<Reminder> listForPracticeAsset(qlonglong practiceAssetId) = 0;
    virtual QList<Reminder> listForDate(const QDate &date) = 0;
    /**
     * @brief Returns reminders due on one day with song title and BPM from a JOIN.
     * @param date The calendar day to load.
     */
    virtual QList<ReminderDayEntry> listForDateWithSong(const QDate &date) = 0;
    /**
     * @brief Returns all active reminders with song title and BPM from a JOIN.
     */
    virtual QList<ReminderDayEntry> listAllActiveWithSong() = 0;
    virtual QList<Reminder> listActiveInRange(const QDate &from, const QDate &to) = 0;

  protected:
    IReminderRepository() = default;
};

#endif // IREMINDERREPOSITORY_H
