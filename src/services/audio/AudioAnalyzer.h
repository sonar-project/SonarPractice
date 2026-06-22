#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QCoreApplication>
#include <QString>
#include <QVector>

class AudioAnalyzer {
    Q_DECLARE_TR_FUNCTIONS(AudioAnalyzer)

  public:
    struct Note {
        double startSec{};
        double endSec{};
        double frequencyHz{};
    };

    struct AnalysisResult {
        bool success{false};
        QString errorMessage;
        QVector<Note> notes;
    };

    [[nodiscard]] AnalysisResult analyze(const QString &filePath) const;
    bool analyzeAndSave(const QString &filePath, QString &errorMessage) const;

    [[nodiscard]] static QString jsonOutputPathFor(const QString &audioFilePath);
};

#endif // AUDIOANALYZER_H
