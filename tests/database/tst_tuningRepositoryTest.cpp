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

void TestTuningRepository::testFindTuningByName() {
    Tuning tuning;
    tuning.name = QStringLiteral("E Standard");

    const std::optional<qlonglong> tuningId = m_tuningRepo.createTuning(tuning);
    QVERIFY(tuningId.has_value());

    const std::optional<Tuning> loadedTuning =
        m_tuningRepo.findTuningByName(QStringLiteral("E Standard"));
    QVERIFY(loadedTuning.has_value());
    QCOMPARE(loadedTuning->id, *tuningId);
    QCOMPARE(loadedTuning->name, QStringLiteral("E Standard"));

    QVERIFY(!m_tuningRepo.findTuningByName(QStringLiteral("Missing Tuning")).has_value());
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

QTEST_MAIN(TestTuningRepository)
