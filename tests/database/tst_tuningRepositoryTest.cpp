#include "tst_tuningRepositoryTest.h"

#include "Tuning.h"

#include <QTest>

void TestTuningRepository::init() { setUp(); }

void TestTuningRepository::cleanup() { tearDown(); }

void TestTuningRepository::testCreateTuning() {
    Tuning firstTuning;
    firstTuning.name = QStringLiteral("E Standard");

    Tuning secondTuning;
    secondTuning.name = QStringLiteral("Drop D");

    const std::optional<qlonglong> firstId = m_tuningRepo.createTuning(firstTuning);
    const std::optional<qlonglong> secondId = m_tuningRepo.createTuning(secondTuning);

    QVERIFY(firstId.has_value());
    QVERIFY(secondId.has_value());
    QVERIFY(*firstId > 0);
    QVERIFY(*secondId > 0);
    QVERIFY(*firstId != *secondId);
}

void TestTuningRepository::testGetTuning() {
    Tuning tuning;
    tuning.name = QStringLiteral("E Standard");

    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    const std::optional<Tuning> loadedTuning = m_tuningRepo.getTuning(*tuningId);
    QVERIFY(loadedTuning.has_value());
    QCOMPARE(loadedTuning->name, QStringLiteral("E Standard"));
}

void TestTuningRepository::testUpdateTuning() {
    Tuning tuning;
    tuning.name = QStringLiteral("Old Tuning");

    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    Tuning updatedTuning;
    updatedTuning.id = *tuningId;
    updatedTuning.name = QStringLiteral("New Tuning");

    QVERIFY(m_tuningRepo.updateTuning(updatedTuning));

    const std::optional<Tuning> loadedTuning = m_tuningRepo.getTuning(*tuningId);
    QVERIFY(loadedTuning.has_value());
    QCOMPARE(loadedTuning->name, QStringLiteral("New Tuning"));
}

void TestTuningRepository::testDeleteTuning() {
    Tuning tuning;
    tuning.name = QStringLiteral("To Delete");

    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    QVERIFY(m_tuningRepo.deleteTuning(*tuningId));

    const std::optional<Tuning> loadedTuning = m_tuningRepo.getTuning(*tuningId);
    QVERIFY(!loadedTuning.has_value());
}

void TestTuningRepository::testGetTuningNotFound() {
    const std::optional<Tuning> tuning = m_tuningRepo.getTuning(99999);
    QVERIFY(!tuning.has_value());
}

void TestTuningRepository::testCreateTuningDuplicateName() {
    Tuning firstTuning;
    firstTuning.name = QStringLiteral("Duplicate Tuning");

    Tuning secondTuning;
    secondTuning.name = QStringLiteral("Duplicate Tuning");

    const std::optional<qlonglong> firstId = m_tuningRepo.createTuning(firstTuning);
    const std::optional<qlonglong> secondId = m_tuningRepo.createTuning(secondTuning);

    QVERIFY(firstId.has_value());
    QVERIFY(!secondId.has_value());
}

void TestTuningRepository::testListAllTunings() {
    Tuning dropD;
    dropD.name = QStringLiteral("Drop D");
    Tuning standard;
    standard.name = QStringLiteral("E Standard");

    QVERIFY(m_tuningRepo.createTuning(dropD).has_value());
    QVERIFY(m_tuningRepo.createTuning(standard).has_value());

    const QList<Tuning> tunings = m_tuningRepo.listAllTunings();
    QCOMPARE(tunings.size(), 2);
    QCOMPARE(tunings.first().name, QStringLiteral("Drop D"));
    QCOMPARE(tunings.last().name, QStringLiteral("E Standard"));
}

QTEST_MAIN(TestTuningRepository)
