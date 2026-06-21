#ifndef LIBRARYSEARCHEXPRESSION_H
#define LIBRARYSEARCHEXPRESSION_H

#include <QString>

// Parses library queries like "Picking Licks && mp4 || pdf".
// Terms match case-insensitively; /pattern/ uses QRegularExpression.
class LibrarySearchExpression {
  public:
    [[nodiscard]] static bool matches(const QString &expression, const QString &haystack);

  private:
    [[nodiscard]] static bool matchesTerm(const QString &term, const QString &haystack);
    [[nodiscard]] static QStringList splitByOperator(const QString &text, const QString &op);
};

#endif // LIBRARYSEARCHEXPRESSION_H
