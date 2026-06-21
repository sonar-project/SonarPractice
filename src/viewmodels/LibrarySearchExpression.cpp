#include "LibrarySearchExpression.h"

#include <QRegularExpression>
#include <QStringList>

bool LibrarySearchExpression::matchesTerm(const QString &term, const QString &haystack) {
    const QString trimmed = term.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    if (trimmed.size() >= 2 && trimmed.startsWith(QLatin1Char('/')) &&
        trimmed.endsWith(QLatin1Char('/'))) {
        const QRegularExpression pattern(trimmed.mid(1, static_cast<int>(trimmed.size()) - 2),
                                         QRegularExpression::CaseInsensitiveOption);
        if (pattern.isValid()) {
            return pattern.match(haystack).hasMatch();
        }
    }

    return haystack.contains(trimmed.toLower());
}

QStringList LibrarySearchExpression::splitByOperator(const QString &text, const QString &op) {
    QStringList parts;
    int start = 0;

    while (start <= static_cast<int>(text.size())) {
        const int index = static_cast<int>(text.indexOf(op, start, Qt::CaseInsensitive));
        if (index < 0) {
            parts.append(text.mid(start).trimmed());
            break;
        }

        parts.append(text.mid(start, index - start).trimmed());
        start = index + static_cast<int>(op.size());
    }

    return parts;
}

bool LibrarySearchExpression::matches(const QString &expression, const QString &haystack) {
    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    const QStringList orGroups = splitByOperator(trimmed, QStringLiteral("||"));
    for (const QString &orGroup : orGroups) {
        if (orGroup.isEmpty()) {
            continue;
        }

        bool andGroupMatches = true;
        const QStringList andTerms = splitByOperator(orGroup, QStringLiteral("&&"));
        for (const QString &term : andTerms) {
            if (!matchesTerm(term, haystack)) {
                andGroupMatches = false;
                break;
            }
        }

        if (andGroupMatches) {
            return true;
        }
    }

    return false;
}
