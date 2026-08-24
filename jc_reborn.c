/*
 *  This file is part of 'Johnny Reborn'
 *
 *  An open-source engine for the classic
 *  'Johnny Castaway' screensaver by Sierra.
 *
 *  Copyright (C) 2019 Jeremie GUILLAUME
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mytypes.h"
#include "utils.h"
#include "resource.h"
#include "dump.h"
#include "graphics.h"
#include "events.h"
#include "sound.h"
#include "ttm.h"
#include "ads.h"
#include "story.h"
#include "config.h"
#include "scrconfig.h"


static int  argDump      = 0;
static int  argBench     = 0;
static int  argTtm       = 0;
static int  argAds       = 0;
static int  argPlayAll   = 0;
static int  argIsland    = 0;
static int  argMonitors  = 0;
static int  argAudioDevices = 0;

static int  argSpeed       = -1;
static int  argDay         = -1;
static int  argHoliday     = -1;
static int  argNight       = -1;
static int  argDateOffset  = 0;
static int  argDateOffsetSet = 0;
static int  argScale       = -100; // -100 = not set, -2 = fit, -1 = fill/stretch, 0 = auto, N = explicit
static int  argMonitor     = -1;
static int  argMonitorMode = -1;
static int  argAudioDevice = -2;   // -2 = not set, -1 = explicit default device
static int  argCrtFilter   = -1;   // -1 = not set
static int  argSkipIntro   = -1;   // -1 = not set
static int  argVolume      = -1;   // -1 = not set
static int  argExitClick   = -1;   // -1 = not set
static int  argExitMove    = -1;   // -1 = not set

// Windows screensaver (.scr) command-line convention:
//   /s          -> run fullscreen
//   /p <hwnd>   -> render embedded into the given preview window
//   /c[:<hwnd>] -> show the configuration dialog
#define SCR_MODE_NONE     0
#define SCR_MODE_RUN      1
#define SCR_MODE_PREVIEW  2
#define SCR_MODE_CONFIG   3

static int      argScrMode = SCR_MODE_NONE;
static long long argScrHwnd = 0;

static char *args[3];
static int  numArgs  = 0;


#ifdef _WIN32
static int isScrFlag(const char *arg, char letter)
{
    if (arg[0] != '/' && arg[0] != '-')
        return 0;

    char c = arg[1];
    return (c == letter || c == (letter - 32));
}
#endif


static void usage()
{
        printf("\n");
        printf(" Usage :\n");
        printf("         jc_reborn\n");
        printf("         jc_reborn help\n");
        printf("         jc_reborn version\n");
        printf("         jc_reborn dump\n");
        printf("         jc_reborn monitors\n");
        printf("         jc_reborn audiodevices\n");
        printf("         jc_reborn [<options>] bench\n");
        printf("         jc_reborn [<options>] ttm <TTM name>\n");
        printf("         jc_reborn [<options>] ads <ADS name> <ADS tag no>\n");
        printf("\n");
        printf(" Windows .scr screensaver convention:\n");
        printf("         jc_reborn /s          - run fullscreen\n");
        printf("         jc_reborn /p <hwnd>   - render into the given preview window\n");
        printf("         jc_reborn /c[:<hwnd>] - show the configuration dialog\n");
        printf("\n");
        printf(" Available options are:\n");
        printf("         window        - play in windowed mode\n");
        printf("         nosound       - quiet mode\n");
        printf("         island        - display the island as background for ADS play\n");
        printf("         debug         - print some debug info on stdout\n");
        printf("         hotkeys       - enable hot keys\n");
        printf("         speed=<n>     - %% of normal speed (100=normal, 200=double)\n");
        printf("         day=<1..11>   - pin the story to a specific day\n");
        printf("         holiday=<n>   - force a holiday (0=auto,1=Halloween,2=St Patrick,3=Christmas,4=New Year)\n");
        printf("         night=<n>     - force day/night (0=auto,1=night,2=day)\n");
        printf("         dateoffset=<n>- add <n> days to the system date (virtual date)\n");
        printf("         scale=<n>     - 0=auto (integer), N=explicit integer, fit=fractional (keeps aspect, bars),\n");
        printf("                         fill=stretch exactly (distorts), cover=fractional (keeps aspect, crops, no bars)\n");
        printf("         monitor=<n>   - display index to use (see: jc_reborn monitors)\n");
        printf("         monitormode=<n> - 0=single monitor, 1=clone (every monitor), 2=extend (span all)\n");
        printf("         audiodevice=<n> - playback device index (see: jc_reborn audiodevices)\n");
        printf("         crt=<0..5>    - old-monitor filter: 0=off,1=CRT scanlines,2=green mono,3=amber mono,4=strong CRT,5=faded\n");
        printf("         nointro       - skip the intro screen and go straight to the story\n");
        printf("         volume=<0..100> - sound effects volume\n");
        printf("         exitclick=<0|1> - exit on mouse click (fullscreen mode; 1=default)\n");
        printf("         exitmove=<0|1>  - exit on mouse movement (fullscreen mode; 1=default)\n");
        printf("\n");
        printf(" While-playing hot-keys (if enabled):\n");
        printf("         Esc        - Terminate immediately\n");
        printf("         Alt+Return - Toggle full screen / windowed mode\n");
        printf("         Space      - Toggle pause / unpause\n");
        printf("         Return     - When paused, advance one frame\n");
        printf("         <M>        - toggle max / normal speed\n");
        printf("\n");
        exit(1);
}


static void version()
{
        printf("\n");
        printf("    Johnny Reborn, an open-source engine for\n");
        printf("    the classic Johnny Castaway screensaver by Sierra.\n");
        printf("    Development version Copyright (C) 2019 Jeremie GUILLAUME\n");
        printf("\n");
        exit(1);
}


static void parseArgs(int argc, char **argv)
{
    int numExpectedArgs = 0;

    for (int i=1; i < argc; i++) {

        if (numExpectedArgs) {
            args[numArgs++] = argv[i];
            numExpectedArgs--;
        }
        else {
            if (!strcmp(argv[i], "help")) {
                usage();
            }
            if (!strcmp(argv[i], "version")) {
                version();
            }
            else if (!strcmp(argv[i], "dump")) {
                argDump = 1;
            }
            else if (!strcmp(argv[i], "monitors")) {
                argMonitors = 1;
            }
            else if (!strcmp(argv[i], "audiodevices")) {
                argAudioDevices = 1;
            }
            else if (!strcmp(argv[i], "bench")) {
                argBench = 1;
            }
            else if (!strcmp(argv[i], "ttm")) {
                argTtm = 1;
                numExpectedArgs = 1;
            }
            else if (!strcmp(argv[i], "ads")) {
                argAds = 1;
                numExpectedArgs = 2;
            }
            else if (!strcmp(argv[i], "window")) {
                grWindowed = 1;
            }
            else if (!strcmp(argv[i], "nosound")) {
                soundDisabled = 1;
            }
            else if (!strcmp(argv[i], "island")) {
                argIsland = 1;
            }
            else if (!strcmp(argv[i], "debug")) {
                debugMode = 1;
            }
            else if (!strcmp(argv[i], "hotkeys")) {
                evHotKeysEnabled = 1;
            }
            else if (!strncmp(argv[i], "speed=", 6)) {
                argSpeed = atoi(argv[i] + 6);
            }
            else if (!strncmp(argv[i], "day=", 4)) {
                argDay = atoi(argv[i] + 4);
            }
            else if (!strncmp(argv[i], "holiday=", 8)) {
                argHoliday = atoi(argv[i] + 8);
            }
            else if (!strncmp(argv[i], "night=", 6)) {
                argNight = atoi(argv[i] + 6);
            }
            else if (!strncmp(argv[i], "dateoffset=", 11)) {
                argDateOffset = atoi(argv[i] + 11);
                argDateOffsetSet = 1;
            }
            else if (!strncmp(argv[i], "scale=", 6)) {
                if (!strcmp(argv[i] + 6, "fill"))
                    argScale = -1;
                else if (!strcmp(argv[i] + 6, "fit"))
                    argScale = -2;
                else if (!strcmp(argv[i] + 6, "cover"))
                    argScale = -3;
                else if (!strcmp(argv[i] + 6, "auto"))
                    argScale = 0;
                else
                    argScale = atoi(argv[i] + 6);
            }
            else if (!strcmp(argv[i], "crt")) {
                argCrtFilter = 1;
            }
            else if (!strncmp(argv[i], "crt=", 4)) {
                argCrtFilter = atoi(argv[i] + 4);
            }
            else if (!strcmp(argv[i], "nointro")) {
                argSkipIntro = 1;
            }
            else if (!strncmp(argv[i], "skipintro=", 10)) {
                argSkipIntro = atoi(argv[i] + 10);
            }
            else if (!strncmp(argv[i], "monitormode=", 12)) {
                argMonitorMode = atoi(argv[i] + 12);
            }
            else if (!strncmp(argv[i], "monitor=", 8)) {
                argMonitor = atoi(argv[i] + 8);
            }
            else if (!strncmp(argv[i], "audiodevice=", 12)) {
                argAudioDevice = atoi(argv[i] + 12);
            }
            else if (!strncmp(argv[i], "volume=", 7)) {
                argVolume = atoi(argv[i] + 7);
            }
            else if (!strncmp(argv[i], "exitclick=", 10)) {
                argExitClick = atoi(argv[i] + 10);
            }
            else if (!strncmp(argv[i], "exitmove=", 9)) {
                argExitMove = atoi(argv[i] + 9);
            }
#ifdef _WIN32
            else if (isScrFlag(argv[i], 's') && argv[i][2] == '\0') {
                argScrMode = SCR_MODE_RUN;
            }
            else if (isScrFlag(argv[i], 'p') && argv[i][2] == '\0') {
                argScrMode = SCR_MODE_PREVIEW;
                numExpectedArgs = 1;   // next token: hwnd
            }
            else if (isScrFlag(argv[i], 'p') && argv[i][2] == ':') {
                argScrMode = SCR_MODE_PREVIEW;
                argScrHwnd = atoll(argv[i] + 3);
            }
            else if (isScrFlag(argv[i], 'c') && argv[i][2] == '\0') {
                argScrMode = SCR_MODE_CONFIG;
            }
            else if (isScrFlag(argv[i], 'c') && argv[i][2] == ':') {
                argScrMode = SCR_MODE_CONFIG;
                argScrHwnd = atoll(argv[i] + 3);
            }
#endif
        }
    }

    if (argScrMode == SCR_MODE_PREVIEW && numArgs >= 1)
        argScrHwnd = atoll(args[0]);

    if (numExpectedArgs)
        usage();

    if (argDump + argBench + argTtm + argAds > 1)
        usage();

    if (argDump + argBench + argTtm + argAds == 0)
        argPlayAll = 1;
}


#ifdef _WIN32
// Whatever launched us (a shortcut with no "Start in" set, the installer's
// finish-page "Run now" checkbox, Wallpaper Engine, a double-click from
// Explorer) may leave the working directory pointing anywhere. Every
// relative path in this codebase (RESOURCE.MAP, sound*.wav, ...) is only
// meaningful relative to the executable's own folder, so pin it here,
// once, before anything else runs.
static void ChdirToExeDir()
{
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);

    if (len > 0 && len < MAX_PATH) {
        char *lastSlash = strrchr(path, '\\');
        if (lastSlash != NULL) {
            *lastSlash = '\0';
            SetCurrentDirectoryA(path);
        }
    }
}
#endif


int main(int argc, char **argv)
{
#ifdef _WIN32
    // Without this, Windows treats the process as "DPI unaware" and
    // fabricates virtualized screen coordinates/metrics for it whenever
    // any monitor isn't at 100% scaling -- GetSystemMetrics(SM_*VIRTUALSCREEN)
    // and SDL's display bounds then no longer match real screen pixels.
    // On a mixed-DPI multi-monitor setup this can place fullscreen/extend
    // windows at the wrong position or size entirely. Must happen before
    // any window/display call, so it's the very first thing in main().
    SetProcessDPIAware();

    ChdirToExeDir();
#endif

    parseArgs(argc, argv);

    if (argDump)
        debugMode = 1;

    if (argMonitors) {
        grPrintMonitors();
        return 0;
    }

    if (argAudioDevices) {
        soundPrintDevices();
        return 0;
    }

    cfgFileRead(&gConfig);

    if (argSpeed > 0)
        gConfig.speed = argSpeed;

    if (argHoliday >= 0)
        gConfig.holiday = argHoliday;

    if (argNight >= 0)
        gConfig.night = argNight;

    if (argDateOffsetSet)
        gConfig.dateOffset = argDateOffset;

    if (argScale != -100)
        gConfig.scale = argScale;

    if (argMonitor >= 0)
        gConfig.monitor = argMonitor;

    if (argMonitorMode >= 0)
        gConfig.monitorMode = argMonitorMode;

    if (argAudioDevice != -2)
        gConfig.audioDevice = argAudioDevice;

    if (argCrtFilter >= 0)
        gConfig.crtFilter = argCrtFilter;

    if (argSkipIntro >= 0)
        gConfig.skipIntro = argSkipIntro;

    if (argVolume >= 0) {
        gConfig.volume = argVolume;
        if (gConfig.volume > 100)
            gConfig.volume = 100;
    }

    if (argExitClick >= 0)
        gConfig.exitOnClick = argExitClick ? 1 : 0;

    if (argExitMove >= 0)
        gConfig.exitOnMouseMove = argExitMove ? 1 : 0;

    if (argDay >= 1 && argDay <= 11)
        gForcedDay = argDay;

    gDateOffsetDays = gConfig.dateOffset;

#ifdef _WIN32
    if (argScrMode == SCR_MODE_CONFIG) {
        scrShowConfigDialog((void*)(intptr_t) argScrHwnd);
        return 0;
    }

    if (argScrMode == SCR_MODE_RUN) {
        grWindowed = 0;
    }
#endif

    parseResourceFiles("RESOURCE.MAP");

#ifdef _WIN32
    if (argScrMode == SCR_MODE_PREVIEW) {
        graphicsInitEmbedded((void*)(intptr_t) argScrHwnd);
        soundInit();

        // The preview thumbnail in the Windows screensaver settings dialog
        // must keep rendering while the user moves the mouse over the
        // surrounding dialog -- it isn't a real screensaver session.
        evExitOnInputEnabled = 0;

        storyPlay();

        soundEnd();
        graphicsEnd();
        return 0;
    }

#endif

    if (argPlayAll) {
        graphicsInit();
        soundInit();

        storyPlay();

        soundEnd();
        graphicsEnd();
    }

    else if (argDump) {
        dumpAllResources();
    }

    else if (argBench) {
        graphicsInit();
        adsPlayBench();
        graphicsEnd();
    }

    else if (argTtm) {
        graphicsInit();
        soundInit();

        adsPlaySingleTtm(args[0]);

        soundEnd();
        graphicsEnd();
    }

    else if (argAds) {

        graphicsInit();
        soundInit();

        if (argIsland)
            adsInitIsland();
        else
            adsNoIsland();

        adsPlay(args[0], atoi(args[1]));

        soundEnd();
        graphicsEnd();
    }

    return 0;
}

