#ifndef FNV1A_HPP
#define FNV1A_HPP

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QtGlobal>

class FNV1a {
  public:
    [[nodiscard]] static QString calculate(const QString &filePath) {
        constexpr uint64_t kFnvPrime = 1099511628211u;
        constexpr uint64_t kFnvOffsetBasis = 14695981039346656037u;
        constexpr int kNumSamples = 10;
        constexpr qint64 kSampleSize = 320;

        uint64_t hash = kFnvOffsetBasis;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) [[unlikely]] {
            return QString();
        }

        const qint64 fileSize = file.size();
        if (fileSize == 0) {
            return QStringLiteral("0000000000000000");
        }

        hash ^= static_cast<uint64_t>(fileSize);
        hash *= kFnvPrime;

        for (int i = 0; i < kNumSamples; ++i) {
            qint64 offset = 0;
            if (fileSize > kSampleSize) {
                offset = (fileSize - kSampleSize) * i / (kNumSamples - 1);
            }

            if (file.seek(offset)) {
                QByteArray chunk = file.read(kSampleSize);
                for (char byte : std::as_const(chunk)) {
                    hash ^= static_cast<uint8_t>(byte);
                    hash *= kFnvPrime;
                }
            }
        }

        file.close();

        return QStringLiteral("%1").arg(hash, 16, 16, QChar('0')).toUpper();
    }
};

#endif
