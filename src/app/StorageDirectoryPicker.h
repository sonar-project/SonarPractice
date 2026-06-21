#ifndef STORAGEDIRECTORYPICKER_H
#define STORAGEDIRECTORYPICKER_H

#include <QString>

/**
 * \brief Provides functionality to browse and select storage directories.
 *
 * This class offers a static method to open a directory browser dialog, allowing users
 * to select a storage directory. The selected directory can be specified as the starting point,
 * and a title can be provided for the dialog.
 */
class StorageDirectoryPicker {
  public:
    /**
     * \brief Opens a directory browser dialog and returns the selected directory.
     *
     * This static method displays a directory browser dialog, allowing the user to select
     * a storage directory. The dialog starts at the specified start directory and has the given
     * title.
     *
     * \param startDir The starting directory for the dialog.
     * \param title The title of the dialog.
     * \return The selected directory as a QString.
     */
    [[nodiscard]] static QString browse(const QString &startDir, const QString &title);
};

#endif // STORAGEDIRECTORYPICKER_H
