#include "StorageDirectoryPicker.h"

#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QWidget>

QString StorageDirectoryPicker::browse(const QString &startDir, const QString &title) {
    QWidget *parent = QApplication::activeWindow();
    QFileDialog dialog(parent, title, startDir);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    const QStringList selected = dialog.selectedFiles();
    return selected.isEmpty() ? QString() : selected.first();
}
