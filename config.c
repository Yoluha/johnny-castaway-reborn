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

#ifdef _WIN32
#include <windows.h>
#endif

#include "mytypes.h"
#include "utils.h"
#include "config.h"

#define BUFFER_LEN 100

struct TConfig gConfig;
int gForcedDay = -1;


static char *cfgFullPath()
{
    static char *result = NULL;

    if (result == NULL) {

#ifdef _WIN32
        // HOME isn't a standard Windows environment variable (it's not
        // reliably set outside of dev-tool shells), and the working
        // directory at launch time can't be trusted either -- the
        // screensaver is often started from System32, which a normal
        // user can't write to. %APPDATA% is always writable by the
        // logged-in user, regardless of where the exe itself lives.
        char *appData = getenv("APPDATA");

        if (appData != NULL && strlen(appData)) {
            char dir[MAX_PATH];
            snprintf(dir, sizeof(dir), "%s\\JohnnyCastawayReborn", appData);
            CreateDirectoryA(dir, NULL);   // fine if it already exists

            result = safe_malloc(strlen(dir) + strlen(CFG_FILENAME) + 2);
            strcpy(result, dir);
            strcat(result, "\\");
            strcat(result, CFG_FILENAME);
        }
#else
        char *home = getenv("HOME");

        if (home != NULL && strlen(home)) {
            result = safe_malloc(strlen(home) + strlen(CFG_FILENAME) + 2);
            strcpy(result, home);
            strcat(result, "/");
            strcat(result, CFG_FILENAME);
        }
#endif

        if (result == NULL) {
            result = safe_malloc(strlen(CFG_FILENAME) + 1);
            strcpy(result, CFG_FILENAME);
        }
    }

    return result;
}


void cfgFileWrite(struct TConfig *cfg)
{
    FILE *f = fopen(cfgFullPath(), "w");

    if (f == NULL) {
        debugMsg("Warning: couldn't open %s for writing", CFG_FILENAME);
    }
    else {
        fprintf(f, "currentDay=%d\n", cfg->currentDay);
        fprintf(f, "date=%d\n", cfg->date);
        fprintf(f, "speed=%d\n", cfg->speed);
        fprintf(f, "holiday=%d\n", cfg->holiday);
        fprintf(f, "night=%d\n", cfg->night);
        fprintf(f, "dateOffset=%d\n", cfg->dateOffset);
        fprintf(f, "scale=%d\n", cfg->scale);
        fprintf(f, "monitor=%d\n", cfg->monitor);
        fprintf(f, "monitorMode=%d\n", cfg->monitorMode);
        fprintf(f, "audioDevice=%d\n", cfg->audioDevice);
        fprintf(f, "crtFilter=%d\n", cfg->crtFilter);
        fprintf(f, "skipIntro=%d\n", cfg->skipIntro);
        fprintf(f, "language=%d\n", cfg->language);
        fprintf(f, "volume=%d\n", cfg->volume);
        fprintf(f, "exitOnClick=%d\n", cfg->exitOnClick);
        fprintf(f, "exitOnMouseMove=%d\n", cfg->exitOnMouseMove);
        fclose(f);
    }
}


void cfgFileRead(struct TConfig *cfg)
{
    char buf[BUFFER_LEN];

    cfg->currentDay  = 0;
    cfg->date        = 0;
    cfg->speed       = 100;
    cfg->holiday     = 0;
    cfg->night       = 0;
    cfg->dateOffset  = 0;
    cfg->scale       = 0;
    cfg->monitor     = 0;
    cfg->monitorMode = 0;
    cfg->audioDevice = -1;
    cfg->crtFilter   = 0;
    cfg->skipIntro   = 1;
    cfg->language    = 0;
    cfg->volume      = 100;
    cfg->exitOnClick     = 1;
    cfg->exitOnMouseMove = 1;

    FILE *f = fopen(cfgFullPath(), "r");

    if (f != NULL) {

        while (!feof(f)) {

            fgets(buf, BUFFER_LEN, f);

            if (!feof(f)) {

                if (strstr(buf, "currentDay=") == buf)
                    cfg->currentDay = atoi(buf + 11);

                else if (strstr(buf, "dateOffset=") == buf)
                    cfg->dateOffset = atoi(buf + 11);

                else if (strstr(buf, "date=") == buf)
                    cfg->date = atoi(buf + 5);

                else if (strstr(buf, "speed=") == buf)
                    cfg->speed = atoi(buf + 6);

                else if (strstr(buf, "holiday=") == buf)
                    cfg->holiday = atoi(buf + 8);

                else if (strstr(buf, "night=") == buf)
                    cfg->night = atoi(buf + 6);

                else if (strstr(buf, "scale=") == buf)
                    cfg->scale = atoi(buf + 6);

                else if (strstr(buf, "monitorMode=") == buf)
                    cfg->monitorMode = atoi(buf + 12);

                else if (strstr(buf, "monitor=") == buf)
                    cfg->monitor = atoi(buf + 8);

                else if (strstr(buf, "audioDevice=") == buf)
                    cfg->audioDevice = atoi(buf + 12);

                else if (strstr(buf, "crtFilter=") == buf)
                    cfg->crtFilter = atoi(buf + 10);

                else if (strstr(buf, "skipIntro=") == buf)
                    cfg->skipIntro = atoi(buf + 10);

                else if (strstr(buf, "language=") == buf)
                    cfg->language = atoi(buf + 9);

                else if (strstr(buf, "volume=") == buf)
                    cfg->volume = atoi(buf + 7);

                else if (strstr(buf, "exitOnClick=") == buf)
                    cfg->exitOnClick = atoi(buf + 12);

                else if (strstr(buf, "exitOnMouseMove=") == buf)
                    cfg->exitOnMouseMove = atoi(buf + 16);
            }
        }

        fclose(f);
    }

    if (cfg->speed <= 0)
        cfg->speed = 100;

    // Defensive clamps: a hand-edited or corrupted config file must never
    // be able to make the rest of the program misbehave (e.g. language is
    // used as an array index into a 3-entry string table).
    if (cfg->language < 0 || cfg->language > 2)
        cfg->language = 0;

    if (cfg->volume < 0 || cfg->volume > 100)
        cfg->volume = 100;

    if (cfg->exitOnClick != 0 && cfg->exitOnClick != 1)
        cfg->exitOnClick = 1;

    if (cfg->exitOnMouseMove != 0 && cfg->exitOnMouseMove != 1)
        cfg->exitOnMouseMove = 1;

    if (cfg->currentDay < 0 || cfg->currentDay > 11)
        cfg->currentDay = 0;

    if (cfg->crtFilter < 0 || cfg->crtFilter > 5)
        cfg->crtFilter = 0;
}

