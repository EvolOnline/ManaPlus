/*
 *  The ManaPlus Client
 *  Copyright (C) 2004-2009  The Mana World Development Team
 *  Copyright (C) 2009-2010  The Mana Developers
 *  Copyright (C) 2011  The ManaPlus Developers
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

#include "main.h"

#include "utils/gettext.h"

#include "client.h"
#include "log.h"

#include <libxml/parser.h>

#include <getopt.h>
#include <iostream>
#include <physfs.h>

#include "utils/stringutils.h"

#ifdef __MINGW32__
#include <windows.h>
#endif

static void printHelp()
{
    using std::endl;

    std::cout
        << _("manaplus [options] [mana-file]") << endl << endl
        << _("[mana-file] : The mana file is an XML file (.mana)") << endl
        << _("              used to set custom parameters") << endl
        << _("              to the manaplus client.")
        << endl << endl
        << _("Options:") << endl
        << _("  -l --log-file       : Log file to use") << endl
        << _("  -L --chat-log-dir   : Chat log dir to use") << endl
        << _("  -v --version        : Display the version") << endl
        << _("  -h --help           : Display this help") << endl
        << _("  -C --config-dir     : Configuration directory to use") << endl
        << _("  -U --username       : Login with this username") << endl
        << _("  -P --password       : Login with this password") << endl
        << _("  -c --character      : Login with this character") << endl
        << _("  -s --server         : Login server name or IP") << endl
        << _("  -p --port           : Login server port") << endl
        << _("     --update-host    : Use this update host") << endl
        << _("  -D --default        : Choose default character server and "
                                     "character") << endl
        << _("  -u --skip-update    : Skip the update downloads") << endl
        << _("  -d --data           : Directory to load game "
             "data from") << endl
        << _("  -L --localdata-dir  : Directory to use as local data"
             " directory") << endl
        << _("     --screenshot-dir : Directory to store screenshots") << endl
        << _("     --safemode       : Start game in safe mode") << endl
#ifdef USE_OPENGL
        << _("     --no-opengl      : Disable OpenGL for this session") << endl
#endif
        ;
}

static void printVersion()
{
    std::cout << strprintf("ManaPlus client %s", FULL_VERSION) << std::endl;
}

static void parseOptions(int argc, char *argv[], Client::Options &options)
{
    const char *optstring = "hvud:U:P:Dc:p:l:L:C:";

    const struct option long_options[] =
    {
        { "config-dir",     required_argument, 0, 'C' },
        { "data",           required_argument, 0, 'd' },
        { "default",        no_argument,       0, 'D' },
        { "password",       required_argument, 0, 'P' },
        { "character",      required_argument, 0, 'c' },
        { "help",           no_argument,       0, 'h' },
        { "localdata-dir",  required_argument, 0, 'L' },
        { "update-host",    required_argument, 0, 'H' },
        { "port",           required_argument, 0, 'p' },
        { "server",         required_argument, 0, 's' },
        { "skip-update",    no_argument,       0, 'u' },
        { "username",       required_argument, 0, 'U' },
        { "no-opengl",      no_argument,       0, 'O' },
        { "chat-log-dir",   required_argument, 0, 'l' },
        { "version",        no_argument,       0, 'v' },
        { "log-file",       required_argument, 0, 'l' },
        { "chat-log-dir",   required_argument, 0, 'L' },
        { "screenshot-dir", required_argument, 0, 'i' },
        { "safemode",       no_argument,       0, 'm' },
        { 0,                0,                 0, 0 }
    };

    while (optind < argc)
    {
        int result = getopt_long(argc, argv, optstring, long_options, NULL);

        if (result == -1)
            break;

        switch (result)
        {
            case 'C':
                options.configDir = optarg;
                break;
            case 'd':
                options.dataPath = optarg;
                break;
            case 'D':
                options.chooseDefault = true;
                break;
            case '?': // Unknown option
            case ':': // Missing argument
            case 'h':
                options.printHelp = true;
                break;
            case 'H':
                if (checkPath(optarg))
                    options.updateHost = optarg;
                else
                    options.updateHost = "";
                break;
            case 'c':
                options.character = optarg;
                break;
            case 'P':
                options.password = optarg;
                break;
            case 's':
                options.serverName = optarg;
                break;
            case 'p':
                options.serverPort = static_cast<short>(atoi(optarg));
                break;
            case 'u':
                options.skipUpdate = true;
                break;
            case 'U':
                options.username = optarg;
                break;
            case 'v':
                options.printVersion = true;
                break;
            case 'S':
                options.localDataDir = optarg;
                break;
            case 'O':
                options.noOpenGL = true;
                break;
            case 'l':
                options.logFileName = std::string(optarg);
                break;
            case 'L':
                options.chatLogDir = std::string(optarg);
            case 'i':
                options.screenshotDir = optarg;
                break;
            case 'm':
                options.safeMode = true;
                break;
            default:
                break;
        }
    }

    // when there are still options left use the last
    // one as branding file
    if (optind < argc)
    {
        options.brandingPath = argv[optind];
    }
}

#ifdef WIN32
extern "C" char const *_nl_locale_name_default(void);
#endif

static void initInternationalization()
{
#if ENABLE_NLS
#ifdef WIN32
    putenv(("LANG=" + std::string(_nl_locale_name_default())).c_str());
    // mingw doesn't like LOCALEDIR to be defined for some reason
    bindtextdomain("manaplus", "translations/");
#else
#ifdef ENABLE_PORTABLE
    bindtextdomain("manaplus", (std::string(PHYSFS_getBaseDir())
        + "../locale/").c_str());
#else
    bindtextdomain("manaplus", LOCALEDIR);
#endif
#endif
    setlocale(LC_MESSAGES, "");
    bind_textdomain_codeset("manaplus", "UTF-8");
    textdomain("manaplus");
#endif
}

static void xmlNullLogger(void *ctx _UNUSED_, const char *msg _UNUSED_, ...)
{
    // Does nothing, that's the whole point of it
}

// Initialize libxml2 and check for potential ABI mismatches between
// compiled version and the shared library actually used.
static void initXML()
{
    xmlInitParser();
    LIBXML_TEST_VERSION;

    // Suppress libxml2 error messages
    xmlSetGenericErrorFunc(NULL, xmlNullLogger);
}


int main(int argc, char *argv[])
{
#if defined(__MINGW32__)
    // load mingw crash handler. Won't fail if dll is not present.
    // may load libray from current dir, it may not same as program dir
    LoadLibrary("exchndl.dll");
#endif

    // Parse command line options
    Client::Options options;
    parseOptions(argc, argv, options);

    if (options.printHelp)
    {
        printHelp();
        return 0;
    }
    else if (options.printVersion)
    {
        printVersion();
        return 0;
    }

    // Initialize PhysicsFS
    if (!PHYSFS_init(argv[0]))
    {
        std::cout << "Error while initializing PhysFS: "
                  << PHYSFS_getLastError() << std::endl;
        return 1;
    }

    initInternationalization();

    atexit((void(*)()) PHYSFS_deinit);

    initXML();

    Client client(options);
    return client.exec();
}
