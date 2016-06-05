/**********************************************************************
 * Copyright (C) 2014 MX Authors
 *
 * Authors: Adrian
 *          MEPIS Community <http://forum.mepiscommunity.org>
 *
 * This file is part of MX Tools.
 *
 * MX Tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MX Tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MX Tools.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "mxtools.h"
#include "ui_mxtools.h"
#include "flatbutton.h"

#include <QFile>
#include <QDebug>


mxtools::mxtools(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mxtools)
{
    ui->setupUi(this);
    // detect if tools are displayed in the menu (check for only one since all are set at the same time)
    if (system("grep -q \"NoDisplay=true\" /usr/share/applications/mx/mx-user.desktop") == 0) {
        ui->hideCheckBox->setChecked(true);
    }
    //QIcon::setThemeName(getCmdOut("xfconf-query -c xsettings -p /Net/IconThemeName"));
    //ui->buttonMenuEditor->setIcon(QIcon::fromTheme("edit-copy"));

    live_list = listDesktopFiles("MX-Live", "/usr/share/applications");
    maintenance_list = listDesktopFiles("MX-Maintenance", "/usr/share/applications");
    setup_list = listDesktopFiles("MX-Setup", "/usr/share/applications");
    software_list = listDesktopFiles("MX-Software", "/usr/share/applications");
    utilities_list = listDesktopFiles("MX-Utilities", "/usr/share/applications");

    QMultiMap<QString, QStringList> multimap;
    multimap.insertMulti("MX-Live", live_list);
    multimap.insertMulti("MX-Maintenance", maintenance_list);
    multimap.insertMulti("MX-Setup", setup_list);
    multimap.insertMulti("MX-Software", software_list);
    multimap.insertMulti("MX-Utilities", utilities_list);
    addButton(multimap);
    this->adjustSize();
}

mxtools::~mxtools()
{
    delete ui;
}

// Util function
QString mxtools::getCmdOut(QString cmd) {
    proc = new QProcess(this);
    proc->start("/bin/bash", QStringList() << "-c" << cmd);
    proc->setReadChannel(QProcess::StandardOutput);
    proc->setReadChannelMode(QProcess::MergedChannels);
    proc->waitForFinished(-1);
    return proc->readAllStandardOutput().trimmed();
}

// Get version of the program
QString mxtools::getVersion(QString name) {
    QString cmd = QString("dpkg -l %1 | awk 'NR==6 {print $3}'").arg(name);
    return getCmdOut(cmd);
}


// List .desktop files that contain a specific string
QStringList mxtools::listDesktopFiles(QString search_string, QString location)
{
    QStringList listDesktop;
    QString cmd = QString("grep -Elr %1 %2").arg(search_string).arg(location);
    QString out = getCmdOut(cmd);
    if (out != "") {
        listDesktop = out.split("\n");
    }
    return listDesktop;
}

void mxtools::addButton(QMultiMap<QString, QStringList> multimap)
{
    int col = 0;
    int row = 0;
    int max = 3; // no. max of col
    QString name;
    QString comment;
    QString exec;
    QString icon_name;
    QStringList list;

    foreach (QString category, multimap.keys()) {
        QLabel *label = new QLabel();
        QFont font;
        font.setBold(true);
        font.setUnderline(true);
        label->setFont(font);
        QString label_txt = category;
        label_txt.remove("MX-");
        label->setText(label_txt);
        col = 0;
        row += 1;
        ui->gridLayout_btn->addWidget(label, row, col);
        row += 1;
        list = multimap.value(category);
        foreach (QString item, list) {
            name = getCmdOut("grep ^Name= " + item + " | cut -f2 -d=");
            comment = getCmdOut("grep ^Comment= " + item + " | cut -f2 -d=");
            exec = getCmdOut("grep ^Exec= " + item + " | cut -f2 -d=");
            icon_name = getCmdOut("grep ^Icon= " + item + " | cut -f2 -d=");
            btn = new FlatButton(name);
            btn->setToolTip(comment);
            btn->setIcon(findIcon(icon_name));
            ui->gridLayout_btn->addWidget(btn, row, col);
            col += 1;
            if (col >= max) {
                col = 0;
                row += 1;
            }
            btn->setObjectName(exec); // add the command to be executed to the object name
            QObject::connect(btn, SIGNAL(clicked()), this, SLOT(btn_clicked()));

        }
        // add empty row if it's not the last key
        if (category != multimap.lastKey()) {
            col = 0;
            row += 1;
            label = new QLabel();
            ui->gridLayout_btn->addWidget(label, row, col);
        }
    }
}

// find icon by name specified in .desktop file
QIcon mxtools::findIcon(QString icon_name)
{
    // return icon if fully specified
    if (QFile(icon_name).exists()) {
        return QIcon(icon_name);
    } else {
        // return the icon from the theme if it exists
        if (QIcon::fromTheme(icon_name).name() != "") {
            return QIcon::fromTheme(icon_name);
        // return png, svg, xpm icons from /usr/share/pixmaps
        } else if (QFile("/usr/share/pixmaps/" + icon_name + ".png").exists()) {
            return QIcon("/usr/share/pixmaps/" + icon_name + ".png");
        } else if (QFile("/usr/share/pixmaps/" + icon_name + ".svg").exists()) {
            return QIcon("/usr/share/pixmaps/" + icon_name + ".svg");
        } else if (QFile("/usr/share/pixmaps/" + icon_name + ".xpm").exists()) {
            return QIcon("/usr/share/pixmaps/" + icon_name + ".xpm");
        } else if (QFile("/usr/share/pixmaps/" + icon_name).exists()) {
            return QIcon("/usr/share/pixmaps/" + icon_name);
        } else {
            qDebug() << "could not find icon: " << icon_name;
            return QIcon();
        }
    }
}

// run code when button is clicked
void mxtools::btn_clicked()
{
    this->hide();
    system(sender()->objectName().toUtf8());
    this->show();
}

// About button clicked
void mxtools::on_buttonAbout_clicked()
{
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Tools"), "<p align=\"center\"><b><h2>" +
                       tr("MX Tools") + "</h2></b></p><p align=\"center\">" + tr("Version: ") +
                       getVersion("mx-tools") + "</p><p align=\"center\"><h3>" +
                       tr("Configuration Tools for MX Linux") + "</h3></p><p align=\"center\"><a href=\"http://www.mepiscommunity.org/mx\">http://www.mepiscommunity.org/mx</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) antiX") + "<br /><br /></p>", 0, this);
    msgBox.addButton(tr("License"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    if (msgBox.exec() == QMessageBox::AcceptRole)
        system("mx-viewer file:///usr/share/doc/mx-tools/license.html 'MX Tools License'");
    this->show();
}


// Help button clicked
void mxtools::on_buttonHelp_clicked()
{
    this->hide();
    QString cmd = QString("mx-viewer file:///usr/local/share/doc/mxum.html#toc-Subsection-3.2");
    system(cmd.toUtf8());
    this->show();
}
