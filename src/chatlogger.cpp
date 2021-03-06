/*
 *  The ManaPlus Client
 *  Copyright (C) 2009-2010  The Mana Developers
 *  Copyright (C) 2009-2010  Andrei Karas
 *  Copyright (C) 2011-2012  The ManaPlus Developers
 *
 *  This file is part of The ManaPlus Client.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "chatlogger.h"

#include <iostream>
#include <sstream>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef WIN32
#include <windows.h>
#elif defined __APPLE__
#include <Carbon/Carbon.h>
#endif

#include "logger.h"
#include "configuration.h"
#include "utils/mkdir.h"
#include "utils/stringutils.h"

#include <physfs.h>

#include "debug.h"

ChatLogger::ChatLogger() :
    mLogDir(""),
    mBaseLogDir(""),
    mServerName(""),
    mLogFileName("")
{
}

ChatLogger::~ChatLogger()
{
    if (mLogFile.is_open())
        mLogFile.close();
}

void ChatLogger::setLogFile(const std::string &logFilename)
{
    if (mLogFile.is_open())
        mLogFile.close();

    mLogFile.open(logFilename.c_str(), std::ios_base::app);
    mLogFileName = logFilename;

    if (!mLogFile.is_open())
    {
        std::cout << "Warning: error while opening " << logFilename <<
            " for writing.\n";
    }
}

void ChatLogger::setLogDir(const std::string &logDir)
{
    mLogDir = logDir;

    if (mLogFile.is_open())
        mLogFile.close();

    DIR *dir = opendir(mLogDir.c_str());
    if (!dir)
        mkdir_r(mLogDir.c_str());
    else
        closedir(dir);
}

void ChatLogger::log(std::string str)
{
    std::string dateStr = getDir();
    std::string logFileName = strprintf("%s/#General.log", dateStr.c_str());
    if (!mLogFile.is_open() || logFileName != mLogFileName)
    {
        setLogDir(dateStr);
        setLogFile(logFileName);
    }

    str = removeColors(str);
    writeTo(mLogFile, str);
}

void ChatLogger::log(std::string name, std::string str)
{
    std::ofstream logFile;
    std::string dateStr = getDir();
    std::string logFileName = strprintf("%s/%s.log",
        dateStr.c_str(), secureName(name).c_str());

    if (!mLogFile.is_open() || logFileName != mLogFileName)
    {
        setLogDir(dateStr);
        setLogFile(logFileName);
    }

    str = removeColors(str);
    writeTo(mLogFile, str);
}

std::string ChatLogger::getDir() const
{
    std::string date;

    time_t rawtime;
    struct tm *timeinfo;
    char buffer [81];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 79, "%Y-%m/%d", timeinfo);

    date = strprintf("%s/%s/%s", mBaseLogDir.c_str(),
        mServerName.c_str(), buffer);

    return date;
}

std::string ChatLogger::secureName(std::string &name) const
{
    for (unsigned int f = 0; f < name.length(); f ++)
    {
        if ((name[f] < '0' || name[f] > '9')
            && (name[f] < 'a' || name[f] > 'z')
            && (name[f] < 'A' || name[f] > 'Z')
            && name[f] != '-' && name[f] != '+' && name[f] != '='
            && name[f] != '.' && name[f] != ',' && name[f] != ')'
            && name[f] != '(' && name[f] != '[' && name[f] != ']'
            && name[f] != '#')
        {
            name[f] = '_';
        }
    }
    return name;
}

void ChatLogger::writeTo(std::ofstream &file, const std::string &str) const
{
    file << str << std::endl;
}

void ChatLogger::setServerName(const std::string &serverName)
{
    mServerName = serverName;
    if (mServerName == "")
        mServerName = config.getStringValue("MostUsedServerName0");

    if (mLogFile.is_open())
        mLogFile.close();

    secureName(mServerName);
    if (mLogDir != "")
    {
        DIR *dir = opendir((mLogDir + PHYSFS_getDirSeparator()
            + mServerName).c_str());
        if (!dir)
        {
            mkdir_r((mLogDir + PHYSFS_getDirSeparator()
                + mServerName).c_str());
        }
        else
        {
            closedir(dir);
        }
    }
}

void ChatLogger::loadLast(std::string name, std::list<std::string> &list,
                          unsigned n)
{
    std::ifstream logFile;
    std::string fileName = strprintf("%s/%s.log", getDir().c_str(),
        secureName(name).c_str());

    logFile.open(fileName.c_str(), std::ios::in);

    if (!logFile.is_open())
        return;

    char line[710];
    while (logFile.getline(line, 700))
    {
        list.push_back(line);
        if (list.size() > n)
            list.pop_front();
    }

    if (logFile.is_open())
        logFile.close();
}

void ChatLogger::clear()
{
    mLogDir = "";
    mServerName = "";
    mLogFileName = "";
}
