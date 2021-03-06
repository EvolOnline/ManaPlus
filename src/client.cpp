/*
 *  The ManaPlus Client
 *  Copyright (C) 2004-2009  The Mana World Development Team
 *  Copyright (C) 2009-2010  The Mana Developers
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

#include "client.h"
#include "main.h"

#include "auctionmanager.h"
#include "chatlogger.h"
#include "configuration.h"
#include "depricatedevent.h"
#include "dropshortcut.h"
#include "emoteshortcut.h"
#include "game.h"
#include "guild.h"
#include "guildmanager.h"
#include "graphicsmanager.h"
#include "graphicsvertexes.h"
#include "itemshortcut.h"
#include "joystick.h"
#include "inputmanager.h"
#include "keyboardconfig.h"
#include "party.h"
#include "playerrelations.h"
#include "sound.h"
#include "statuseffect.h"
#include "units.h"

#include "gui/buydialog.h"
#include "gui/buyselldialog.h"
#include "gui/changeemaildialog.h"
#include "gui/changepassworddialog.h"
#include "gui/charselectdialog.h"
#include "gui/connectiondialog.h"
#include "gui/gui.h"
#include "gui/logindialog.h"
#include "gui/okdialog.h"
#include "gui/quitdialog.h"
#include "gui/registerdialog.h"
#include "gui/npcdialog.h"
#include "gui/selldialog.h"
#include "gui/sdlinput.h"
#include "gui/serverdialog.h"
#include "gui/setup.h"
#include "gui/theme.h"
#include "gui/unregisterdialog.h"
#include "gui/updaterwindow.h"
#include "gui/userpalette.h"
#include "gui/worldselectdialog.h"

#include "gui/widgets/button.h"
#include "gui/widgets/desktop.h"

#include "net/charhandler.h"
#include "net/gamehandler.h"
#include "net/generalhandler.h"
#include "net/guildhandler.h"
#include "net/logindata.h"
#include "net/loginhandler.h"
#include "net/net.h"
#include "net/npchandler.h"
#include "net/partyhandler.h"
#include "net/worldinfo.h"

#include "resources/beinginfo.h"
#include "resources/chardb.h"
#include "resources/colordb.h"
#include "resources/emotedb.h"
#include "resources/image.h"
#include "resources/imagehelper.h"
#include "resources/openglimagehelper.h"
#include "resources/sdlimagehelper.h"
#include "resources/itemdb.h"
#include "resources/mapdb.h"
#include "resources/monsterdb.h"
#ifdef MANASERV_SUPPORT
#include "resources/specialdb.h"
#endif
#include "resources/npcdb.h"
#include "resources/resourcemanager.h"

#include "utils/gettext.h"
#include "utils/mkdir.h"
#include "utils/paths.h"
#include "utils/stringutils.h"

#include "utils/translation/translationmanager.h"

#include "test/testlauncher.h"
#include "test/testmain.h"

#ifdef __APPLE__
#include <CoreFoundation/CFBundle.h>
#endif

#include <physfs.h>
#include <SDL_image.h>

#ifdef WIN32
#include <SDL_syswm.h>
#include "utils/specialfolder.h"
#else
#include <cerrno>
#endif

#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include "mumblemanager.h"

#include "debug.h"

/**
 * Tells the max tick value,
 * setting it back to zero (and start again).
 */
static const int MAX_TICK_VALUE = 10000;

static const int defaultSfxVolume = 100;
static const int defaultMusicVolume = 60;

// TODO: Get rid fo these globals
std::string errorMessage;
ErrorListener errorListener;
LoginData loginData;

Configuration config;             /**< XML file configuration reader */
Configuration serverConfig;       /**< XML file server configuration reader */
Configuration branding;           /**< XML branding information reader */
Configuration paths;              /**< XML default paths information reader */
Logger *logger = nullptr;         /**< Log object */
ChatLogger *chatLogger = nullptr; /**< Chat log object */
KeyboardConfig keyboard;
UserPalette *userPalette = nullptr;
Graphics *mainGraphics = nullptr;

Sound sound;

uint32_t nextTick(uint32_t interval, void *param A_UNUSED);
uint32_t nextSecond(uint32_t interval, void *param A_UNUSED);

void ErrorListener::action(const gcn::ActionEvent &)
{
    Client::setState(STATE_CHOOSE_SERVER);
}

volatile int tick_time;       /**< Tick counter */
volatile int fps = 0;         /**< Frames counted in the last second */
volatile int lps = 0;         /**< Logic processed per second */
volatile int frame_count = 0; /**< Counts the frames during one second */
volatile int logic_count = 0; /**< Counts the logic during one second */
volatile int cur_time;
volatile bool runCounters;
bool isSafeMode = false;
int serverVersion = 0;
int start_time;

int textures_count = 0;

#ifdef WIN32
extern "C" char const *_nl_locale_name_default(void);
#endif

/**
 * Advances game logic counter.
 * Called every 10 milliseconds by SDL_AddTimer()
 * @see MILLISECONDS_IN_A_TICK value
 */
uint32_t nextTick(uint32_t interval, void *param A_UNUSED)
{
    tick_time++;
    if (tick_time == MAX_TICK_VALUE)
        tick_time = 0;
    return interval;
}

/**
 * Updates fps.
 * Called every seconds by SDL_AddTimer()
 */
uint32_t nextSecond(uint32_t interval, void *param A_UNUSED)
{
    fps = frame_count;
    lps = logic_count;
    frame_count = 0;
    logic_count = 0;

    return interval;
}

/**
 * @return the elapsed time in milliseconds
 * between two tick values.
 */
int get_elapsed_time(int startTime)
{
    if (startTime <= tick_time)
    {
        return (tick_time - startTime) * MILLISECONDS_IN_A_TICK;
    }
    else
    {
        return (tick_time + (MAX_TICK_VALUE - startTime))
            * MILLISECONDS_IN_A_TICK;
    }
}

int get_elapsed_time1(int startTime)
{
    if (startTime <= tick_time)
        return tick_time - startTime;
    else
        return tick_time + (MAX_TICK_VALUE - startTime);
}

// This anonymous namespace hides whatever is inside from other modules.
namespace
{

class AccountListener : public gcn::ActionListener
{
    public:
        void action(const gcn::ActionEvent &)
        {
            Client::setState(STATE_CHAR_SELECT);
        }
} accountListener;

class LoginListener : public gcn::ActionListener
{
    public:
        void action(const gcn::ActionEvent &)
        {
            Client::setState(STATE_PRE_LOGIN);
        }
} loginListener;

} // anonymous namespace


Client *Client::mInstance = nullptr;

Client::Client(const Options &options):
    mOptions(options),
    mServerConfigDir(""),
    mUsersDir(""),
    mNpcsDir(""),
    mGame(nullptr),
    mCurrentDialog(nullptr),
    mQuitDialog(nullptr),
    mDesktop(nullptr),
    mSetupButton(nullptr),
    mVideoButton(nullptr),
    mThemesButton(nullptr),
    mPerfomanceButton(nullptr),
    mState(STATE_CHOOSE_SERVER),
    mOldState(STATE_START),
    mIcon(nullptr),
    mLogicCounterId(nullptr),
    mSecondsCounterId(nullptr),
    mLimitFps(false),
    mConfigAutoSaved(false),
    mIsMinimized(false),
    mInputFocused(true),
    mMouseFocused(true),
    mGuiAlpha(1.0f)
{
    mInstance = this;
}

void Client::testsInit()
{
    if (!mOptions.test.empty() && mOptions.test != "99")
    {
        gameInit();
    }
    else
    {
        logger = new Logger;
        initLocalDataDir();
        initConfigDir();
    }
}

void Client::gameInit()
{
    logger = new Logger;

    // Load branding information
    if (!mOptions.brandingPath.empty())
        branding.init(mOptions.brandingPath);
    branding.setDefaultValues(getBrandingDefaults());

    initRootDir();
    initHomeDir();

    // Configure logger
    if (!mOptions.logFileName.empty())
        logger->setLogFile(mOptions.logFileName);
    else
        logger->setLogFile(mLocalDataDir + std::string("/manaplus.log"));

    initConfiguration();
    logger->log("init 4");
    logger->setDebugLog(config.getBoolValue("debugLog"));

    storeSafeParameters();

#if ENABLE_NLS
    std::string lang = config.getValue("lang", "");
#ifdef WIN32
    if (lang.empty())
        lang = std::string(_nl_locale_name_default());

    putenv((char*)("LANG=" + lang).c_str());
    putenv((char*)("LANGUAGE=" + lang).c_str());
    // mingw doesn't like LOCALEDIR to be defined for some reason
    if (lang != "C")
        bindtextdomain("manaplus", "translations/");
#else
    if (!lang.empty())
    {
        putenv(const_cast<char*>(("LANG=" + lang).c_str()));
        putenv(const_cast<char*>(("LANGUAGE=" + lang).c_str()));
    }
#ifdef ENABLE_PORTABLE
    bindtextdomain("manaplus", (std::string(PHYSFS_getBaseDir())
        + "../locale/").c_str());
#else
    bindtextdomain("manaplus", LOCALEDIR);
#endif
#endif
    setlocale(LC_MESSAGES, lang.c_str());
    bind_textdomain_codeset("manaplus", "UTF-8");
    textdomain("manaplus");
#endif

#if defined(WIN32) || defined(__APPLE__)
    if (config.getBoolValue("centerwindow"))
        putenv("SDL_VIDEO_CENTERED=1");
    else
        putenv("SDL_VIDEO_CENTERED=0");
#endif

    chatLogger = new ChatLogger;
    if (mOptions.chatLogDir == "")
        chatLogger->setBaseLogDir(mLocalDataDir + std::string("/logs/"));
    else
        chatLogger->setBaseLogDir(mOptions.chatLogDir);

    logger->setLogToStandardOut(config.getBoolValue("logToStandardOut"));

    // Log the mana version
    logger->log("ManaPlus %s", FULL_VERSION);
    logger->log("Start configPath: " + config.getConfigPath());

    initScreenshotDir();

    // Initialize SDL
    logger->log1("Initializing SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
        logger->safeError(strprintf("Could not initialize SDL: %s",
                      SDL_GetError()));
    }
    atexit(SDL_Quit);

    initPacketLimiter();
    SDL_EnableUNICODE(1);
    applyKeyRepeat();

    // disable unused SDL events
    SDL_EventState(SDL_VIDEOEXPOSE, SDL_IGNORE);
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    SDL_WM_SetCaption(strprintf("%s %s",
        branding.getStringValue("appName").c_str(),
        SMALL_VERSION).c_str(), nullptr);

    ResourceManager *resman = ResourceManager::getInstance();

    if (!resman->setWriteDir(mLocalDataDir))
    {
        logger->error(strprintf("%s couldn't be set as home directory! "
                                "Exiting.", mLocalDataDir.c_str()));
    }


    resman->addToSearchPath(PKG_DATADIR "data/perserver/default", false);

#if defined __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path,
        PATH_MAX))
    {
        fprintf(stderr, "Can't find Resources directory\n");
    }
    CFRelease(resourcesURL);
    //possible crash
    strncat(path, "/data", PATH_MAX - 1);
    resman->addToSearchPath(path, false);
// possible this need for support run client from dmg images.
//    mPackageDir = path;

    resman->addToSearchPath(PKG_DATADIR "data", false);
    mPackageDir = PKG_DATADIR "data";
#else
    resman->addToSearchPath(PKG_DATADIR "data", false);
    mPackageDir = PKG_DATADIR "data";
#endif

    resman->addToSearchPath("data", false);

    // Add branding/data to PhysFS search path
    if (!mOptions.brandingPath.empty())
    {
        std::string path = mOptions.brandingPath;

        // Strip blah.mana from the path
#ifdef WIN32
        int loc1 = path.find_last_of('/');
        int loc2 = path.find_last_of('\\');
        int loc = static_cast<int>(std::max(loc1, loc2));
#else
        int loc = static_cast<int>(path.find_last_of('/'));
#endif
        if (loc > 0)
            resman->addToSearchPath(path.substr(0, loc + 1) + "data", false);
    }

    if (mOptions.dataPath.empty()
        && !branding.getStringValue("dataPath").empty())
    {
        if (isRealPath(branding.getStringValue("dataPath")))
        {
            mOptions.dataPath = branding.getStringValue("dataPath");
        }
        else
        {
            mOptions.dataPath = branding.getDirectory()
                + PHYSFS_getDirSeparator()
                + branding.getStringValue("dataPath");
        }
        mOptions.skipUpdate = true;
    }

    // Add the main data directories to our PhysicsFS search path
    if (!mOptions.dataPath.empty())
        resman->addToSearchPath(mOptions.dataPath, false);

    // Add the local data directory to PhysicsFS search path
    resman->addToSearchPath(mLocalDataDir, false);

    //resman->selectSkin();

    TranslationManager::loadCurrentLang();

    std::string iconFile = branding.getValue("appIcon", "icons/manaplus");
#ifdef WIN32
    iconFile += ".ico";
#else
    iconFile += ".png";
#endif
    iconFile = resman->getPath(iconFile);
    logger->log("Loading icon from file: %s", iconFile.c_str());

#ifdef WIN32
    static SDL_SysWMinfo pInfo;
    SDL_GetWMInfo(&pInfo);
    // Attempt to load icon from .ico file
    HICON icon = (HICON) LoadImage(nullptr, iconFile.c_str(),
        IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
    // If it's failing, we load the default resource file.
    if (!icon)
        icon = LoadIcon(GetModuleHandle(nullptr), "A");

    if (icon)
        SetClassLong(pInfo.window, GCL_HICON, (LONG)(icon));
#else
    mIcon = IMG_Load(iconFile.c_str());
    if (mIcon)
    {
        SDL_SetAlpha(mIcon, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
        SDL_WM_SetIcon(mIcon, nullptr);
    }
#endif

#ifdef WIN32
    if (mOptions.test.empty() && !config.getBoolValue("videodetected"))
    {
        int val = graphicsManager.startDetection();
        if (val >= 0 && val <= 2)
            config.setValue("opengl", val);
        config.setValue("videodetected", true);
    }
#endif

#if defined USE_OPENGL
    OpenGLImageHelper::setBlur(config.getBoolValue("blur"));
    SDLImageHelper::SDLSetEnableAlphaCache(config.getBoolValue("alphaCache")
        && !config.getIntValue("opengl"));
    ImageHelper::setEnableAlpha(config.getFloatValue("guialpha") != 1.0f
        || config.getIntValue("opengl"));
#else
    SDLImageHelper::SDLSetEnableAlphaCache(config.getBoolValue("alphaCache"));
    ImageHelper::setEnableAlpha(config.getFloatValue("guialpha") != 1.0f);
#endif

    graphicsManager.initGraphics(mOptions.noOpenGL);

    runCounters = config.getBoolValue("packetcounters");

    applyVSync();

    graphicsManager.setVideoMode();

    applyGrabMode();
    applyGamma();

    // Initialize for drawing
    mainGraphics->_beginDraw();

    Theme::selectSkin();
//    Theme::prepareThemePath();

    // Initialize the item and emote shortcuts.
    for (int f = 0; f < SHORTCUT_TABS; f ++)
        itemShortcut[f] = new ItemShortcut(f);

    emoteShortcut = new EmoteShortcut;

    // Initialize the drop shortcuts.
    dropShortcut = new DropShortcut;

    gui = new Gui(mainGraphics);

    // Initialize sound engine
    try
    {
        if (config.getBoolValue("sound"))
            sound.init();

        sound.setSfxVolume(config.getIntValue("sfxVolume"));
        sound.setMusicVolume(config.getIntValue("musicVolume"));
    }
    catch (const char *err)
    {
        mState = STATE_ERROR;
        errorMessage = err;
        logger->log("Warning: %s", err);
    }

    // Initialize keyboard
    keyboard.init();
    inputManager.init();

    // Initialise player relations
    player_relations.init();

    Joystick::init();

    userPalette = new UserPalette;
    setupWindow = new Setup;

    sound.playMusic(branding.getValue("loginMusic", "Magick - Real.ogg"));

    // Initialize default server
    mCurrentServer.hostname = mOptions.serverName;
    mCurrentServer.port = mOptions.serverPort;

    loginData.username = mOptions.username;
    loginData.password = mOptions.password;
    loginData.remember = serverConfig.getValue("remember", 1);
    loginData.registerLogin = false;

    if (mCurrentServer.hostname.empty())
    {
        mCurrentServer.hostname =
            branding.getValue("defaultServer", "").c_str();
        mOptions.serverName = mCurrentServer.hostname;
    }

    if (mCurrentServer.port == 0)
    {
        mCurrentServer.port = static_cast<short>(branding.getValue(
            "defaultPort", DEFAULT_PORT));
        mCurrentServer.type = ServerInfo::parseType(
            branding.getValue("defaultServerType", "tmwathena"));
    }

    if (chatLogger)
        chatLogger->setServerName(mCurrentServer.hostname);

    if (loginData.username.empty() && loginData.remember)
        loginData.username = serverConfig.getValue("username", "");

    if (mState != STATE_ERROR)
        mState = STATE_CHOOSE_SERVER;

    // Initialize logic and seconds counters
    tick_time = 0;
    mLogicCounterId = SDL_AddTimer(MILLISECONDS_IN_A_TICK, nextTick, nullptr);
    mSecondsCounterId = SDL_AddTimer(1000, nextSecond, nullptr);

    const int fpsLimit = config.getIntValue("fpslimit");
    mLimitFps = fpsLimit > 0;

    // Initialize frame limiting
    mFpsManager.framecount = 0;
    mFpsManager.rateticks = 0;
    mFpsManager.lastticks = 0;
    mFpsManager.rate = 0;

    SDL_initFramerate(&mFpsManager);
    setFramerate(fpsLimit);
    config.addListener("fpslimit", this);
    config.addListener("guialpha", this);
    config.addListener("gamma", this);
    config.addListener("particleEmitterSkip", this);
    config.addListener("vsync", this);
    config.addListener("repeateDelay", this);
    config.addListener("repeateInterval", this);
    setGuiAlpha(config.getFloatValue("guialpha"));

    optionChanged("fpslimit");

    start_time = static_cast<int>(time(nullptr));

    // Initialize PlayerInfo
    PlayerInfo::init();
}

Client::~Client()
{
    if (!mOptions.testMode)
        gameClear();
    else
        testsClear();
}

void Client::testsClear()
{
    if (!mOptions.test.empty())
    {
        gameClear();
    }
    else
    {
        BeingInfo::clear();

        //delete logger;
        //logger = nullptr;
    }
}

void Client::gameClear()
{
    if (logger)
        logger->log1("Quitting1");
    config.removeListeners(this);

    SDL_RemoveTimer(mLogicCounterId);
    SDL_RemoveTimer(mSecondsCounterId);

    // Unload XML databases
    CharDB::unload();
    ColorDB::unload();
    EmoteDB::unload();
    ItemDB::unload();
    MonsterDB::unload();
    NPCDB::unload();
    StatusEffect::unload();

    if (Net::getLoginHandler())
        Net::getLoginHandler()->clearWorlds();

    delete mumbleManager;
    mumbleManager = nullptr;

    PlayerInfo::deinit();

    // Before config.write() since it writes the shortcuts to the config
    for (int f = 0; f < SHORTCUT_TABS; f ++)
    {
        delete itemShortcut[f];
        itemShortcut[f] = nullptr;
    }
    delete emoteShortcut;
    emoteShortcut = nullptr;
    delete dropShortcut;
    dropShortcut = nullptr;

    player_relations.store();

    if (logger)
        logger->log1("Quitting2");

    delete gui;
    gui = nullptr;

    if (logger)
        logger->log1("Quitting3");

    delete mainGraphics;
    mainGraphics = nullptr;

    delete imageHelper;
    imageHelper = nullptr;

    if (logger)
        logger->log1("Quitting4");

    XML::cleanupXML();

    if (logger)
        logger->log1("Quitting5");

    BeingInfo::clear();

    // Shutdown sound
    sound.close();

    if (logger)
        logger->log1("Quitting6");

    ActorSprite::unload();

    ResourceManager::deleteInstance();

    if (logger)
        logger->log1("Quitting8");

    SDL_FreeSurface(mIcon);

    if (logger)
        logger->log1("Quitting9");

    delete userPalette;
    userPalette = nullptr;

    delete joystick;
    joystick = nullptr;

    keyboard.deinit();

    if (logger)
        logger->log1("Quitting10");

#ifdef DEBUG_CONFIG
    config.enableKeyLogging();
#endif
    config.removeOldKeys();
    config.write();
    serverConfig.write();

    config.clear();
    serverConfig.clear();

    if (logger)
        logger->log1("Quitting11");

#ifdef DEBUG_OPENGL_LEAKS
    if (logger)
        logger->log("textures left: %d", textures_count);
#endif

    delete chatLogger;
    chatLogger = nullptr;

    //delete logger;
    //logger = nullptr;

    TranslationManager::close();

    mInstance = nullptr;
}

int Client::testsExec()
{
    if (mOptions.test.empty())
    {
        TestMain test;
        return test.exec();
    }
    else
    {
        TestLauncher launcher(mOptions.test);
        return launcher.exec();
    }
    return 0;
}

int Client::gameExec()
{
    int lastTickTime = tick_time;

    if (!mumbleManager)
        mumbleManager = new MumbleManager();

    SDL_Event event;

    while (mState != STATE_EXIT)
    {
        if (mGame)
        {
            // Let the game handle the events while it is active
            mGame->handleInput();
        }
        else
        {
            // Handle SDL events
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_QUIT:
                        mState = STATE_EXIT;
                        break;

                    case SDL_KEYDOWN:
                        if (inputManager.handleAssignKey(
                            event, INPUT_KEYBOARD))
                        {
                            continue;
                        }
                        inputManager.updateConditionMask();
                        break;

                    case SDL_KEYUP:
                        if (inputManager.handleAssignKey(
                            event, INPUT_KEYBOARD))
                        {
                            continue;
                        }
                        inputManager.updateConditionMask();
                        break;

                    case SDL_JOYBUTTONDOWN:
                        inputManager.handleAssignKey(event, INPUT_JOYSTICK);
                        break;

                    default:
                        break;

                    case SDL_VIDEORESIZE:
                        resizeVideo(event.resize.w, event.resize.h, false);
                        break;
                }

                guiInput->pushInput(event);
                if (player_node && mumbleManager)
                {
                    mumbleManager->setPos(player_node->getTileX(),
                        player_node->getTileY(), player_node->getDirection());
                }
            }
        }

        if (Net::getGeneralHandler())
            Net::getGeneralHandler()->flushNetwork();

        int k = 0;
        while (lastTickTime != tick_time && k < 40)
        {
            if (gui)
                gui->logic();
            if (mGame)
                mGame->logic();
            else if (gui)
                gui->handleInput();

            sound.logic();

            ++lastTickTime;
            k ++;
        }
        logic_count += k;
        if (gui)
            gui->slowLogic();
        if (mGame)
            mGame->slowLogic();

        // This is done because at some point tick_time will wrap.
        lastTickTime = tick_time;

        // Update the screen when application is active, delay otherwise.
        if (SDL_GetAppState() & SDL_APPACTIVE)
        {
            frame_count++;
            if (gui)
                gui->draw();
            mainGraphics->updateScreen();
//            logger->log("active");
        }
        else
        {
//            logger->log("inactive");
            SDL_Delay(100);
        }

        if (mLimitFps)
            SDL_framerateDelay(&mFpsManager);

        // TODO: Add connect timeouts
        if (mState == STATE_CONNECT_GAME &&
            Net::getGameHandler()->isConnected())
        {
            Net::getLoginHandler()->disconnect();
        }
        else if (mState == STATE_CONNECT_SERVER &&
                 mOldState == STATE_CHOOSE_SERVER)
        {
            mServerName = mCurrentServer.hostname;
            initServerConfig(mCurrentServer.hostname);
            if (mOptions.username.empty())
                loginData.username = serverConfig.getValue("username", "");
            else
                loginData.username = mOptions.username;

            loginData.remember = serverConfig.getValue("remember", 1);

            Net::connectToServer(mCurrentServer);

            if (mumbleManager)
                mumbleManager->setServer(mCurrentServer.hostname);

            GuildManager::init();
            AuctionManager::init();

            if (!mConfigAutoSaved)
            {
                mConfigAutoSaved = true;
                config.write();
            }
        }
        else if (mState == STATE_CONNECT_SERVER &&
                 mOldState != STATE_CHOOSE_SERVER &&
                 Net::getLoginHandler()->isConnected())
        {
            mState = STATE_PRE_LOGIN;
        }
        else if (mState == STATE_WORLD_SELECT && mOldState == STATE_UPDATE)
        {
            if (Net::getLoginHandler()->getWorlds().size() < 2)
                mState = STATE_PRE_LOGIN;
        }
        else if (mOldState == STATE_START ||
                 (mOldState == STATE_GAME && mState != STATE_GAME))
        {
            if (!gui)
                break;

            gcn::Container *top = static_cast<gcn::Container*>(gui->getTop());

            if (!top)
                break;

            mDesktop = new Desktop;
            top->add(mDesktop);
            int x = top->getWidth();
            mSetupButton = new Button(_("Setup"), "Setup", this);
            x -= mSetupButton->getWidth() + 3;
            mSetupButton->setPosition(x, 3);
            top->add(mSetupButton);

#ifndef WIN32
            mPerfomanceButton = new Button(
                _("Perfomance"), "Perfomance", this);
            x -= mPerfomanceButton->getWidth() + 6;
            mPerfomanceButton->setPosition(x, 3);
            top->add(mPerfomanceButton);

            mVideoButton = new Button(_("Video"), "Video", this);
            x -= mVideoButton->getWidth() + 6;
            mVideoButton->setPosition(x, 3);
            top->add(mVideoButton);

            mThemesButton = new Button(_("Themes"), "Themes", this);
            x -= mThemesButton->getWidth() + 6;
            mThemesButton->setPosition(x, 3);
            top->add(mThemesButton);
#endif

            mDesktop->setSize(mainGraphics->getWidth(),
                mainGraphics->getHeight());
        }

        if (mState == STATE_SWITCH_LOGIN && mOldState == STATE_GAME)
            Net::getGameHandler()->disconnect();

        if (mState != mOldState)
        {
            DepricatedEvent evt(EVENT_STATECHANGE);
            evt.setInt("oldState", mOldState);
            evt.setInt("newState", mState);
            DepricatedEvent::trigger(CHANNEL_CLIENT, evt);

            if (mOldState == STATE_GAME)
            {
                delete mGame;
                mGame = nullptr;
                Game::clearInstance();
                ResourceManager *resman = ResourceManager::getInstance();
                if (resman)
                    resman->cleanOrphans();
                Party::clearParties();
                Guild::clearGuilds();
                Net::getNpcHandler()->clearDialogs();
                Net::getGuildHandler()->clear();
                Net::getPartyHandler()->clear();
                if (chatLogger)
                    chatLogger->clear();
            }

            mOldState = mState;

            // Get rid of the dialog of the previous state
            delete mCurrentDialog;
            mCurrentDialog = nullptr;
            // State has changed, while the quitDialog was active, it might
            // not be correct anymore
            if (mQuitDialog)
            {
                mQuitDialog->scheduleDelete();
                mQuitDialog = nullptr;
            }

            switch (mState)
            {
                case STATE_CHOOSE_SERVER:
                    logger->log1("State: CHOOSE SERVER");

                    loginData.clearUpdateHost();
                    serverVersion = 0;

                    // Allow changing this using a server choice dialog
                    // We show the dialog box only if the command-line
                    // options weren't set.
                    if (mOptions.serverName.empty() && mOptions.serverPort == 0
                        && !branding.getValue("onlineServerList", "a").empty())
                    {
                        // Don't allow an alpha opacity
                        // lower than the default value
                        Theme::instance()->setMinimumOpacity(0.8f);

                        mCurrentDialog = new ServerDialog(&mCurrentServer,
                                                          mConfigDir);
                    }
                    else
                    {
                        mState = STATE_CONNECT_SERVER;

                        // Reset options so that cancelling or connect
                        // timeout will show the server dialog.
                        mOptions.serverName.clear();
                        mOptions.serverPort = 0;
                    }
                    break;

                case STATE_CONNECT_SERVER:
                    logger->log1("State: CONNECT SERVER");
                    loginData.updateHosts.clear();
                    mCurrentDialog = new ConnectionDialog(
                            _("Connecting to server"), STATE_SWITCH_SERVER);
                    TranslationManager::loadCurrentLang();
                    break;

                case STATE_PRE_LOGIN:
                    logger->log1("State: PRE_LOGIN");
//                    if (serverVersion < 5)
//                        setState(STATE_LOGIN);
                    break;

                case STATE_LOGIN:
                    logger->log1("State: LOGIN");
                    // Don't allow an alpha opacity
                    // lower than the default value
                    Theme::instance()->setMinimumOpacity(0.8f);

                    loginData.updateType
                        = serverConfig.getValue("updateType", 1);

                    if (mOptions.username.empty()
                        || mOptions.password.empty())
                    {
                        mCurrentDialog = new LoginDialog(&loginData,
                            mCurrentServer.hostname, &mOptions.updateHost);
                    }
                    else
                    {
                        mState = STATE_LOGIN_ATTEMPT;
                        // Clear the password so that when login fails, the
                        // dialog will show up next time.
                        mOptions.password.clear();
                    }
                    break;

                case STATE_LOGIN_ATTEMPT:
                    logger->log1("State: LOGIN ATTEMPT");
                    accountLogin(&loginData);
                    mCurrentDialog = new ConnectionDialog(
                            _("Logging in"), STATE_SWITCH_SERVER);
                    break;

                case STATE_WORLD_SELECT:
                    logger->log1("State: WORLD SELECT");
                    {
                        TranslationManager::loadCurrentLang();
                        Worlds worlds = Net::getLoginHandler()->getWorlds();

                        if (worlds.empty())
                        {
                            // Trust that the netcode knows what it's doing
                            mState = STATE_UPDATE;
                        }
                        else if (worlds.size() == 1)
                        {
                            Net::getLoginHandler()->chooseServer(0);
                            mState = STATE_UPDATE;
                        }
                        else
                        {
                            mCurrentDialog = new WorldSelectDialog(worlds);
                            if (mOptions.chooseDefault)
                            {
                                static_cast<WorldSelectDialog*>(mCurrentDialog)
                                    ->action(gcn::ActionEvent(nullptr, "ok"));
                            }
                        }
                    }
                    break;

                case STATE_WORLD_SELECT_ATTEMPT:
                    logger->log1("State: WORLD SELECT ATTEMPT");
                    mCurrentDialog = new ConnectionDialog(
                            _("Entering game world"), STATE_WORLD_SELECT);
                    break;

                case STATE_UPDATE:
                    // Determine which source to use for the update host
                    if (!mOptions.updateHost.empty())
                        mUpdateHost = mOptions.updateHost;
                    else
                        mUpdateHost = loginData.updateHost;
                    initUpdatesDir();

                    if (mOptions.skipUpdate)
                    {
                        mState = STATE_LOAD_DATA;
                    }
                    else if (loginData.updateType & LoginData::Upd_Skip)
                    {
                        UpdaterWindow::loadLocalUpdates(mLocalDataDir
                            + PHYSFS_getDirSeparator() + mUpdatesDir);
                        mState = STATE_LOAD_DATA;
                    }
                    else
                    {
                        logger->log1("State: UPDATE");
                        mCurrentDialog = new UpdaterWindow(mUpdateHost,
                            mLocalDataDir + PHYSFS_getDirSeparator()
                            + mUpdatesDir, mOptions.dataPath.empty(),
                            loginData.updateType);
                    }
                    break;

                case STATE_LOAD_DATA:
                {
                    logger->log1("State: LOAD DATA");

                    ResourceManager *resman = ResourceManager::getInstance();

                    // If another data path has been set,
                    // we don't load any other files...
                    if (mOptions.dataPath.empty())
                    {
                        // Add customdata directory
                        resman->searchAndAddArchives(
                            "customdata/",
                            "zip",
                            false);
                    }

                    if (!mOptions.skipUpdate)
                    {
                        resman->searchAndAddArchives(
                            mUpdatesDir + "/local/",
                            "zip",
                            false);

                        resman->addToSearchPath(mLocalDataDir
                            + PHYSFS_getDirSeparator()
                            + mUpdatesDir + "/local/", false);
                    }

                    // Read default paths file 'data/paths.xml'
                    paths.init("paths.xml", true);
                    paths.setDefaultValues(getPathsDefaults());
                    if (!SpriteReference::Empty)
                    {
                        SpriteReference::Empty = new SpriteReference(
                            paths.getStringValue("spriteErrorFile"), 0);
                    }

                    if (!BeingInfo::unknown)
                        BeingInfo::unknown = new BeingInfo;

                    TranslationManager::loadCurrentLang();

                    DepricatedEvent evt2(EVENT_STATECHANGE);
                    evt2.setInt("newState", STATE_LOAD_DATA);
                    evt2.setInt("oldState", mOldState);
                    DepricatedEvent::trigger(CHANNEL_CLIENT, evt2);

                    // Load XML databases
                    CharDB::load();
                    ColorDB::load();
                    MapDB::load();
                    ItemDB::load();
                    Being::load(); // Hairstyles
                    MonsterDB::load();
#ifdef MANASERV_SUPPORT
                    SpecialDB::load();
#endif
                    NPCDB::load();
                    EmoteDB::load();
                    StatusEffect::load();
                    Units::loadUnits();

                    ActorSprite::load();

                    if (mDesktop)
                        mDesktop->reloadWallpaper();

                    mState = STATE_GET_CHARACTERS;
                    break;
                }
                case STATE_GET_CHARACTERS:
                    logger->log1("State: GET CHARACTERS");
                    Net::getCharHandler()->requestCharacters();
                    mCurrentDialog = new ConnectionDialog(
                            _("Requesting characters"),
                            STATE_SWITCH_SERVER);
                    break;

                case STATE_CHAR_SELECT:
                    logger->log1("State: CHAR SELECT");
                    // Don't allow an alpha opacity
                    // lower than the default value
                    Theme::instance()->setMinimumOpacity(0.8f);

                    mCurrentDialog = new CharSelectDialog(&loginData);

                    if (!(static_cast<CharSelectDialog*>(mCurrentDialog))
                        ->selectByName(mOptions.character,
                        CharSelectDialog::Choose))
                    {
                        (static_cast<CharSelectDialog*>(mCurrentDialog))
                            ->selectByName(
                            serverConfig.getValue("lastCharacter", ""),
                            mOptions.chooseDefault ?
                            CharSelectDialog::Choose :
                            CharSelectDialog::Focus);
                    }

                    // Choosing character on the command line should work only
                    // once, clear it so that 'switch character' works.
                    mOptions.character.clear();

                    break;

                case STATE_CONNECT_GAME:
                    logger->log1("State: CONNECT GAME");

                    Net::getGameHandler()->connect();
                    mCurrentDialog = new ConnectionDialog(
                            _("Connecting to the game server"),
                            Net::getNetworkType() != ServerInfo::MANASERV ?
                            STATE_CHOOSE_SERVER : STATE_SWITCH_CHARACTER);
                    break;

                case STATE_CHANGE_MAP:
                    logger->log1("State: CHANGE_MAP");

                    Net::getGameHandler()->connect();
                    mCurrentDialog = new ConnectionDialog(
                            _("Changing game servers"),
                            STATE_SWITCH_CHARACTER);
                    break;

                case STATE_GAME:
                    if (player_node)
                    {
                        logger->log("Memorizing selected character %s",
                            player_node->getName().c_str());
                        serverConfig.setValue("lastCharacter",
                            player_node->getName());
                        if (mumbleManager)
                            mumbleManager->setPlayer(player_node->getName());
                    }

                    // Fade out logon-music here too to give the desired effect
                    // of "flowing" into the game.
                    sound.fadeOutMusic(1000);

                    // Allow any alpha opacity
                    Theme::instance()->setMinimumOpacity(-1.0f);

                    delete mSetupButton;
                    mSetupButton = nullptr;
                    delete mVideoButton;
                    mVideoButton = nullptr;
                    delete mThemesButton;
                    mThemesButton = nullptr;
                    delete mPerfomanceButton;
                    mPerfomanceButton = nullptr;
                    delete mDesktop;
                    mDesktop = nullptr;

                    mCurrentDialog = nullptr;

                    logger->log1("State: GAME");
                    if (Net::getGeneralHandler())
                        Net::getGeneralHandler()->reloadPartially();
                    mGame = new Game;
                    break;

                case STATE_LOGIN_ERROR:
                    logger->log1("State: LOGIN ERROR");
                    mCurrentDialog = new OkDialog(_("Error"),
                        errorMessage, DIALOG_ERROR);
                    mCurrentDialog->addActionListener(&loginListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    break;

                case STATE_ACCOUNTCHANGE_ERROR:
                    logger->log1("State: ACCOUNT CHANGE ERROR");
                    mCurrentDialog = new OkDialog(_("Error"),
                        errorMessage, DIALOG_ERROR);
                    mCurrentDialog->addActionListener(&accountListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    break;

                case STATE_REGISTER_PREP:
                    logger->log1("State: REGISTER_PREP");
                    Net::getLoginHandler()->getRegistrationDetails();
                    mCurrentDialog = new ConnectionDialog(
                            _("Requesting registration details"), STATE_LOGIN);
                    break;

                case STATE_REGISTER:
                    logger->log1("State: REGISTER");
                    mCurrentDialog = new RegisterDialog(&loginData);
                    break;

                case STATE_REGISTER_ATTEMPT:
                    logger->log("Username is %s", loginData.username.c_str());
                    Net::getLoginHandler()->registerAccount(&loginData);
                    break;

                case STATE_CHANGEPASSWORD:
                    logger->log1("State: CHANGE PASSWORD");
                    mCurrentDialog = new ChangePasswordDialog(&loginData);
                    break;

                case STATE_CHANGEPASSWORD_ATTEMPT:
                    logger->log1("State: CHANGE PASSWORD ATTEMPT");
                    Net::getLoginHandler()->changePassword(loginData.username,
                        loginData.password, loginData.newPassword);
                    break;

                case STATE_CHANGEPASSWORD_SUCCESS:
                    logger->log1("State: CHANGE PASSWORD SUCCESS");
                    mCurrentDialog = new OkDialog(_("Password Change"),
                        _("Password changed successfully!"), DIALOG_ERROR);
                    mCurrentDialog->addActionListener(&accountListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    loginData.password = loginData.newPassword;
                    loginData.newPassword = "";
                    break;

                case STATE_CHANGEEMAIL:
                    logger->log1("State: CHANGE EMAIL");
                    mCurrentDialog = new ChangeEmailDialog(&loginData);
                    break;

                case STATE_CHANGEEMAIL_ATTEMPT:
                    logger->log1("State: CHANGE EMAIL ATTEMPT");
                    Net::getLoginHandler()->changeEmail(loginData.email);
                    break;

                case STATE_CHANGEEMAIL_SUCCESS:
                    logger->log1("State: CHANGE EMAIL SUCCESS");
                    mCurrentDialog = new OkDialog(_("Email Change"),
                        _("Email changed successfully!"), DIALOG_ERROR);
                    mCurrentDialog->addActionListener(&accountListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    break;

                case STATE_UNREGISTER:
                    logger->log1("State: UNREGISTER");
                    mCurrentDialog = new UnRegisterDialog(&loginData);
                    break;

                case STATE_UNREGISTER_ATTEMPT:
                    logger->log1("State: UNREGISTER ATTEMPT");
                    Net::getLoginHandler()->unregisterAccount(
                            loginData.username, loginData.password);
                    break;

                case STATE_UNREGISTER_SUCCESS:
                    logger->log1("State: UNREGISTER SUCCESS");
                    Net::getLoginHandler()->disconnect();

                    mCurrentDialog = new OkDialog(_("Unregister Successful"),
                        _("Farewell, come back any time..."), DIALOG_ERROR);
                    loginData.clear();
                    //The errorlistener sets the state to STATE_CHOOSE_SERVER
                    mCurrentDialog->addActionListener(&errorListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    break;

                case STATE_SWITCH_SERVER:
                    logger->log1("State: SWITCH SERVER");

                    Net::getLoginHandler()->disconnect();
                    Net::getGameHandler()->disconnect();

                    mState = STATE_CHOOSE_SERVER;
                    break;

                case STATE_SWITCH_LOGIN:
                    logger->log1("State: SWITCH LOGIN");

                    Net::getLoginHandler()->logout();

                    mState = STATE_LOGIN;
                    break;

                case STATE_SWITCH_CHARACTER:
                    logger->log1("State: SWITCH CHARACTER");

                    // Done with game
                    Net::getGameHandler()->disconnect();

                    mState = STATE_GET_CHARACTERS;
                    break;

                case STATE_LOGOUT_ATTEMPT:
                    logger->log1("State: LOGOUT ATTEMPT");
                    // TODO
                    break;

                case STATE_WAIT:
                    logger->log1("State: WAIT");
                    break;

                case STATE_EXIT:
                    logger->log1("State: EXIT");
                    Net::unload();
                    break;

                case STATE_FORCE_QUIT:
                    logger->log1("State: FORCE QUIT");
                    if (Net::getGeneralHandler())
                        Net::getGeneralHandler()->unload();
                    mState = STATE_EXIT;
                  break;

                case STATE_ERROR:
                    logger->log1("State: ERROR");
                    logger->log("Error: %s\n", errorMessage.c_str());
                    mCurrentDialog = new OkDialog(_("Error"),
                        errorMessage, DIALOG_ERROR);
                    mCurrentDialog->addActionListener(&errorListener);
                    mCurrentDialog = nullptr; // OkDialog deletes itself
                    Net::getGameHandler()->disconnect();
                    break;

                case STATE_AUTORECONNECT_SERVER:
                    //++++++
                    break;

                case STATE_START:
                default:
                    mState = STATE_FORCE_QUIT;
                    break;
            }
        }
    }

    return 0;
}

void Client::optionChanged(const std::string &name)
{
    if (name == "fpslimit")
    {
        const int fpsLimit = config.getIntValue("fpslimit");
        mLimitFps = fpsLimit > 0;
        setFramerate(fpsLimit);
    }
    else if (name == "guialpha")
    {
        setGuiAlpha(config.getFloatValue("guialpha"));
        ImageHelper::setEnableAlpha(config.getFloatValue("guialpha") != 1.0f);
    }
    else if (name == "gamma")
    {
        applyGamma();
    }
    else if (name == "particleEmitterSkip")
    {
        Particle::emitterSkip = config.getIntValue("particleEmitterSkip") + 1;
    }
    else if (name == "vsync")
    {
        applyVSync();
    }
    else if (name == "repeateInterval" or name == "repeateDelay")
    {
        applyKeyRepeat();
    }
}

void Client::action(const gcn::ActionEvent &event)
{
    std::string tab;

    if (event.getId() == "Setup")
        tab = "";
    else if (event.getId() == "Video")
        tab = "Video";
    else if (event.getId() == "Themes")
        tab = "Theme";
    else if (event.getId() == "Perfomance")
        tab = "Perfomance";
    else
        return;

    if (setupWindow)
    {
        setupWindow->setVisible(!setupWindow->isVisible());
        if (setupWindow->isVisible())
        {
            if (!tab.empty())
                setupWindow->activateTab(tab);
            setupWindow->requestMoveToTop();
        }
    }
}

void Client::initRootDir()
{
    mRootDir = PHYSFS_getBaseDir();
    std::string portableName = mRootDir + "portable.xml";
    struct stat statbuf;

    if (!stat(portableName.c_str(), &statbuf) && S_ISREG(statbuf.st_mode))
    {
        std::string dir;
        Configuration portable;
        portable.init(portableName);

        logger->log("Portable file: %s", portableName.c_str());

        if (mOptions.localDataDir.empty())
        {
            dir = portable.getValue("dataDir", "");
            if (!dir.empty())
            {
                mOptions.localDataDir = mRootDir + dir;
                logger->log("Portable data dir: %s",
                    mOptions.localDataDir.c_str());
            }
        }

        if (mOptions.configDir.empty())
        {
            dir = portable.getValue("configDir", "");
            if (!dir.empty())
            {
                mOptions.configDir = mRootDir + dir;
                logger->log("Portable config dir: %s",
                    mOptions.configDir.c_str());
            }
        }

        if (mOptions.screenshotDir.empty())
        {
            dir = portable.getValue("screenshotDir", "");
            if (!dir.empty())
            {
                mOptions.screenshotDir = mRootDir + dir;
                logger->log("Portable screenshot dir: %s",
                    mOptions.screenshotDir.c_str());
            }
        }
    }
}

/**
 * Initializes the home directory. On UNIX and FreeBSD, ~/.mana is used. On
 * Windows and other systems we use the current working directory.
 */
void Client::initHomeDir()
{
    initLocalDataDir();
    initConfigDir();
}

void Client::initLocalDataDir()
{
    mLocalDataDir = mOptions.localDataDir;

    if (mLocalDataDir.empty())
    {
#ifdef __APPLE__
        // Use Application Directory instead of .mana
        mLocalDataDir = std::string(PHYSFS_getUserDir()) +
            "/Library/Application Support/" +
            branding.getValue("appName", "ManaPlus");
#elif defined __HAIKU__
        mLocalDataDir = std::string(PHYSFS_getUserDir()) +
           "/config/data/Mana";
#elif defined WIN32
        mLocalDataDir = getSpecialFolderLocation(CSIDL_LOCAL_APPDATA);
        if (mLocalDataDir.empty())
            mLocalDataDir = std::string(PHYSFS_getUserDir());
        mLocalDataDir += "/Mana";
#else
        mLocalDataDir = std::string(PHYSFS_getUserDir()) +
            ".local/share/mana";
#endif
    }

    if (mkdir_r(mLocalDataDir.c_str()))
    {
        logger->error(strprintf(_("%s doesn't exist and can't be created! "
                                  "Exiting."), mLocalDataDir.c_str()));
    }
}

void Client::initConfigDir()
{
    mConfigDir = mOptions.configDir;

    if (mConfigDir.empty())
    {
#ifdef __APPLE__
        mConfigDir = mLocalDataDir + PHYSFS_getDirSeparator()
            + branding.getValue("appShort", "mana");
#elif defined __HAIKU__
        mConfigDir = std::string(PHYSFS_getUserDir()) +
           "/config/settings/Mana" +
           branding.getValue("appName", "ManaPlus");
#elif defined WIN32
        mConfigDir = getSpecialFolderLocation(CSIDL_APPDATA);
        if (mConfigDir.empty())
            mConfigDir = mLocalDataDir;
        else
            mConfigDir += "/mana/" + branding.getValue("appShort", "mana");
#else
        mConfigDir = std::string(PHYSFS_getUserDir()) +
            "/.config/mana/" + branding.getValue("appShort", "mana");
#endif
        logger->log("Generating config dir: " + mConfigDir);
    }

    if (mkdir_r(mConfigDir.c_str()))
    {
        logger->error(strprintf(_("%s doesn't exist and can't be created! "
                                  "Exiting."), mConfigDir.c_str()));
    }
}

/**
 * Initializes the home directory. On UNIX and FreeBSD, ~/.mana is used. On
 * Windows and other systems we use the current working directory.
 */
void Client::initServerConfig(std::string serverName)
{
    mServerConfigDir = mConfigDir + PHYSFS_getDirSeparator() + serverName;

    if (mkdir_r(mServerConfigDir.c_str()))
    {
        logger->error(strprintf(_("%s doesn't exist and can't be created! "
                                  "Exiting."), mServerConfigDir.c_str()));
    }
    FILE *configFile = nullptr;
    std::string configPath;

    configPath = mServerConfigDir + "/config.xml";
    configFile = fopen(configPath.c_str(), "r");
    if (!configFile)
    {
        configFile = fopen(configPath.c_str(), "wt");
        logger->log("Creating new server config: " + configPath);
    }
    if (configFile)
    {
        fclose(configFile);
        serverConfig.init(configPath);
        serverConfig.setDefaultValues(getConfigDefaults());
        logger->log("serverConfigPath: " + configPath);
    }
    initPacketLimiter();
    initTradeFilter();
    initUsersDir();
    player_relations.init();

    // Initialize the item and emote shortcuts.
    for (int f = 0; f < SHORTCUT_TABS; f ++)
    {
        delete itemShortcut[f];
        itemShortcut[f] = new ItemShortcut(f);
    }
    delete emoteShortcut;
    emoteShortcut = new EmoteShortcut;

    // Initialize the drop shortcuts.
    delete dropShortcut;
    dropShortcut = new DropShortcut;
}

/**
 * Initialize configuration.
 */
void Client::initConfiguration()
{
#ifdef DEBUG_CONFIG
    config.setIsMain(true);
#endif

    // Fill configuration with defaults
    config.setValue("hwaccel", false);
#if (defined __APPLE__) && defined USE_OPENGL
    config.setValue("opengl", 1);
#elif (defined WIN32) && defined USE_OPENGL
    config.setValue("opengl", 2);
#else
    config.setValue("opengl", 0);
#endif
    config.setValue("screen", false);
    config.setValue("sound", true);
    config.setValue("guialpha", 0.8f);
//    config.setValue("remember", true);
    config.setValue("sfxVolume", 100);
    config.setValue("musicVolume", 60);
    config.setValue("fpslimit", 60);
    std::string defaultUpdateHost = branding.getValue("defaultUpdateHost", "");
    if (!checkPath(defaultUpdateHost))
        defaultUpdateHost = "";
    config.setValue("updatehost", defaultUpdateHost);
    config.setValue("customcursor", true);
    config.setValue("useScreenshotDirectorySuffix", true);
    config.setValue("ChatLogLength", 128);

    // Checking if the configuration file exists... otherwise create it with
    // default options.
    FILE *configFile = nullptr;
    std::string configPath;
//    bool oldConfig = false;
//    int emptySize = config.getSize();

    if (mOptions.test.empty())
        configPath = mConfigDir + "/config.xml";
    else
        configPath = mConfigDir + "/test.xml";

    configFile = fopen(configPath.c_str(), "r");

    // If we can't read it, it doesn't exist !
    if (!configFile)
    {
        // We reopen the file in write mode and we create it
        configFile = fopen(configPath.c_str(), "wt");
        logger->log1("Creating new config");
//        oldConfig = false;
    }
    if (!configFile)
    {
        logger->log("Can't create %s. Using defaults.", configPath.c_str());
    }
    else
    {
        fclose(configFile);
        config.init(configPath);
        logger->log("init 3");
        config.setDefaultValues(getConfigDefaults());
        logger->log("configPath: " + configPath);
    }
}

/**
 * Parse the update host and determine the updates directory
 * Then verify that the directory exists (creating if needed).
 */
void Client::initUpdatesDir()
{
    std::stringstream updates;

    // If updatesHost is currently empty, fill it from config file
    if (mUpdateHost.empty())
        mUpdateHost = config.getStringValue("updatehost");
    if (!checkPath(mUpdateHost))
        return;

    // Don't go out of range int he next check
    if (mUpdateHost.length() < 2)
        return;

    // Remove any trailing slash at the end of the update host
    if (!mUpdateHost.empty() && mUpdateHost.at(mUpdateHost.size() - 1) == '/')
        mUpdateHost.resize(mUpdateHost.size() - 1);

    // Parse out any "http://" or "ftp://", and set the updates directory
    size_t pos;
    pos = mUpdateHost.find("://");
    if (pos != mUpdateHost.npos)
    {
        if (pos + 3 < mUpdateHost.length() && !mUpdateHost.empty())
        {
            updates << "updates/" << mUpdateHost.substr(pos + 3);
            mUpdatesDir = updates.str();
        }
        else
        {
            logger->log("Error: Invalid update host: %s", mUpdateHost.c_str());
            errorMessage = strprintf(_("Invalid update host: %s"),
                                     mUpdateHost.c_str());
            mState = STATE_ERROR;
        }
    }
    else
    {
        logger->log1("Warning: no protocol was specified for the update host");
        updates << "updates/" << mUpdateHost;
        mUpdatesDir = updates.str();
    }

    ResourceManager *resman = ResourceManager::getInstance();

    // Verify that the updates directory exists. Create if necessary.
    if (!resman->isDirectory("/" + mUpdatesDir))
    {
        if (!resman->mkdir("/" + mUpdatesDir))
        {
#if defined WIN32
            std::string newDir = mLocalDataDir + "\\" + mUpdatesDir;
            size_t loc = newDir.find("/", 0);

            while (loc != std::string::npos)
            {
                newDir.replace(loc, 1, "\\");
                loc = newDir.find("/", loc);
            }

            if (!CreateDirectory(newDir.c_str(), nullptr) &&
                GetLastError() != ERROR_ALREADY_EXISTS)
            {
                logger->log("Error: %s can't be made, but doesn't exist!",
                            newDir.c_str());
                errorMessage = _("Error creating updates directory!");
                mState = STATE_ERROR;
            }
#else
            logger->log("Error: %s/%s can't be made, but doesn't exist!",
                        mLocalDataDir.c_str(), mUpdatesDir.c_str());
            errorMessage = _("Error creating updates directory!");
            mState = STATE_ERROR;
#endif
        }
    }
    std::string updateLocal = "/" + mUpdatesDir + "/local";
    std::string updateFix = "/" + mUpdatesDir + "/fix";
    if (!resman->isDirectory(updateLocal))
        resman->mkdir(updateLocal);
    if (!resman->isDirectory(updateFix))
        resman->mkdir(updateFix);
}

void Client::initScreenshotDir()
{
    if (!mOptions.screenshotDir.empty())
    {
        mScreenshotDir = mOptions.screenshotDir;
        if (mkdir_r(mScreenshotDir.c_str()))
        {
            logger->log(strprintf(
                _("Error: %s doesn't exist and can't be created! "
                "Exiting."), mScreenshotDir.c_str()));
        }
    }
    else if (mScreenshotDir.empty())
    {
        std::string configScreenshotDir =
            config.getStringValue("screenshotDirectory");
        if (!configScreenshotDir.empty())
            mScreenshotDir = configScreenshotDir;
        else
            mScreenshotDir = getDesktopDir();
        //config.setValue("screenshotDirectory", mScreenshotDir);
        logger->log("screenshotDirectory: " + mScreenshotDir);

        if (config.getBoolValue("useScreenshotDirectorySuffix"))
        {
            std::string configScreenshotSuffix =
                branding.getValue("appShort", "mana");

            if (!configScreenshotSuffix.empty())
            {
                mScreenshotDir += PHYSFS_getDirSeparator()
                    + configScreenshotSuffix;
            }
        }
    }
}

void Client::accountLogin(LoginData *data)
{
    if (!data)
        return;

    logger->log("Username is %s", data->username.c_str());

    // Send login infos
    if (data->registerLogin)
        Net::getLoginHandler()->registerAccount(data);
    else
        Net::getLoginHandler()->loginAccount(data);

    // Clear the password, avoids auto login when returning to login
    data->password = "";

    // TODO This is not the best place to save the config, but at least better
    // than the login gui window
    if (data->remember)
        serverConfig.setValue("username", data->username);
    serverConfig.setValue("remember", data->remember);
}

bool Client::copyFile(std::string &configPath, std::string &oldConfigPath)
{
    FILE *configFile = nullptr;

    configFile = fopen(oldConfigPath.c_str(), "r");

    if (configFile)
    {
        fclose(configFile);

        std::ifstream ifs(oldConfigPath.c_str(), std::ios::binary);
        std::ofstream ofs(configPath.c_str(), std::ios::binary);
        ofs << ifs.rdbuf();
        ifs.close();
        ofs.close();
        return true;
    }
    return false;
}

bool Client::createConfig(std::string &configPath)
{
    std::string oldHomeDir;
#ifdef __APPLE__
    // Use Application Directory instead of .mana
    oldHomeDir = std::string(PHYSFS_getUserDir()) +
        "/Library/Application Support/" +
        branding.getValue("appName", "ManaPlus");
#else
    oldHomeDir = std::string(PHYSFS_getUserDir()) +
        "/." + branding.getValue("appShort", "mana");
#endif

    oldHomeDir += "/config.xml";

    logger->log("Restore config from: " + configPath);
    return copyFile(configPath, oldHomeDir);
}

void Client::storeSafeParameters()
{
    bool tmpHwaccel;
    int tmpOpengl;
    int tmpFpslimit;
    int tmpAltFpslimit;
    bool tmpSound;
    int width;
    int height;
    std::string font;
    std::string bFont;
    std::string particleFont;
    std::string helpFont;
    std::string secureFont;
    std::string japanFont;
    bool showBackground;
    bool enableMumble;
    bool enableMapReduce;

    isSafeMode = config.getBoolValue("safemode");
    if (isSafeMode)
        logger->log1("Run in safe mode");

#if defined USE_OPENGL
    tmpOpengl = config.getIntValue("opengl");
#else
    tmpOpengl = 0;
#endif

    width = config.getIntValue("screenwidth");
    height = config.getIntValue("screenheight");
    tmpHwaccel = config.getBoolValue("hwaccel");

    tmpFpslimit = config.getIntValue("fpslimit");
    tmpAltFpslimit = config.getIntValue("altfpslimit");
    tmpSound = config.getBoolValue("sound");

    font = config.getStringValue("font");
    bFont = config.getStringValue("boldFont");
    particleFont = config.getStringValue("particleFont");
    helpFont = config.getStringValue("helpFont");
    secureFont = config.getStringValue("secureFont");
    japanFont = config.getStringValue("japanFont");

    showBackground = config.getBoolValue("showBackground");
    enableMumble = config.getBoolValue("enableMumble");
    enableMapReduce = config.getBoolValue("enableMapReduce");

    if (!mOptions.safeMode && !tmpOpengl)
    {
        // if video mode configured reset most settings to safe
        config.setValue("hwaccel", false);
        config.setValue("opengl", 0);
        config.setValue("altfpslimit", 3);
        config.setValue("sound", false);
        config.setValue("safemode", true);
        config.setValue("screenwidth", 640);
        config.setValue("screenheight", 480);
        config.setValue("font", "fonts/dejavusans.ttf");
        config.setValue("boldFont", "fonts/dejavusans-bold.ttf");
        config.setValue("particleFont", "fonts/dejavusans.ttf");
        config.setValue("helpFont", "fonts/dejavusansmono.ttf");
        config.setValue("secureFont", "fonts/dejavusansmono.ttf");
        config.setValue("japanFont", "fonts/mplus-1p-regular.ttf");
        config.setValue("showBackground", false);
        config.setValue("enableMumble", false);
        config.setValue("enableMapReduce", false);
    }
    else
    {
        // if video mode not configured reset only video mode to safe
        config.setValue("opengl", 0);
        config.setValue("screenwidth", 640);
        config.setValue("screenheight", 480);
    }

    config.write();

    if (mOptions.safeMode)
    {
        isSafeMode = true;
        return;
    }

    if (!tmpOpengl)
    {
        config.setValue("hwaccel", tmpHwaccel);
        config.setValue("opengl", tmpOpengl);
        config.setValue("fpslimit", tmpFpslimit);
        config.setValue("altfpslimit", tmpAltFpslimit);
        config.setValue("sound", tmpSound);
        config.setValue("safemode", false);
        config.setValue("screenwidth", width);
        config.setValue("screenheight", height);
        config.setValue("font", font);
        config.setValue("boldFont", bFont);
        config.setValue("particleFont", particleFont);
        config.setValue("helpFont", helpFont);
        config.setValue("secureFont", secureFont);
        config.setValue("japanFont", japanFont);
        config.setValue("showBackground", showBackground);
        config.setValue("enableMumble", enableMumble);
        config.setValue("enableMapReduce", enableMapReduce);
    }
    else
    {
        config.setValue("opengl", tmpOpengl);
        config.setValue("screenwidth", width);
        config.setValue("screenheight", height);
    }
}

void Client::initTradeFilter()
{
    std::string tradeListName =
        Client::getServerConfigDirectory() + "/tradefilter.txt";

    std::ofstream tradeFile;
    struct stat statbuf;

    if (stat(tradeListName.c_str(), &statbuf) || !S_ISREG(statbuf.st_mode))
    {
        tradeFile.open(tradeListName.c_str(), std::ios::out);
        if (tradeFile.is_open())
        {
            tradeFile << ": sell" << std::endl;
            tradeFile << ": buy" << std::endl;
            tradeFile << ": trade" << std::endl;
            tradeFile << "i sell" << std::endl;
            tradeFile << "i buy" << std::endl;
            tradeFile << "i trade" << std::endl;
            tradeFile << "i trading" << std::endl;
            tradeFile << "i am buy" << std::endl;
            tradeFile << "i am sell" << std::endl;
            tradeFile << "i am trade" << std::endl;
            tradeFile << "i am trading" << std::endl;
            tradeFile << "i'm buy" << std::endl;
            tradeFile << "i'm sell" << std::endl;
            tradeFile << "i'm trade" << std::endl;
            tradeFile << "i'm trading" << std::endl;
        }
        tradeFile.close();
    }
}

void Client::initUsersDir()
{
    mUsersDir = Client::getServerConfigDirectory() + "/users/";
    if (mkdir_r(mUsersDir.c_str()))
    {
        logger->error(strprintf(_("%s doesn't exist and can't be created! "
            "Exiting."), mUsersDir.c_str()));
    }

    mNpcsDir = Client::getServerConfigDirectory() + "/npcs/";
    if (mkdir_r(mNpcsDir.c_str()))
    {
        logger->error(strprintf(_("%s doesn't exist and can't be created! "
            "Exiting."), mNpcsDir.c_str()));
    }
}

void Client::initPacketLimiter()
{
    //here i setting packet limits. but current server is broken,
    // and this limits may not help.

    mPacketLimits[PACKET_CHAT].timeLimit = 10 + 5;
    mPacketLimits[PACKET_CHAT].lastTime = 0;
    mPacketLimits[PACKET_CHAT].cntLimit = 1;
    mPacketLimits[PACKET_CHAT].cnt = 0;

    //10
    mPacketLimits[PACKET_PICKUP].timeLimit = 10 + 5;
    mPacketLimits[PACKET_PICKUP].lastTime = 0;
    mPacketLimits[PACKET_PICKUP].cntLimit = 1;
    mPacketLimits[PACKET_PICKUP].cnt = 0;

    //10 5
    mPacketLimits[PACKET_DROP].timeLimit = 5;
    mPacketLimits[PACKET_DROP].lastTime = 0;
    mPacketLimits[PACKET_DROP].cntLimit = 1;
    mPacketLimits[PACKET_DROP].cnt = 0;

    //100
    mPacketLimits[PACKET_NPC_NEXT].timeLimit = 0;
    mPacketLimits[PACKET_NPC_NEXT].lastTime = 0;
    mPacketLimits[PACKET_NPC_NEXT].cntLimit = 1;
    mPacketLimits[PACKET_NPC_NEXT].cnt = 0;

    mPacketLimits[PACKET_NPC_INPUT].timeLimit = 100;
    mPacketLimits[PACKET_NPC_INPUT].lastTime = 0;
    mPacketLimits[PACKET_NPC_INPUT].cntLimit = 1;
    mPacketLimits[PACKET_NPC_INPUT].cnt = 0;

    //50
    mPacketLimits[PACKET_NPC_TALK].timeLimit = 60;
    mPacketLimits[PACKET_NPC_TALK].lastTime = 0;
    mPacketLimits[PACKET_NPC_TALK].cntLimit = 1;
    mPacketLimits[PACKET_NPC_TALK].cnt = 0;

    //10
    mPacketLimits[PACKET_EMOTE].timeLimit = 10 + 5;
    mPacketLimits[PACKET_EMOTE].lastTime = 0;
    mPacketLimits[PACKET_EMOTE].cntLimit = 1;
    mPacketLimits[PACKET_EMOTE].cnt = 0;

    //100
    mPacketLimits[PACKET_SIT].timeLimit = 100;
    mPacketLimits[PACKET_SIT].lastTime = 0;
    mPacketLimits[PACKET_SIT].cntLimit = 1;
    mPacketLimits[PACKET_SIT].cnt = 0;

    mPacketLimits[PACKET_DIRECTION].timeLimit = 50;
    mPacketLimits[PACKET_DIRECTION].lastTime = 0;
    mPacketLimits[PACKET_DIRECTION].cntLimit = 1;
    mPacketLimits[PACKET_DIRECTION].cnt = 0;

    //2+
    mPacketLimits[PACKET_ATTACK].timeLimit = 2 + 10;
    mPacketLimits[PACKET_ATTACK].lastTime = 0;
    mPacketLimits[PACKET_ATTACK].cntLimit = 1;
    mPacketLimits[PACKET_ATTACK].cnt = 0;

    mPacketLimits[PACKET_STOPATTACK].timeLimit = 2 + 10;
    mPacketLimits[PACKET_STOPATTACK].lastTime = 0;
    mPacketLimits[PACKET_STOPATTACK].cntLimit = 1;
    mPacketLimits[PACKET_STOPATTACK].cnt = 0;

    mPacketLimits[PACKET_ONLINELIST].timeLimit = 1800;
    mPacketLimits[PACKET_ONLINELIST].lastTime = 0;
    mPacketLimits[PACKET_ONLINELIST].cntLimit = 1;
    mPacketLimits[PACKET_ONLINELIST].cnt = 0;

    if (!mServerConfigDir.empty())
    {
        std::string packetLimitsName =
            Client::getServerConfigDirectory() + "/packetlimiter.txt";

        std::ifstream inPacketFile;
        struct stat statbuf;

        if (stat(packetLimitsName.c_str(), &statbuf)
            || !S_ISREG(statbuf.st_mode))
        {
            // wtiting new file
            writePacketLimits(packetLimitsName);
        }
        else
        {   // reading existent file
            inPacketFile.open(packetLimitsName.c_str(), std::ios::in);
            char line[101];

            if (!inPacketFile.is_open() || !inPacketFile.getline(line, 100))
            {
                inPacketFile.close();
                return;
            }

            int ver = atoi(line);

            for (int f = 0; f < PACKET_SIZE; f ++)
            {
                if (!inPacketFile.getline(line, 100))
                    break;

                if (!(ver == 1 && (f == PACKET_DROP || f == PACKET_NPC_NEXT)))
                    mPacketLimits[f].timeLimit = atoi(line);
            }
            inPacketFile.close();
            if (ver < 4)
                writePacketLimits(packetLimitsName);
        }
    }
}

void Client::writePacketLimits(std::string packetLimitsName)
{
    std::ofstream outPacketFile;
    outPacketFile.open(packetLimitsName.c_str(), std::ios::out);
    if (!outPacketFile.is_open())
    {
        outPacketFile.close();
        return;
    }
    outPacketFile << "4" << std::endl;
    for (int f = 0; f < PACKET_SIZE; f ++)
    {
        outPacketFile << toString(mPacketLimits[f].timeLimit)
                      << std::endl;
    }

    outPacketFile.close();
}

bool Client::checkPackets(int type)
{
    if (type > PACKET_SIZE)
        return false;

    if (!serverConfig.getValueBool("enableBuggyServers", true))
        return true;

    PacketLimit &limit = instance()->mPacketLimits[type];
    int timeLimit = limit.timeLimit;

    if (!timeLimit)
        return true;

    int time = tick_time;
    int lastTime = limit.lastTime;
    int cnt = limit.cnt;
    int cntLimit = limit.cntLimit;

    if (lastTime > tick_time)
    {
//        instance()->mPacketLimits[type].lastTime = time;
//        instance()->mPacketLimits[type].cnt = 0;

        return true;
    }
    else if (lastTime + timeLimit > time)
    {
        if (cnt >= cntLimit)
        {
            return false;
        }
        else
        {
//            instance()->mPacketLimits[type].cnt ++;
            return true;
        }
    }
//    instance()->mPacketLimits[type].lastTime = time;
//    instance()->mPacketLimits[type].cnt = 1;
    return true;
}

bool Client::limitPackets(int type)
{
    if (type > PACKET_SIZE)
        return false;

    if (!serverConfig.getValueBool("enableBuggyServers", true))
        return true;

    int timeLimit = instance()->mPacketLimits[type].timeLimit;

    if (!timeLimit)
        return true;

    int time = tick_time;
    int lastTime = instance()->mPacketLimits[type].lastTime;
    int cnt = instance()->mPacketLimits[type].cnt;
    int cntLimit = instance()->mPacketLimits[type].cntLimit;

    if (lastTime > tick_time)
    {
        instance()->mPacketLimits[type].lastTime = time;
        instance()->mPacketLimits[type].cnt = 0;

        return true;
    }
    else if (lastTime + timeLimit > time)
    {
        if (cnt >= cntLimit)
        {
            return false;
        }
        else
        {
            instance()->mPacketLimits[type].cnt ++;
            return true;
        }
    }
    instance()->mPacketLimits[type].lastTime = time;
    instance()->mPacketLimits[type].cnt = 1;
    return true;
}

const std::string Client::getServerConfigDirectory()
{
    return instance()->mServerConfigDir;
}

const std::string Client::getUsersDirectory()
{
    return instance()->mUsersDir;
}

const std::string Client::getNpcsDirectory()
{
    return instance()->mNpcsDir;
}

void Client::setGuiAlpha(float n)
{
    instance()->mGuiAlpha = n;
}

float Client::getGuiAlpha()
{
    return instance()->mGuiAlpha;
}

void Client::setFramerate(int fpsLimit)
{
    if (!fpsLimit || !instance()->mLimitFps)
        return;

    SDL_setFramerate(&instance()->mFpsManager, fpsLimit);
}

int Client::getFramerate()
{
    if (!instance()->mLimitFps)
        return 0;

    return SDL_getFramerate(&instance()->mFpsManager);
}

void Client::closeDialogs()
{
    Net::getNpcHandler()->clearDialogs();
    BuyDialog::closeAll();
    BuySellDialog::closeAll();
    NpcDialog::closeAll();
    SellDialog::closeAll();
}

bool Client::isTmw()
{
    if (getServerName() == "server.themanaworld.org"
        || getServerName() == "themanaworld.org"
        || getServerName() == "81.161.192.4")
    {
        return true;
    }
    return false;
}

void Client::resizeVideo(int width, int height, bool always)
{
    // Keep a minimum size. This isn't adhered to by the actual window, but
    // it keeps some window positions from getting messed up.
    width = std::max(640, width);
    height = std::max(480, height);

    if (!mainGraphics)
        return;
    if (!always && mainGraphics->mWidth == width
        && mainGraphics->mHeight == height)
    {
        return;
    }

    if (mainGraphics->resizeScreen(width, height))
    {
        if (gui)
            gui->videoResized();

        if (mDesktop)
            mDesktop->setSize(width, height);

        if (mSetupButton)
        {
//            mSetupButton->setPosition(width - mSetupButton->getWidth() - 3, 3);

            int x = width - mSetupButton->getWidth() - 3;
            mSetupButton->setPosition(x, 3);

#ifndef WIN32
            x -= mPerfomanceButton->getWidth() + 6;
            mPerfomanceButton->setPosition(x, 3);

            x -= mVideoButton->getWidth() + 6;
            mVideoButton->setPosition(x, 3);

            x -= mThemesButton->getWidth() + 6;
            mThemesButton->setPosition(x, 3);
#endif
        }

        if (mGame)
            mGame->videoResized(width, height);

        if (gui)
            gui->draw();

        // Since everything appears to have worked out, remember to store the
        // new size in the configuration.
        config.setValue("screenwidth", width);
        config.setValue("screenheight", height);
    }
}

void Client::applyGrabMode()
{
    SDL_WM_GrabInput(config.getBoolValue("grabinput")
        ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

void Client::applyGamma()
{
    float val = config.getFloatValue("gamma");
    SDL_SetGamma(val, val, val);
}

void Client::applyVSync()
{
    int val = config.getIntValue("vsync");
    if (val > 0 && val < 2)
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, val);
}

void Client::applyKeyRepeat()
{
    SDL_EnableKeyRepeat(config.getIntValue("repeateDelay"),
        config.getIntValue("repeateInterval"));
}
