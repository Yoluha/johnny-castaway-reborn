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


#define CFG_FILENAME ".jc_reborn"

struct TConfig {
    int currentDay;   // 1..11, day of the story
    int date;         // day-of-year of the last session (internal)
    int speed;        // % of normal speed; 100 = normal, 200 = double
    int holiday;       // 0=auto, 1=Halloween, 2=St Patrick, 3=Christmas, 4=New Year
    int night;          // 0=auto, 1=force night, 2=force day
    int dateOffset;      // days added to the system date (virtual date)
    int scale;             // 0=auto integer fit, -1=fill/stretch (ignores aspect ratio), 1..N=explicit integer factor
    int monitor;             // SDL display index, used when monitorMode==0
    int monitorMode;          // 0=single monitor, 1=clone (same on every monitor), 2=extend (span all monitors)
    int audioDevice;           // -1=default playback device, else index into the SDL playback device list
    int crtFilter;             // 0=off, 1=CRT scanlines, 2=green mono, 3=amber mono, 4=strong CRT, 5=faded/sepia
    int skipIntro;              // 0=show the intro screen, 1=skip straight to the story
    int language;                 // 0=Portuguese, 1=English, 2=Spanish (config dialog UI only)
    int volume;                     // 0-100, sound effects volume
    int exitOnClick;                 // 1=a mouse click dismisses the screensaver, 0=ignore (fullscreen mode only; always ignored in preview)
    int exitOnMouseMove;              // 1=moving the mouse dismisses the screensaver, 0=ignore (fullscreen mode only; always ignored in preview)
};

extern struct TConfig gConfig;
extern int gForcedDay;   // -1 = not forced, 1..11 = pin the story to this day

void cfgFileWrite(struct TConfig *cfg);
void cfgFileRead(struct TConfig *cfg);

