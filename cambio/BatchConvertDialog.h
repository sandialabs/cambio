/**
 Cambio: a simple program to convert or manipulate gamma spectrum data files.
 Copyright (C) 2015 William Johnson
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef BatchConvertDialog_H
#define BatchConvertDialog_H

#include <QList>
#include <QString>
#include <QDialog>

class QCheckBox;
class QLineEdit;
class QGroupBox;
class QTreeView;
class QPushButton;
class QGridLayout;
class QButtonGroup;
class QFileSystemModel;


class BatchConvertDialog : public QDialog
{
  Q_OBJECT
  
public:
  BatchConvertDialog( QWidget * parent = nullptr, Qt::WindowFlags f = 0 );
  ~BatchConvertDialog();
  
  void setDirectory( const QString &path );
  void setFiles( const QList<QString> &files );
  
public slots:
  void convert();
  void browseForDirectory();
  
protected:
  QGridLayout *m_layout;
  
  QTreeView *m_filesystemView;
  QFileSystemModel *m_filesystemModel;
  
  QGroupBox *m_formatBox;
  QButtonGroup *m_format;
  
  QLineEdit *m_saveToPath;
  
  QCheckBox *m_recursive;

  QPushButton *m_convert;
};//class BatchConvertDialog

#endif //BatchConvertDialog_H
