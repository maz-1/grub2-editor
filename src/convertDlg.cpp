/*******************************************************************************
 * Copyright (C) 2008-2013 Konstantinos Smanis <konstantinos.smanis@gmail.com> *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the Free  *
 * Software Foundation, either version 3 of the License, or (at your option)   *
 * any later version.                                                          *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
 * more details.                                                               *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 *******************************************************************************/

//Own
#include "convertDlg.h"

//KDE
#include <KMessageBox>
//#include <KMimeType>
//Qt
#include <QFileDialog>
#include <QMimeType>
#include <QMimeDatabase>
//ImageMagick
#include <Magick++.h>

//Ui
#include "ui_convertDlg.h"

ConvertDialog::ConvertDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags)
{
    QWidget *widget = new QWidget(this);
    ui = new Ui::ConvertDialog;
    ui->setupUi(widget);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(widget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
    buttonBox->button(QDialogButtonBox::Cancel)->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(SlotOkButtonClicked()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    QMimeDatabase db;
    QString readFilter;
    QList<Magick::CoderInfo> coderList;
    coderInfoList(&coderList, Magick::CoderInfo::TrueMatch, Magick::CoderInfo::AnyMatch, Magick::CoderInfo::AnyMatch);
    QList<Magick::CoderInfo>::const_iterator it = coderList.constBegin();
    QList<Magick::CoderInfo>::const_iterator end = coderList.constEnd();
    for (; it != end; ++it) {
        readFilter.append(QString(" *.%1").arg(QString::fromStdString(it->name()).toLower()));
    }
    readFilter.remove(0, 1);
    readFilter.append('|').append(i18nc("@item:inlistbox", "ImageMagick supported image formats"));

    QString writeFilter = QString("*%1|%5 (%1)\n*%2|%6 (%2)\n*%3 *%4|%7 (%3 %4)").arg(".png", ".tga", ".jpg", ".jpeg", db.mimeTypeForName("image/png").comment(), db.mimeTypeForName("image/x-tga").comment(), db.mimeTypeForName("image/jpeg").comment());

    ui->kurlrequester_image->setMode(KFile::File | KFile::ExistingOnly | KFile::LocalOnly);
    ui->kurlrequester_image->fileDialog()->setAcceptMode(QFileDialog::AcceptOpen);
    ui->kurlrequester_image->fileDialog()->setNameFilter(readFilter);
    ui->kurlrequester_converted->setMode(KFile::File | KFile::LocalOnly);
    ui->kurlrequester_converted->fileDialog()->setAcceptMode(QFileDialog::AcceptSave);
    ui->kurlrequester_converted->fileDialog()->setNameFilter(writeFilter);
}
ConvertDialog::~ConvertDialog()
{
    delete ui;
}

void ConvertDialog::setResolution(int width, int height)
{
    if (width > 0 && height > 0) {
        ui->spinBox_width->setValue(width);
        ui->spinBox_height->setValue(height);
    }
}

void ConvertDialog::slotOkButtonClicked()
{
    QRegularExpression getdirectory("\\S*/");
    
        if (ui->kurlrequester_image->text().isEmpty() || ui->kurlrequester_converted->text().isEmpty()) {
            KMessageBox::information(this, i18nc("@info", "Please fill in both <interface>Image</interface> and <interface>Convert To</interface> fields."));
            return;
        } else if (ui->spinBox_width->value() == 0 || ui->spinBox_height->value() == 0) {
            KMessageBox::information(this, i18nc("@info", "Please fill in both <interface>Width</interface> and <interface>Height</interface> fields."));
            return;
        } else if (!QFileInfo(getdirectory.match(ui->kurlrequester_converted->url().toLocalFile()).captured(1)).isWritable()) {
            KMessageBox::information(this, i18nc("@info", "You do not have write permissions in this directory, please select another destination."));
            return;
        }
        Magick::Geometry resolution(ui->spinBox_width->value(), ui->spinBox_height->value());
        resolution.aspect(ui->checkBox_force->isChecked());
        Magick::Image image(std::string(ui->kurlrequester_image->url().toLocalFile().toUtf8()));
        image.zoom(resolution);
        image.depth(8);
        image.classType(Magick::DirectClass);
        image.write(std::string(ui->kurlrequester_converted->url().toLocalFile().toUtf8()));
        if (ui->checkBox_wallpaper->isChecked()) {
            emit splashImageCreated(ui->kurlrequester_converted->url().toLocalFile());
        }
    this->accept();
}
