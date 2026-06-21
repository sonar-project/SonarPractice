#ifndef ICONFIGPROVIDER_H
#define ICONFIGPROVIDER_H

#include <QStringList>

class IConfigProvider {
  public:
    virtual ~IConfigProvider() = default;
    virtual QStringList allowedFileTypes() const = 0;
};

#endif // ICONFIGPROVIDER_H
