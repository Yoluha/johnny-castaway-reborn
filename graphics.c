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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mytypes.h"
#include "utils.h"
#include "graphics.h"
#include "resource.h"
#include "events.h"
#include "config.h"


static SDL_Window *sdl_window;

// Extra windows used by monitorMode==1 (clone): one additional fullscreen
// window per extra monitor, each showing the exact same composed scene.
#define MAX_EXTRA_OUTPUTS 7
static SDL_Window *grExtraWindows[MAX_EXTRA_OUTPUTS];
static int         grNumExtraWindows = 0;

// Set by monitorMode==2 (extend): sdl_window spans the bounding box of every
// monitor, and the composed scene is stretched to fill it exactly (no
// letterboxing), since it's meant to span mismatched monitor bezels as one
// continuous image rather than stay pixel-perfect on any single screen.
static int grExtendMode = 0;

static uint8 ttmPalette[16][4];

static SDL_Surface *grSavedZonesLayer = NULL;

static SDL_Rect grScreenOrigin = { 0, 0, 0, 0 };   // TODO

// The whole scene is composed at a fixed logical resolution of 640x480,
// then scaled (nearest-neighbour, integer factor) into the real window(s),
// which may be larger and live on any monitor.
static SDL_Surface *grComposeSfc = NULL;

// Set when embedded into a foreign window (screensaver /p preview box).
// Scaling then fits the (possibly tiny, non-integer) target window exactly,
// and playback stops as soon as the parent window goes away.
static int grPreviewMode = 0;
static void *grPreviewParentHwnd = NULL;

// How many consecutive frames the preview box has failed the IsWindow()
// check. A single failed check is not trusted on its own -- IsWindow() can
// return a transient false negative right around window creation/
// reparenting, especially when the parent is owned by another process (the
// classic Screen Saver Settings preview box hosted inside the modern
// Settings app is a legacy dialog wrapped by a shim, and has been observed
// to behave a little erratically). Only exit once several consecutive
// frames agree the window is really gone.
static int grPreviewMissingFrames = 0;
#define GR_MISSING_FRAMES_THRESHOLD 15

#ifdef _WIN32
// Always-on diagnostic log (unlike debugMsg(), which prints to a console
// that doesn't exist when launched from the Settings app). Written so a
// "preview opens and closes" report can be checked against what actually
// happened, instead of guessing.
static void grDiagLog(const char *fmt, ...)
{
    char *appData = getenv("APPDATA");
    if (appData == NULL || !strlen(appData))
        return;

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s\\JohnnyCastawayReborn", appData);
    CreateDirectoryA(dir, NULL);

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\preview.log", dir);

    FILE *f = fopen(path, "a");
    if (f == NULL)
        return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t != NULL)
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fclose(f);
}
#endif

SDL_Surface *grBackgroundSfc = NULL;

int grDx = 0;
int grDy = 0;
int grWindowed = 0;
int grUpdateDelay = 0;


static void grReleaseScreen()
{
    free(grBackgroundSfc->pixels);
    SDL_FreeSurface(grBackgroundSfc);
    grBackgroundSfc = NULL;
}


static void grReleaseSavedLayer()
{
    SDL_FreeSurface(grSavedZonesLayer);
    grSavedZonesLayer = NULL;
}


static void grPutPixel(SDL_Surface *sfc, uint16 x, uint16 y, uint8 color)
{
    // TODO: Implement Cohen-Sutherland clipping algorithm or such for
    // grDrawLine(), and another ad hoc algorithm for grDrawCircle()

    if (x>=0 && y>=0 && x<640 && y<480) {

        uint8 *pixel = (uint8*) sfc->pixels;

        pixel += (y * sfc->pitch) + (x * sfc->format->BytesPerPixel);

        pixel[0] = ttmPalette[color][0];
        pixel[1] = ttmPalette[color][1];
        pixel[2] = ttmPalette[color][2];
        pixel[3] = 0;
    }
}


static void grDrawHorizontalLine(SDL_Surface *sfc, sint16 x1, sint16 x2, sint16 y, uint8 color)
{
    if (y < 0 || y > 479)
        return;

    x1 = x1 < 0   ? 0   : x1;
    x2 = x2 > 639 ? 639 : x2;

    for (int x=x1; x<=x2; x++)
        grPutPixel(sfc, x, y, color);
}


void grLoadPalette(struct TPalResource *palResource)
{
    if (palResource == NULL)
        fatalError("NULL palette\n");

    for (int i=0; i < 16; i++) {
        ttmPalette[i][0] = palResource->colors[i].b << 2;
        ttmPalette[i][1] = palResource->colors[i].g << 2;
        ttmPalette[i][2] = palResource->colors[i].r << 2;
        ttmPalette[i][3] = 0;
    }
}


// Old-monitor look-alike filters, applied to the composed 640x480 scene
// BEFORE it gets scaled up to the real window. Applying it here (rather
// than after scaling) means each scanline is exactly one game pixel row
// tall in logical space, so after scaling it becomes a full
// `scale`-pixels-thick band - clearly visible at any zoom level, instead
// of a near-invisible single physical pixel line.
//
// Pixel channel order in this codebase's 32bpp surfaces is [0]=B [1]=G
// [2]=R (see grLoadPalette()), which the monochrome/tint cases below rely
// on.
#define FILTER_NONE       0
#define FILTER_SCANLINES  1
#define FILTER_GREEN_MONO 2
#define FILTER_AMBER_MONO 3
#define FILTER_STRONG_CRT 4
#define FILTER_FADED      5

static void grApplyFilter(SDL_Surface *sfc, int filterType)
{
    if (SDL_LockSurface(sfc) != 0)
        return;

    int bpp = sfc->format->BytesPerPixel;
    int w = sfc->w, h = sfc->h;
    int cx = w / 2, cy = h / 2;
    double maxDistSq = (double) (cx*cx + cy*cy);

    for (int y = 0; y < h; y++) {

        uint8 *row = (uint8*) sfc->pixels + y * sfc->pitch;
        int scanlineRow = (y % 2) == 1;

        for (int x = 0; x < w; x++) {

            uint8 *px = row + x * bpp;

            switch (filterType) {

                case FILTER_SCANLINES:
                    if (scanlineRow) {
                        px[0] = (uint8) (px[0] * 3 / 5);
                        px[1] = (uint8) (px[1] * 3 / 5);
                        px[2] = (uint8) (px[2] * 3 / 5);
                    }
                    break;

                case FILTER_GREEN_MONO:
                case FILTER_AMBER_MONO: {
                    // Standard luma weighting, then tinted like an old
                    // single-colour phosphor monitor.
                    int lum = (px[2]*77 + px[1]*151 + px[0]*28) >> 8;
                    if (filterType == FILTER_GREEN_MONO) {
                        px[2] = (uint8) (lum * 15 / 100);
                        px[1] = (uint8) lum;
                        px[0] = (uint8) (lum * 20 / 100);
                    }
                    else {
                        px[2] = (uint8) lum;
                        px[1] = (uint8) (lum * 65 / 100);
                        px[0] = (uint8) (lum * 10 / 100);
                    }
                    break;
                }

                case FILTER_STRONG_CRT: {
                    // Darker scanlines than the plain CRT filter, plus a
                    // vignette that dims the corners like an old curved
                    // tube losing brightness at the edges.
                    if (scanlineRow) {
                        px[0] = (uint8) (px[0] / 2);
                        px[1] = (uint8) (px[1] / 2);
                        px[2] = (uint8) (px[2] / 2);
                    }

                    int dx = x - cx, dy = y - cy;
                    double falloff = 1.0 - 0.35 * ((dx*dx + dy*dy) / maxDistSq);
                    if (falloff < 0.5)
                        falloff = 0.5;

                    px[0] = (uint8) (px[0] * falloff);
                    px[1] = (uint8) (px[1] * falloff);
                    px[2] = (uint8) (px[2] * falloff);
                    break;
                }

                case FILTER_FADED: {
                    // The earlier version of this filter (a flat 70-85%
                    // brightness scale) was too close to the original
                    // image to read as "faded" at a glance. Use the
                    // classic sepia-tone matrix transform instead, then
                    // flatten contrast toward a washed-out mid-tone -- an
                    // old, sun-bleached CRT look that's unmistakable.
                    int r = px[2], g = px[1], b = px[0];

                    int sepiaR = (r*393 + g*769 + b*189) / 1000;
                    int sepiaG = (r*349 + g*686 + b*168) / 1000;
                    int sepiaB = (r*272 + g*534 + b*131) / 1000;

                    if (sepiaR > 255) sepiaR = 255;
                    if (sepiaG > 255) sepiaG = 255;
                    if (sepiaB > 255) sepiaB = 255;

                    sepiaR = sepiaR * 75 / 100 + 32;
                    sepiaG = sepiaG * 75 / 100 + 28;
                    sepiaB = sepiaB * 75 / 100 + 20;

                    px[2] = (uint8) (sepiaR > 255 ? 255 : sepiaR);
                    px[1] = (uint8) (sepiaG > 255 ? 255 : sepiaG);
                    px[0] = (uint8) (sepiaB > 255 ? 255 : sepiaB);
                    break;
                }
            }
        }
    }

    SDL_UnlockSurface(sfc);
}


// Blits the fixed 640x480 composed scene onto one real window surface,
// nearest-neighbour scaled to fit it (or stretched to fill it exactly, in
// extend mode).
static void grPresentToWindow(SDL_Window *win)
{
    SDL_Surface *winSfc = SDL_GetWindowSurface(win);
    if (winSfc == NULL)
        return;

    int winW = winSfc->w;
    int winH = winSfc->h;
    SDL_Rect dest;

    if (grExtendMode || gConfig.scale == -1) {
        // Fill mode: stretch to the exact window size, ignoring the 4:3
        // aspect ratio and the integer-scale constraint. Chosen explicitly
        // (scale=-1 / scale=fill) when someone wants the picture to cover
        // the whole screen rather than stay pixel-perfect.
        dest.x = 0;
        dest.y = 0;
        dest.w = winW;
        dest.h = winH;
    }
    else if (grPreviewMode || gConfig.scale == -2) {
        // Fit mode: fractional scale allowed (not limited to whole
        // multiples), but the 4:3 aspect ratio is preserved -- fills as
        // much of the window as possible without distorting the picture.
        // Also used for the (possibly tiny) screensaver preview box.
        double scaleX = (double) winW / SCREEN_WIDTH;
        double scaleY = (double) winH / SCREEN_HEIGHT;
        double scale = scaleX < scaleY ? scaleX : scaleY;
        if (scale <= 0)
            scale = 0.01;

        dest.w = (int) (SCREEN_WIDTH * scale);
        dest.h = (int) (SCREEN_HEIGHT * scale);
        dest.x = (winW - dest.w) / 2;
        dest.y = (winH - dest.h) / 2;
    }
    else if (gConfig.scale == -3) {
        // Cover mode: like "fit", the aspect ratio is preserved (no
        // distortion), but the *larger* of the two scale factors is used
        // instead of the smaller one, so the picture overflows the window
        // on one axis instead of leaving letterbox bars -- the overflow is
        // simply clipped by SDL's blit, like CSS background-size:cover.
        double scaleX = (double) winW / SCREEN_WIDTH;
        double scaleY = (double) winH / SCREEN_HEIGHT;
        double scale = scaleX > scaleY ? scaleX : scaleY;
        if (scale <= 0)
            scale = 0.01;

        dest.w = (int) (SCREEN_WIDTH * scale);
        dest.h = (int) (SCREEN_HEIGHT * scale);
        dest.x = (winW - dest.w) / 2;
        dest.y = (winH - dest.h) / 2;
    }
    else {
        int scale = gConfig.scale;

        if (scale <= 0) {
            int maxScaleW = winW / SCREEN_WIDTH;
            int maxScaleH = winH / SCREEN_HEIGHT;
            scale = maxScaleW < maxScaleH ? maxScaleW : maxScaleH;
        }

        if (scale < 1)
            scale = 1;

        dest.w = SCREEN_WIDTH * scale;
        dest.h = SCREEN_HEIGHT * scale;
        dest.x = (winW - dest.w) / 2;
        dest.y = (winH - dest.h) / 2;
    }

    SDL_FillRect(winSfc, NULL, SDL_MapRGB(winSfc->format, 0, 0, 0));
    SDL_BlitScaled(grComposeSfc, NULL, winSfc, &dest);

    SDL_UpdateWindowSurface(win);
}


// Presents the composed scene to every active output window (normally just
// one, but clone mode uses one per monitor).
static void grPresent()
{
#ifdef _WIN32
    // If we're embedded in someone else's window (screensaver preview box)
    // and that window has gone away, there's nothing left to render into.
    // A single failed check isn't trusted (see GR_MISSING_FRAMES_THRESHOLD
    // comment above) - only exit once it's failed several frames in a row.
    if (grPreviewMode && grPreviewParentHwnd) {
        if (!IsWindow((HWND) grPreviewParentHwnd)) {
            grPreviewMissingFrames++;
            if (grPreviewMissingFrames == 1)
                grDiagLog("preview: IsWindow() failed on hwnd=%p (frame 1 of %d before exiting)",
                    grPreviewParentHwnd, GR_MISSING_FRAMES_THRESHOLD);
            if (grPreviewMissingFrames >= GR_MISSING_FRAMES_THRESHOLD) {
                grDiagLog("preview: hwnd=%p confirmed gone after %d consecutive frames, exiting",
                    grPreviewParentHwnd, grPreviewMissingFrames);
                exit(0);
            }
        }
        else {
            grPreviewMissingFrames = 0;
        }
    }
#endif

    // Applied once to the logical 640x480 scene (not per output window),
    // so clone mode doesn't darken it multiple times.
    if (gConfig.crtFilter > FILTER_NONE)
        grApplyFilter(grComposeSfc, gConfig.crtFilter);

    grPresentToWindow(sdl_window);

    for (int i = 0; i < grNumExtraWindows; i++)
        grPresentToWindow(grExtraWindows[i]);
}


static void graphicsInitCommon()
{
    grComposeSfc = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    SDL_UpdateWindowSurface(sdl_window);

    grLoadPalette(palResources[0]);  // TODO ?

    srand(time(NULL));

    eventsInit();
}


void grPrintMonitors()
{
    SDL_Init(SDL_INIT_VIDEO);

    int numDisplays = SDL_GetNumVideoDisplays();
    printf("\n Monitores disponiveis:\n\n");

    for (int i = 0; i < numDisplays; i++) {
        SDL_Rect bounds = {0,0,0,0};
        SDL_GetDisplayBounds(i, &bounds);
        const char *name = SDL_GetDisplayName(i);

        printf("   monitor=%d  ->  %dx%d  posicao (%d,%d)  %s%s\n",
            i, bounds.w, bounds.h, bounds.x, bounds.y,
            name ? name : "",
            (bounds.h > bounds.w) ? "  [vertical]" : "");
    }

    printf("\n Usa: jc_reborn monitor=<indice> [window|scale=<n>|...]\n\n");

    SDL_Quit();
}


// monitorMode==2: one borderless window spanning the bounding box of every
// detected monitor; the composed scene is stretched to fill it (see
// grExtendMode in grPresentToWindow)
static void graphicsInitExtend(int numDisplays)
{
    SDL_Rect u;
    SDL_GetDisplayBounds(0, &u);

    for (int i = 1; i < numDisplays; i++) {
        SDL_Rect b;
        SDL_GetDisplayBounds(i, &b);
        int minX = u.x < b.x ? u.x : b.x;
        int minY = u.y < b.y ? u.y : b.y;
        int maxX = (u.x + u.w) > (b.x + b.w) ? (u.x + u.w) : (b.x + b.w);
        int maxY = (u.y + u.h) > (b.y + b.h) ? (u.y + u.h) : (b.y + b.h);
        u.x = minX; u.y = minY;
        u.w = maxX - minX; u.h = maxY - minY;
    }

    grExtendMode = 1;
    grWindowed = 0;

    // Unlike the single-monitor and clone paths, this window can't use
    // SDL_WINDOW_FULLSCREEN_DESKTOP (that flag is tied to one specific
    // display in SDL's model) - it's a plain borderless window manually
    // sized to span every monitor's combined bounding box instead. SDL
    // doesn't give a plain borderless window the same "always on top and
    // focused" treatment it gives a real fullscreen one, so without an
    // explicit push it can end up created successfully yet sitting behind
    // whatever else had focus - invisible despite "working".
    sdl_window = SDL_CreateWindow("Johnny Reborn ...?",
        u.x, u.y, u.w, u.h, SDL_WINDOW_BORDERLESS);

    if (sdl_window == NULL)
        fatalError("Could not create window: %s", SDL_GetError());

#ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
        HWND hwnd = wmInfo.info.win.window;
        SetWindowPos(hwnd, HWND_TOPMOST, u.x, u.y, u.w, u.h, SWP_SHOWWINDOW);
        SetForegroundWindow(hwnd);
    }
#endif

    SDL_RaiseWindow(sdl_window);
    SDL_ShowCursor(SDL_DISABLE);
}


// monitorMode==1: one fullscreen window per monitor, all showing the exact
// same composed scene
static void graphicsInitClone(int numDisplays)
{
    grWindowed = 0;

    sdl_window = SDL_CreateWindow("Johnny Reborn ...?",
        SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0),
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (sdl_window == NULL)
        fatalError("Could not create window: %s", SDL_GetError());

    SDL_ShowCursor(SDL_DISABLE);

    grNumExtraWindows = 0;
    for (int i = 1; i < numDisplays && grNumExtraWindows < MAX_EXTRA_OUTPUTS; i++) {
        SDL_Window *w = SDL_CreateWindow("Johnny Reborn ...?",
            SDL_WINDOWPOS_CENTERED_DISPLAY(i), SDL_WINDOWPOS_CENTERED_DISPLAY(i),
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);

        if (w != NULL)
            grExtraWindows[grNumExtraWindows++] = w;
    }
}


// monitorMode==0 (default): a single window on one chosen monitor
static void graphicsInitSingle(int numDisplays)
{
    int monitor = gConfig.monitor;
    if (monitor < 0 || monitor >= numDisplays)
        monitor = 0;

    int initialScale = gConfig.scale > 0 ? gConfig.scale : 2;

    int winW = SCREEN_WIDTH * initialScale;
    int winH = SCREEN_HEIGHT * initialScale;

    // In windowed mode, never create a window bigger than the target
    // display, regardless of the requested scale
    if (grWindowed) {
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(monitor, &usable) == 0) {

            if (gConfig.scale == -1 || gConfig.scale == -2 || gConfig.scale == -3) {
                // Fill/fit mode: make the window itself take up the whole
                // usable desktop area, so there's something big to stretch
                // (or fractionally fit) the picture into -- the actual
                // scaling happens at present time, in grPresentToWindow.
                winW = usable.w;
                winH = usable.h;
            }
            else {
                int maxFitScale = usable.w / SCREEN_WIDTH;
                int maxFitScaleH = usable.h / SCREEN_HEIGHT;
                if (maxFitScaleH < maxFitScale)
                    maxFitScale = maxFitScaleH;
                if (maxFitScale < 1)
                    maxFitScale = 1;
                if (initialScale > maxFitScale)
                    initialScale = maxFitScale;

                winW = SCREEN_WIDTH * initialScale;
                winH = SCREEN_HEIGHT * initialScale;
            }
        }
    }

    sdl_window = SDL_CreateWindow(
        "Johnny Reborn ...?",
        SDL_WINDOWPOS_CENTERED_DISPLAY(monitor),
        SDL_WINDOWPOS_CENTERED_DISPLAY(monitor),
        winW,
        winH,
        (grWindowed ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP)
    );

    if (sdl_window == NULL)
        fatalError("Could not create window: %s", SDL_GetError());

    if (!grWindowed)
        SDL_ShowCursor(SDL_DISABLE);
}


void graphicsInit()
{
    SDL_Init(SDL_INIT_VIDEO);

    int numDisplays = SDL_GetNumVideoDisplays();

    if (gConfig.monitorMode == 2 && numDisplays > 1)
        graphicsInitExtend(numDisplays);
    else if (gConfig.monitorMode == 1 && numDisplays > 1)
        graphicsInitClone(numDisplays);
    else
        graphicsInitSingle(numDisplays);

    graphicsInitCommon();
}


void graphicsInitEmbedded(void *nativeWindowHandle)
{
#ifdef _WIN32
    grDiagLog("graphicsInitEmbedded: hwnd=%p IsWindow=%d",
        nativeWindowHandle, nativeWindowHandle ? IsWindow((HWND) nativeWindowHandle) : -1);
#endif

    SDL_Init(SDL_INIT_VIDEO);

    grPreviewMode = 1;
    grPreviewParentHwnd = nativeWindowHandle;
    grPreviewMissingFrames = 0;

    sdl_window = SDL_CreateWindowFrom(nativeWindowHandle);

    if (sdl_window == NULL)
        fatalError("Could not wrap preview window: %s", SDL_GetError());

    graphicsInitCommon();
}


void graphicsEnd()
{
    SDL_FreeSurface(grComposeSfc);
    grComposeSfc = NULL;

    for (int i = 0; i < grNumExtraWindows; i++)
        SDL_DestroyWindow(grExtraWindows[i]);
    grNumExtraWindows = 0;

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}


void grRefreshDisplay()
{
    if (grComposeSfc != NULL)
        grPresent();
    SDL_UpdateWindowSurface(sdl_window);
}


void grToggleFullScreen()
{
    grWindowed = !grWindowed;

    if (grWindowed) {
        SDL_SetWindowFullscreen(sdl_window, 0);
        SDL_ShowCursor(SDL_ENABLE);
    }
    else {
        SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    }

    SDL_UpdateWindowSurface(sdl_window);
}


void grUpdateDisplay(struct TTtmThread *ttmBackgroundThread,
                     struct TTtmThread *ttmThreads,
                     struct TTtmThread *ttmHolidayThread)
{
    // Blit the background
    if (grBackgroundSfc != NULL)
        SDL_BlitSurface(grBackgroundSfc,
                        NULL,
                        grComposeSfc,
                        &grScreenOrigin);

    // If not NULL, blit the optional layer of saved zones
    if (grSavedZonesLayer != NULL)
        SDL_BlitSurface(grSavedZonesLayer,
                        NULL,
                        grComposeSfc,
                        &grScreenOrigin);


    // Blit successively each thread's layer
    for (int i=0; i < MAX_TTM_THREADS; i++)
        if (ttmThreads[i].isRunning)
            SDL_BlitSurface(ttmThreads[i].ttmLayer,
                            NULL,
                            grComposeSfc,
                            &grScreenOrigin);

    // Finally, blit the holiday layer
    if (ttmHolidayThread != NULL)
        if (ttmHolidayThread->isRunning)
            SDL_BlitSurface(ttmHolidayThread->ttmLayer,
                            NULL,
                            grComposeSfc,
                            &grScreenOrigin);

    // Scale the composed scene onto the real window
    grPresent();

    // Wait for the tick ...
    eventsWaitTick(grUpdateDelay);

    // ... and refresh the display
    SDL_UpdateWindowSurface(sdl_window);
}


SDL_Surface *grNewLayer()
{
    SDL_Surface *sfc = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480, 32, 0, 0, 0, 0);
    SDL_Rect dest = { 0, 0, 640, 480 };
    SDL_FillRect(sfc, &dest, SDL_MapRGB(sfc->format, 0xa8, 0, 0xa8));
    SDL_SetColorKey(sfc, SDL_TRUE, SDL_MapRGB(sfc->format, 0xa8, 0, 0xa8));

    return sfc;
}


void grFreeLayer(SDL_Surface *sfc)
{
    SDL_FreeSurface(sfc);
}


void grSetClipZone(SDL_Surface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2)
{
    x1 += grDx; y1 += grDy;
    x2 += grDx; y2 += grDy;

    SDL_Rect rect = { x1, y1, x2-x1, y2-y1 };
    SDL_SetClipRect(sfc, &rect);
}


void grCopyZoneToBg(SDL_Surface *sfc, uint16 x, uint16 y, uint16 width, uint16 height)
{
    x += grDx; y += grDy;
    SDL_Rect rect = { (short) x, (short) y, width + 2, height };

    if (grSavedZonesLayer == NULL)
        grSavedZonesLayer = grNewLayer();

    SDL_BlitSurface(sfc, &rect, grSavedZonesLayer, &rect);

    // Note : without the +2 in width+2 above, there would be a graphical
    // glitch (2 unfilled pixels) on the hull of the cargo, caused by an
    // error in coordinates in GJIVS6.TTM
    // Obviously, the original soft rounds the SAVE_IMAGE boundaries on
    // one way or another.
}


void grSaveImage1(SDL_Surface *sfc, uint16 arg0, uint16 arg1, uint16 arg2, uint16 arg3) // TODO : rename ?
{
//    ttmSetColors(4,4);
//    ttmDrawRect(arg0,arg1,arg2,arg3);
//    ttmSaveImage0(arg0,arg1,arg2,arg3);
//    ttmUpdate();
}


void grSaveZone(SDL_Surface *sfc, uint16 x, uint16 y, uint16 width, uint16 height)
{
    // Minimalistic implementation: we don't really save the zone,
    // and let grRestoreZone() simply erase the 'saved zones' layer
}


void grRestoreZone(SDL_Surface *sfc, uint16 x, uint16 y, uint16 width, uint16 height)
{
    // In Johnny's TTMs, we never have RESTORE_ZONE called
    // while several zones are saved. So we simply free the
    // whole saved zones layer
    grReleaseSavedLayer();
}


void grDrawPixel(SDL_Surface *sfc, sint16 x, sint16 y, uint8 color)
{
    x += grDx; y += grDy;
    grPutPixel(sfc, x, y, color);
}


void grDrawLine(SDL_Surface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2, uint8 color)
{
    x1 += grDx; y1 += grDy;
    x2 += grDx; y2 += grDy;

    SDL_LockSurface(sfc);

    // Bresenham's line drawing algorithm
    // Note : the code below intends to be pixel-perfect

    uint16 dx, dy, cumul, x, y;
    int xinc, yinc;

    x = x1;
    y = y1;
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    xinc = (x2>x1 ? 1 : -1);
    yinc = (y2>y1 ? 1 : -1);

    if (dy < dx) {
        cumul = (dx + 1) >> 1;

        for (int i=0; i < dx; i++) {

            grPutPixel(sfc, x, y, color);

            x += xinc;
            cumul += dy;

            if (cumul > dx) {
                cumul -= dx;
                y += yinc;
            }
        }
    }
    else {
        cumul = (dy + 1) >> 1;

        for (int i=0; i < dy; i++) {

            grPutPixel(sfc, x, y, color);

            y += yinc;
            cumul += dx;

            if (cumul > dy) {
                cumul -= dy;
                x += xinc;
            }
        }
    }

    SDL_UnlockSurface(sfc);
}


void grDrawRect(SDL_Surface *sfc, sint16 x, sint16 y, uint16 width, uint16 height, uint8 color)
{
    x += grDx; y += grDy;

    SDL_Rect dest = { x, y, width, height };
    SDL_FillRect(sfc,
                 &dest,
                 SDL_MapRGB(sfc->format,
                            ttmPalette[color][2],  // TODO ?
                            ttmPalette[color][1],
                            ttmPalette[color][0]
                 )
    );
}


void grDrawCircle(SDL_Surface *sfc, sint16 x1, sint16 y1, uint16 width, uint16 height, uint8 fgColor, uint8 bgColor)
{
    x1 += grDx; y1 += grDy;

    // We can only draw regular circles
    if (width != height) {
        fprintf(stderr, "Warning : grDrawCircle() : unable to draw ellipse\n");
        return;
    }

    // In original data, every width is even
    if (width % 2) {
        fprintf(stderr, "Warning : grDrawCircle() : unable to process odd diameters\n");
        return;
    }

    // Bresenham's circle drawing algorithm
    // Note : the code below intends to be pixel-perfect

    SDL_LockSurface(sfc);

    uint16 r = (width >> 1) - 1;
    uint16 xc = x1 + r;
    uint16 yc = y1 + r;
    sint16 x = 0;
    sint16 y = r;
    int d = 1 - r;

    while (1) {

        grDrawHorizontalLine(sfc, xc-x, xc+x+1, yc+y+1, bgColor);
        grDrawHorizontalLine(sfc, xc-x, xc+x+1, yc-y  , bgColor);

        grDrawHorizontalLine(sfc, xc-y, xc+y+1, yc+x+1, bgColor);
        grDrawHorizontalLine(sfc, xc-y, xc+y+1, yc-x  , bgColor);

        if (y-x <= 1)
            break;

        if (d < 0)
            d += (x << 1) + 3;
        else {
            d += ((x - y) << 1) + 5;
            y--;
        }

        x++;
    }

    if (fgColor != bgColor) {

        x = 0;
        y = r;
        d = 1 - r;

        while (1) {

            grPutPixel(sfc, xc-x  , yc+y+1, fgColor);
            grPutPixel(sfc, xc+x+1, yc+y+1, fgColor);

            grPutPixel(sfc, xc-x  , yc-y  , fgColor);
            grPutPixel(sfc, xc+x+1, yc-y  , fgColor);

            grPutPixel(sfc, xc-y  , yc+x+1, fgColor);
            grPutPixel(sfc, xc+y+1, yc+x+1, fgColor);

            grPutPixel(sfc, xc-y  , yc-x  , fgColor);
            grPutPixel(sfc, xc+y+1, yc-x  , fgColor);

            if (y-x <= 1)
                break;

            if (d < 0)
                d += (x << 1) + 3;
            else {
                d += ((x - y) << 1) + 5;
                y--;
            }

            x++;
        }
    }

    SDL_UnlockSurface(sfc);
}


void grDrawSprite(SDL_Surface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo)
{
    if (spriteNo >= ttmSlot->numSprites[imageNo]) {
        fprintf(stderr, "Warning : grDrawSprite(): less than %d sprites loaded in slot %d\n", imageNo, spriteNo);
        return;
    }

    x += grDx; y += grDy;

    SDL_Surface *srcSfc = ttmSlot->sprites[imageNo][spriteNo];

    SDL_Rect dest = { x, y, 0, 0 };
    SDL_BlitSurface(srcSfc, NULL, sfc, &dest);
}


void grDrawSpriteFlip(SDL_Surface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo)
{
    if (spriteNo >= ttmSlot->numSprites[imageNo]) {
        fprintf(stderr, "Warning : grDrawSpriteFlip(): less than %d sprites loaded in slot %d\n", imageNo, spriteNo);
        return;
    }

    x += grDx; y += grDy;

    SDL_Surface *srcSfc = ttmSlot->sprites[imageNo][spriteNo];
    x += srcSfc->w - 1;

    for (int i=0; i < srcSfc->w; i++) {

        SDL_Rect src = { i, 0, 1, srcSfc->h };
        SDL_Rect dest = { x - i, y, 0, 0 };

        SDL_BlitSurface(srcSfc, &src, sfc, &dest);
    }
}


void grClearScreen(SDL_Surface *sfc)
{
    SDL_Rect rect;

    SDL_GetClipRect(sfc, &rect);
    SDL_SetClipRect(sfc, NULL);
    SDL_FillRect(sfc, NULL, SDL_MapRGB(sfc->format, 0xa8, 0, 0xa8));
    SDL_SetClipRect(sfc, &rect);
}


void grLoadScreen(char *strArg)
{
    if (grBackgroundSfc != NULL)
        grReleaseScreen();

    if (grSavedZonesLayer != NULL)
        grReleaseSavedLayer();

    struct TScrResource *scrResource = findScrResource(strArg);

    if ((scrResource->width % 2) == 1) {
        fprintf(stderr, "Warning: grLoadScreen(): can't manage odd widths\n");
    }

    if (scrResource->width > 640 || scrResource->height > 480) {
        fatalError("grLoadScreen(): can't manage more than 640x480 resolutions");
    }

    uint16 width  = scrResource->width;
    uint16 height = scrResource->height;

    uint8 *outData = safe_malloc(width * height * sizeof(uint32));

    uint8 *inPtr  = scrResource->uncompressedData;
    uint8 *outPtr = outData;

    for (int inOffset=0; inOffset < width*height/2; inOffset++) {
        memcpy(outPtr, ttmPalette[(inPtr[0] & 0xf0) >> 4] , 4); outPtr += 4;
        memcpy(outPtr, ttmPalette[(inPtr[0] & 0x0f)     ] , 4); outPtr += 4;
        inPtr++;
    }

    grBackgroundSfc = SDL_CreateRGBSurfaceFrom((void*)outData,
                                      width, height, 32, 4*width, 0, 0, 0, 0);
}


void grInitEmptyBackground()
{
    if (grBackgroundSfc != NULL)
        grReleaseScreen();

    if (grSavedZonesLayer != NULL)
        grReleaseSavedLayer();

    uint8 *data = safe_malloc(640 * 480 * sizeof(uint32));
    memset(data, 0, 640 * 480 * sizeof(uint32));
    grBackgroundSfc = SDL_CreateRGBSurfaceFrom((void*)data,
                                      640, 480, 32, 4*640, 0, 0, 0, 0);
}


void grReleaseBmp(struct TTtmSlot *ttmSlot, uint16 bmpSlotNo)
{
    for (int i=0; i < ttmSlot->numSprites[bmpSlotNo]; i++) {
        free(ttmSlot->sprites[bmpSlotNo][i]->pixels);
        SDL_FreeSurface(ttmSlot->sprites[bmpSlotNo][i]);
    }

    ttmSlot->numSprites[bmpSlotNo] = 0;
}


void grLoadBmp(struct TTtmSlot *ttmSlot, uint16 slotNo, char *strArg)
{
    if (ttmSlot->numSprites[slotNo])
        grReleaseBmp(ttmSlot, slotNo);

    struct TBmpResource *bmpResource = findBmpResource(strArg);
    uint8 *inPtr = bmpResource->uncompressedData;

    ttmSlot->numSprites[slotNo] = bmpResource->numImages;

    for (int image=0; image < bmpResource->numImages; image++) {

        if ((bmpResource->widths[image] % 2) == 1)
            fatalError("grLoadBmp(): can't manage odd widths");

        uint16 width  = bmpResource->widths[image];
        uint16 height = bmpResource->heights[image];

        uint8 *outData = safe_malloc(width * height * sizeof(uint32));

        uint8 *outPtr = outData;

        for (int inOffset=0; inOffset < (width*height/2); inOffset++) {
            memcpy(outPtr, ttmPalette[(inPtr[0] & 0xf0) >> 4] , 4); outPtr += 4;
            memcpy(outPtr, ttmPalette[(inPtr[0] & 0x0f)     ] , 4); outPtr += 4;
            inPtr++;
        }

        SDL_Surface *surface = SDL_CreateRGBSurfaceFrom((void*)outData,
                                               width, height, 32, 4*width, 0, 0, 0, 0);
        SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0xa8, 0, 0xa8));
        ttmSlot->sprites[slotNo][image] = surface;
    }
}


void grFadeOut()
{
    static int fadeOutType = 0;
    SDL_Surface *sfc = grComposeSfc;
    SDL_Surface *tmpSfc = grNewLayer();


    grDx = grDy = 0;

    switch (fadeOutType) {

        // Circle from center
        case 0:
            // Note: we use tmpSfc to be sure we have a 32bpp surface,
            // which is needed by grDrawCircle()
            for (int radius=20; radius <= 400; radius += 20) {
                grDrawCircle(tmpSfc, 320 - radius, 240 - radius,
                    radius << 1, radius << 1, 5, 5);
                SDL_BlitSurface(tmpSfc, NULL, sfc, &grScreenOrigin);
                eventsWaitTick(1);
                grPresent();
                SDL_UpdateWindowSurface(sdl_window);
            }
            break;

        // Rectangle from center
        case 1:
            for (int i=1; i <= 20; i++) {
                grDrawRect(sfc, grScreenOrigin.x + 320 - i*16, grScreenOrigin.y + 240 - i*12, i*32, i*24, 5);
                eventsWaitTick(1);
                grPresent();
                SDL_UpdateWindowSurface(sdl_window);
            }
            break;

        // Right to left
        case 2:
            for (int i=600; i >= 0; i -= 40) {
                grDrawRect(sfc, grScreenOrigin.x + i, grScreenOrigin.y, 40, 480, 5);
                eventsWaitTick(1);
                grPresent();
                SDL_UpdateWindowSurface(sdl_window);
            }
            break;

        // Left to right
        case 3:
            for (int i=0; i < 640; i += 40) {
                grDrawRect(sfc, grScreenOrigin.x + i, grScreenOrigin.y, 40, 480, 5);
                eventsWaitTick(1);
                grPresent();
                SDL_UpdateWindowSurface(sdl_window);
            }
            break;

        // Middle to left and right
        case 4:
            for (int i=0; i < 320; i += 20) {
                grDrawRect(sfc, grScreenOrigin.x + 320+i, grScreenOrigin.y, 20, 480, 5);
                grDrawRect(sfc, grScreenOrigin.x + 300-i, grScreenOrigin.y, 20, 480, 5);
                eventsWaitTick(1);
                grPresent();
                SDL_UpdateWindowSurface(sdl_window);
            }
            break;
    }

    grFreeLayer(tmpSfc);

    fadeOutType = (fadeOutType + 1) % 5;
}

