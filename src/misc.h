/*
 * Bittorrent Client using Qt4 and libtorrent.
 * Copyright (C) 2006  Christophe Dumez
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contact : chris@qbittorrent.org
 */

#ifndef MISC_H
#define MISC_H

#include <sstream>
#include <stdexcept>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QPair>
#include <QThread>

#include <libtorrent/torrent_info.hpp>
#include "qtorrenthandle.h"
using namespace libtorrent;

#define MAX_CHAR_TMP 128

/*  Miscellaneaous functions that can be useful */
class misc : public QObject{
 Q_OBJECT

  public:
    // Convert any type of variable to C++ String
    // convert=true will convert -1 to 0
    template <class T> static std::string toString(const T& x, bool convert=false) {
      std::ostringstream o;
      if(!(o<<x)) {
        throw std::runtime_error("::toString()");
      }
      if(o.str() == "-1" && convert)
        return "0";
      return o.str();
    }

    template <class T> static QString toQString(const T& x, bool convert=false) {
      std::ostringstream o;
      if(!(o<<x)) {
        throw std::runtime_error("::toString()");
      }
      if(o.str() == "-1" && convert)
        return QString::fromUtf8("0");
      return QString::fromUtf8(o.str().c_str());
    }

    template <class T> static QByteArray toQByteArray(const T& x, bool convert=false) {
      std::ostringstream o;
      if(!(o<<x)) {
        throw std::runtime_error("::toString()");
      }
      if(o.str() == "-1" && convert)
        return "0";
      return QByteArray(o.str().c_str());
    }

    // Convert C++ string to any type of variable
    template <class T> static T fromString(const std::string& s) {
      T x;
      std::istringstream i(s);
      if(!(i>>x)) {
        throw std::runtime_error("::fromString()");
      }
      return x;
    }

//     template <class T> static T fromQString::fromUtf8(const QString& s) {
//       T x;
//       std::istringstream i((const char*)s.toUtf8());
//       if(!(i>>x)) {
//         throw std::runtime_error("::fromString()");
//       }
//       return x;
//     }
// 
//     template <class T> static T fromQByteArray(const QByteArray& s) {
//       T x;
//       std::istringstream i((const char*)s);
//       if(!(i>>x)) {
//         throw std::runtime_error("::fromString()");
//       }
//       return x;
//     }

    // return best userfriendly storage unit (B, KiB, MiB, GiB, TiB)
    // use Binary prefix standards from IEC 60027-2
    // see http://en.wikipedia.org/wiki/Kilobyte
    // value must be given in bytes
    static QString friendlyUnit(float val) {
      char tmp[MAX_CHAR_TMP];
      if(val < 0) {
        return tr("Unknown", "Unknown (size)");
      }
      const QString units[4] = {tr("B", "bytes"), tr("KiB", "kibibytes (1024 bytes)"), tr("MiB", "mebibytes (1024 kibibytes)"), tr("GiB", "gibibytes (1024 mibibytes)")};
      for(unsigned int i=0; i<5; ++i) {
        if (val < 1024.) {
          snprintf(tmp, MAX_CHAR_TMP, "%.1f ", val);
          return QString::fromUtf8(tmp) + units[i];
        }
        val /= 1024.;
      }
      snprintf(tmp, MAX_CHAR_TMP, "%.1f ", val);
      return  QString::fromUtf8(tmp) + tr("TiB", "tebibytes (1024 gibibytes)");
    }

    // return qBittorrent config path
    static QString qBittorrentPath() {
      QString qBtPath = QDir::cleanPath(QDir::homePath()+QDir::separator()+QString::fromUtf8(".qbittorrent") + QDir::separator());
      // Create dir if it does not exist
      if(!QFile::exists(qBtPath)){
        QDir dir(qBtPath);
        dir.mkpath(qBtPath);
      }
      return qBtPath;
    }

// Not used anymore because it is not safe
//     static bool removePath(QString path) {
//       qDebug((QString::fromUtf8("file to delete:") + path).toUtf8());
//       if(!QFile::remove(path)) {
//         // Probably a folder
//         QDir current_dir(path);
//         if(current_dir.exists()) {
//           //Remove sub items
//           QStringList subItems = current_dir.entryList();
//           QString item;
//           foreach(item, subItems) {
//             if(item != QString::fromUtf8(".") && item != QString::fromUtf8("..")) {
//               qDebug("-> Removing "+(path+QDir::separator()+item).toUtf8());
//               removePath(path+QDir::separator()+item);
//             }
//           }
//           // Remove empty folder
//           if(current_dir.rmdir(path)) {
//             return true;
//           }else{
//             return false;
//           }
//         }else{
//           return false;
//         }
//       }
//       return true;
//     }

    // safe function to remove a torrent from hard-drive
    static bool removeTorrentSavePath(QString savePath, QStringList filesPath) {
      bool success = true;
      QDir saveDir(savePath);
      QString path;
      // Check how many file there are
      if(filesPath.size() == 1){
        // Only one file, not in a folder
        path = filesPath.first();
        if(QFile::exists(path)) {
          if(QFile::remove(path)){
            qDebug("Deleted only file in torrent at %s", path.toUtf8().data());
          } else {
            std::cerr << "Could not delete only file in torrent at " << path.toUtf8().data() << '\n';
            success = false;
          }
        }else{
          // File didn't exist, nothing to do
          qDebug("Only file %s did not exist, nothing to delete", path.toUtf8().data());
        }
        // Try to remove parent folder if empty (and not save_dir)
        QFileInfo fi(path);
        QDir parentFolder = fi.absoluteDir();
        while(parentFolder != saveDir) {
          qDebug("trying to remove parent folder: %s", parentFolder.absolutePath().toUtf8().data());
          if(!saveDir.rmdir(parentFolder.absolutePath())) break;
          parentFolder.cdUp();
        }
        return success;
      }
      // Torrent has several files in a subFolder
      foreach(path, filesPath) {
        if(QFile::exists(path)) {
          if(QFile::remove(path)){
            qDebug("Deleted file in torrent at %s", path.toUtf8().data());
          } else {
            std::cerr << "Could not delete file in torrent at " << path.toUtf8().data() << '\n';
            success = false;
          }
        } else {
          qDebug("File %s did not exist, nothing to delete", path.toUtf8().data());
        }
        // Try to remove parent folder if empty (and not save_dir)
        QFileInfo fi(path);
        QDir parentFolder = fi.absoluteDir();
        while(parentFolder != saveDir) {
          qDebug("trying to remove parent folder: %s", parentFolder.absolutePath().toUtf8().data());
          if(!saveDir.rmdir(parentFolder.absolutePath())) break;
          parentFolder.cdUp();
        }
      }
      return success;
    }

    static QString findFileInDir(QString dir_path, QString fileName) {
      QDir dir(dir_path);
      if(dir.exists(fileName)) {
        return dir.filePath(fileName);
      }
      QStringList subDirs = dir.entryList(QDir::Dirs);
      QString subdir_name;
      foreach(subdir_name, subDirs) {
        QString result = findFileInDir(dir.path()+QDir::separator()+subdir_name, fileName);
        if(!result.isNull()) {
          return result;
        }
      }
      return QString();
    }

    static void fixTrackersTiers(std::vector<announce_entry> trackers) {
      unsigned int nbTrackers = trackers.size();
      for(unsigned int i=0; i<nbTrackers; ++i) {
        trackers[i].tier = i;
      }
    }

    // Insertion sort, used instead of bubble sort because it is
    // approx. 5 times faster.
    template <class T> static void insertSort(QList<QPair<int, T> > &list, const QPair<int, T>& value, Qt::SortOrder sortOrder) {
      int i = 0;
      if(sortOrder == Qt::AscendingOrder) {
        while(i < list.size() and value.second > list.at(i).second) {
          ++i;
        }
      }else{
        while(i < list.size() and value.second < list.at(i).second) {
          ++i;
        }
      }
      list.insert(i, value);
    }

    template <class T> static void insertSort2(QList<QPair<int, T> > &list, const QPair<int, T>& value, Qt::SortOrder sortOrder) {
      int i = 0;
      if(sortOrder == Qt::AscendingOrder) {
        while(i < list.size() and value.first > list.at(i).first) {
          ++i;
        }
      }else{
        while(i < list.size() and value.first < list.at(i).first) {
          ++i;
        }
      }
      list.insert(i, value);
    }

    // Can't use template class for QString because >,< use unicode code for sorting
    // which is not what a human would expect when sorting strings.
    static void insertSortString(QList<QPair<int, QString> > &list, QPair<int, QString> value, Qt::SortOrder sortOrder) {
      int i = 0;
      if(sortOrder == Qt::AscendingOrder) {
        while(i < list.size() and QString::localeAwareCompare(value.second, list.at(i).second) > 0) {
          ++i;
        }
      }else{
        while(i < list.size() and QString::localeAwareCompare(value.second, list.at(i).second) < 0) {
          ++i;
        }
      }
      list.insert(i, value);
    }

    // Take a number of seconds and return an user-friendly
    // time duration like "1d 2h 10m".
    static QString userFriendlyDuration(const long int seconds) {
      if(seconds < 0) {
        return tr("Unknown");
      }
      if(seconds < 60) {
        return tr("< 1m", "< 1 minute");
      }
      int minutes = seconds / 60;
      if(minutes < 60) {
        return tr("%1m","e.g: 10minutes").arg(QString::QString::fromUtf8(misc::toString(minutes).c_str()));
      }
      int hours = minutes / 60;
      minutes = minutes - hours*60;
      if(hours < 24) {
        return tr("%1h%2m", "e.g: 3hours 5minutes").arg(QString::fromUtf8(misc::toString(hours).c_str())).arg(QString::fromUtf8(misc::toString(minutes).c_str()));
      }
      int days = hours / 24;
      hours = hours - days * 24;
      if(days < 100) {
        return tr("%1d%2h%3m", "e.g: 2days 10hours 2minutes").arg(QString::fromUtf8(misc::toString(days).c_str())).arg(QString::fromUtf8(misc::toString(hours).c_str())).arg(QString::fromUtf8(misc::toString(minutes).c_str()));
      }
      return tr("Unknown");
    }
};

//  Trick to get a portable sleep() function
class SleeperThread : public QThread{
  public:
    static void msleep(unsigned long msecs)
    {
      QThread::msleep(msecs);
    }
};

#endif
