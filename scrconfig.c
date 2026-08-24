/*
 *  This file is part of 'Johnny Reborn'
 *
 *  Native Win32 configuration dialog for the .scr screensaver wrapper
 *  (/c command line convention). Built with plain CreateWindow calls,
 *  no resource compiler needed. Supports PT/EN/ES with live retranslation.
 */

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "config.h"
#include "scrconfig.h"

#define ID_DAY          101
#define ID_SPEED        102
#define ID_HOLIDAY      103
#define ID_NIGHT        104
#define ID_SCALE        105
#define ID_MONITOR      106
#define ID_MONITORMODE  109
#define ID_AUDIODEVICE  110
#define ID_CRTFILTER    111
#define ID_SKIPINTRO    112
#define ID_LANGUAGE     113
#define ID_VOLUME       114
#define ID_EXITCLICK    115
#define ID_EXITMOVE     116
#define ID_OK           107
#define ID_CANCEL       108

#define LANG_PT 0
#define LANG_EN 1
#define LANG_ES 2
#define NUM_LANGS 3

enum {
    STR_TITLE,
    STR_DAY,
    STR_SPEED,
    STR_HOLIDAY,
    STR_NIGHT,
    STR_SCALE,
    STR_MONITOR,
    STR_MONITORMODE,
    STR_AUDIODEVICE,
    STR_FILTER_LABEL,
    STR_SKIPINTRO,
    STR_LANGUAGE,
    STR_VOLUME,
    STR_EXITCLICK,
    STR_EXITMOVE,
    STR_OK,
    STR_CANCEL,
    STR_MONITOR_ITEM_FMT,
    STR_VERTICAL,
    STR_AUDIO_DEFAULT_ITEM,
    STR_AUDIO_ITEM_FMT,
    STR_FILTER_NONE,
    STR_FILTER_SCANLINES,
    STR_FILTER_GREEN,
    STR_FILTER_AMBER,
    STR_FILTER_STRONG,
    STR_FILTER_FADED,
    STR_HOLIDAY_AUTO,
    STR_HOLIDAY_HALLOWEEN,
    STR_HOLIDAY_STPATRICK,
    STR_HOLIDAY_CHRISTMAS,
    STR_HOLIDAY_NEWYEAR,
    STR_NIGHT_AUTO,
    STR_NIGHT_FORCE_NIGHT,
    STR_NIGHT_FORCE_DAY,
    STR_SCALE_AUTO,
    STR_SCALE_FILL,
    STR_SCALE_FIT,
    STR_SCALE_COVER,
    STR_MODE_SINGLE,
    STR_MODE_CLONE,
    STR_MODE_EXTEND,
    NUM_STRINGS
};

static const char *gStrings[NUM_LANGS][NUM_STRINGS] = {
    // LANG_PT
    {
        "Johnny Castaway - Configuracao",
        "Dia da historia (1-11):",
        "Velocidade % (100=normal):",
        "Feriado:",
        "Noite:",
        "Escala:",
        "Monitor:",
        "Modo:",
        "Dispositivo de audio:",
        "Filtro de imagem:",
        "Saltar o ecra de introducao",
        "Idioma:",
        "Volume (0-100):",
        "Sair ao clicar com o rato",
        "Sair ao mexer o rato",
        "OK",
        "Cancelar",
        "%d - %dx%d em (%d,%d)%s",
        "  [vertical]",
        "Predefinicao do sistema",
        "%d - %s",
        "Nenhum",
        "CRT (linhas de varrimento)",
        "Monocromatico verde",
        "Monocromatico ambar",
        "CRT forte (vinheta)",
        "Desbotado / sepia",
        "Automatico",
        "Halloween",
        "St Patrick",
        "Natal",
        "Ano Novo",
        "Automatico",
        "Forcar noite",
        "Forcar dia",
        "Automatico (ajuste inteiro)",
        "Preencher (distorce)",
        "Ajustar (barras pretas)",
        "Cobrir (corta, sem distorcer)",
        "Um monitor",
        "Clone (igual em todos)",
        "Estender (um ecra continuo)",
    },
    // LANG_EN
    {
        "Johnny Castaway - Settings",
        "Story day (1-11):",
        "Speed % (100=normal):",
        "Holiday:",
        "Night:",
        "Scale:",
        "Monitor:",
        "Mode:",
        "Audio device:",
        "Image filter:",
        "Skip the intro screen",
        "Language:",
        "Volume (0-100):",
        "Exit on mouse click",
        "Exit on mouse movement",
        "OK",
        "Cancel",
        "%d - %dx%d at (%d,%d)%s",
        "  [portrait]",
        "System default",
        "%d - %s",
        "None",
        "CRT (scanlines)",
        "Green monochrome",
        "Amber monochrome",
        "Strong CRT (vignette)",
        "Faded / sepia",
        "Automatic",
        "Halloween",
        "St Patrick",
        "Christmas",
        "New Year",
        "Automatic",
        "Force night",
        "Force day",
        "Automatic (integer fit)",
        "Fill (distorts)",
        "Fit (letterbox bars)",
        "Cover (crops, no distortion)",
        "Single monitor",
        "Clone (same on all)",
        "Extend (one continuous screen)",
    },
    // LANG_ES
    {
        "Johnny Castaway - Configuracion",
        "Dia de la historia (1-11):",
        "Velocidad % (100=normal):",
        "Festividad:",
        "Noche:",
        "Escala:",
        "Monitor:",
        "Modo:",
        "Dispositivo de audio:",
        "Filtro de imagen:",
        "Saltar la pantalla de introduccion",
        "Idioma:",
        "Volumen (0-100):",
        "Salir al hacer clic",
        "Salir al mover el raton",
        "OK",
        "Cancelar",
        "%d - %dx%d en (%d,%d)%s",
        "  [vertical]",
        "Predeterminado del sistema",
        "%d - %s",
        "Ninguno",
        "CRT (lineas de escaneo)",
        "Monocromo verde",
        "Monocromo ambar",
        "CRT fuerte (vineta)",
        "Desvanecido / sepia",
        "Automatico",
        "Halloween",
        "San Patricio",
        "Navidad",
        "Ano Nuevo",
        "Automatico",
        "Forzar noche",
        "Forzar dia",
        "Automatico (ajuste entero)",
        "Llenar (distorsiona)",
        "Ajustar (barras negras)",
        "Cubrir (recorta, sin distorsion)",
        "Un monitor",
        "Clon (igual en todos)",
        "Extendido (una pantalla continua)",
    },
};

// Maps the Scale combo's selection index to/from gConfig.scale's actual
// value (0, -1, -2, -3). A gConfig.scale outside this set (an explicit
// integer factor, only settable via the CLI) has no combo entry of its own
// -- it shows as "Automatico" and saving via this dialog normalizes it to
// one of the four presets; the CLI remains the way to pick an exact factor.
static const int gScaleValues[4] = { 0, -1, -2, -3 };

static HWND hDay, hSpeed, hHoliday, hNight, hScale, hMonitor, hMonitorMode,
            hAudioDevice, hFilterCombo, hSkipIntro, hLangCombo, hVolume,
            hExitClick, hExitMove, hOkBtn, hCancelBtn;

static HWND hLblDay, hLblSpeed, hLblHoliday, hLblNight, hLblScale,
            hLblMonitor, hLblMonitorMode, hLblAudioDevice, hLblLanguage,
            hLblVolume, hLblFilter;

static int gCurrentLang = LANG_PT;

static void SetIntText(HWND hwnd, int value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    SetWindowTextA(hwnd, buf);
}

static int GetIntText(HWND hwnd, int fallback)
{
    char buf[16];
    GetWindowTextA(hwnd, buf, sizeof(buf));
    if (buf[0] == '\0')
        return fallback;
    return atoi(buf);
}

static HWND AddLabel(HWND parent, const char *text, int y)
{
    return CreateWindowA("STATIC", text, WS_CHILD | WS_VISIBLE,
        15, y, 300, 18, parent, NULL, NULL, NULL);
}

static HWND AddEdit(HWND parent, int id, int y)
{
    return CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
        200, y - 2, 120, 22, parent, (HMENU)(intptr_t) id, NULL, NULL);
}

// Every enumerated setting (holiday, night, scale, monitor, mode, audio
// device, image filter, language) is a full-width dropdown placed on its
// own row below its label, so long descriptive text (monitor resolutions,
// audio device names) never gets truncated -- no more typing a bare number
// and having to cross-reference a separate list to know what it means.
static HWND AddWideCombo(HWND parent, int id, int y)
{
    return CreateWindowA("COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        15, y, 305, 200, parent, (HMENU)(intptr_t) id, NULL, NULL);
}

static void RebuildHolidayCombo(int lang)
{
    int sel = (int) SendMessageA(hHoliday, CB_GETCURSEL, 0, 0);
    SendMessageA(hHoliday, CB_RESETCONTENT, 0, 0);
    SendMessageA(hHoliday, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_HOLIDAY_AUTO]);
    SendMessageA(hHoliday, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_HOLIDAY_HALLOWEEN]);
    SendMessageA(hHoliday, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_HOLIDAY_STPATRICK]);
    SendMessageA(hHoliday, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_HOLIDAY_CHRISTMAS]);
    SendMessageA(hHoliday, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_HOLIDAY_NEWYEAR]);
    if (sel < 0) sel = gConfig.holiday;
    if (sel < 0 || sel > 4) sel = 0;
    SendMessageA(hHoliday, CB_SETCURSEL, sel, 0);
}

static void RebuildNightCombo(int lang)
{
    int sel = (int) SendMessageA(hNight, CB_GETCURSEL, 0, 0);
    SendMessageA(hNight, CB_RESETCONTENT, 0, 0);
    SendMessageA(hNight, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_NIGHT_AUTO]);
    SendMessageA(hNight, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_NIGHT_FORCE_NIGHT]);
    SendMessageA(hNight, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_NIGHT_FORCE_DAY]);
    if (sel < 0) sel = gConfig.night;
    if (sel < 0 || sel > 2) sel = 0;
    SendMessageA(hNight, CB_SETCURSEL, sel, 0);
}

static void RebuildScaleCombo(int lang)
{
    int sel = (int) SendMessageA(hScale, CB_GETCURSEL, 0, 0);
    SendMessageA(hScale, CB_RESETCONTENT, 0, 0);
    SendMessageA(hScale, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_SCALE_AUTO]);
    SendMessageA(hScale, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_SCALE_FILL]);
    SendMessageA(hScale, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_SCALE_FIT]);
    SendMessageA(hScale, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_SCALE_COVER]);

    if (sel < 0) {
        sel = 0;
        for (int i = 0; i < 4; i++)
            if (gScaleValues[i] == gConfig.scale)
                sel = i;
    }
    if (sel < 0 || sel > 3) sel = 0;
    SendMessageA(hScale, CB_SETCURSEL, sel, 0);
}

static void RebuildMonitorModeCombo(int lang)
{
    int sel = (int) SendMessageA(hMonitorMode, CB_GETCURSEL, 0, 0);
    SendMessageA(hMonitorMode, CB_RESETCONTENT, 0, 0);
    SendMessageA(hMonitorMode, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_MODE_SINGLE]);
    SendMessageA(hMonitorMode, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_MODE_CLONE]);
    SendMessageA(hMonitorMode, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_MODE_EXTEND]);
    if (sel < 0) sel = gConfig.monitorMode;
    if (sel < 0 || sel > 2) sel = 0;
    SendMessageA(hMonitorMode, CB_SETCURSEL, sel, 0);
}

static void RebuildMonitorCombo(int lang)
{
    int sel = (int) SendMessageA(hMonitor, CB_GETCURSEL, 0, 0);
    SendMessageA(hMonitor, CB_RESETCONTENT, 0, 0);

    int numDisplays = SDL_GetNumVideoDisplays();
    for (int i = 0; i < numDisplays; i++) {
        SDL_Rect bounds = {0,0,0,0};
        SDL_GetDisplayBounds(i, &bounds);
        char line[160];
        snprintf(line, sizeof(line), gStrings[lang][STR_MONITOR_ITEM_FMT],
            i, bounds.w, bounds.h, bounds.x, bounds.y,
            (bounds.h > bounds.w) ? gStrings[lang][STR_VERTICAL] : "");
        SendMessageA(hMonitor, CB_ADDSTRING, 0, (LPARAM) line);
    }

    if (sel < 0) sel = gConfig.monitor;
    if (sel < 0 || sel >= numDisplays) sel = 0;
    SendMessageA(hMonitor, CB_SETCURSEL, sel, 0);
}

static void RebuildAudioCombo(int lang)
{
    int sel = (int) SendMessageA(hAudioDevice, CB_GETCURSEL, 0, 0);
    SendMessageA(hAudioDevice, CB_RESETCONTENT, 0, 0);
    SendMessageA(hAudioDevice, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_AUDIO_DEFAULT_ITEM]);

    int numAudioDevices = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < numAudioDevices; i++) {
        char line[160];
        snprintf(line, sizeof(line), gStrings[lang][STR_AUDIO_ITEM_FMT], i, SDL_GetAudioDeviceName(i, 0));
        SendMessageA(hAudioDevice, CB_ADDSTRING, 0, (LPARAM) line);
    }

    if (sel < 0)
        sel = gConfig.audioDevice + 1;   // -1 (default) -> combo index 0
    if (sel < 0 || sel > numAudioDevices) sel = 0;
    SendMessageA(hAudioDevice, CB_SETCURSEL, sel, 0);
}

static void RebuildFilterCombo(int lang)
{
    int sel = (int) SendMessageA(hFilterCombo, CB_GETCURSEL, 0, 0);

    SendMessageA(hFilterCombo, CB_RESETCONTENT, 0, 0);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_NONE]);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_SCANLINES]);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_GREEN]);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_AMBER]);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_STRONG]);
    SendMessageA(hFilterCombo, CB_ADDSTRING, 0, (LPARAM) gStrings[lang][STR_FILTER_FADED]);

    if (sel < 0)
        sel = gConfig.crtFilter;
    if (sel < 0 || sel > 5)
        sel = 0;

    SendMessageA(hFilterCombo, CB_SETCURSEL, sel, 0);
}

static void ApplyLanguage(HWND hwnd, int lang)
{
    if (lang < 0 || lang >= NUM_LANGS)
        lang = LANG_PT;

    gCurrentLang = lang;

    SetWindowTextA(hwnd, gStrings[lang][STR_TITLE]);
    SetWindowTextA(hLblDay, gStrings[lang][STR_DAY]);
    SetWindowTextA(hLblSpeed, gStrings[lang][STR_SPEED]);
    SetWindowTextA(hLblHoliday, gStrings[lang][STR_HOLIDAY]);
    SetWindowTextA(hLblNight, gStrings[lang][STR_NIGHT]);
    SetWindowTextA(hLblScale, gStrings[lang][STR_SCALE]);
    SetWindowTextA(hLblMonitor, gStrings[lang][STR_MONITOR]);
    SetWindowTextA(hLblMonitorMode, gStrings[lang][STR_MONITORMODE]);
    SetWindowTextA(hLblAudioDevice, gStrings[lang][STR_AUDIODEVICE]);
    SetWindowTextA(hLblLanguage, gStrings[lang][STR_LANGUAGE]);
    SetWindowTextA(hLblVolume, gStrings[lang][STR_VOLUME]);
    SetWindowTextA(hLblFilter, gStrings[lang][STR_FILTER_LABEL]);
    SetWindowTextA(hSkipIntro, gStrings[lang][STR_SKIPINTRO]);
    SetWindowTextA(hExitClick, gStrings[lang][STR_EXITCLICK]);
    SetWindowTextA(hExitMove, gStrings[lang][STR_EXITMOVE]);
    SetWindowTextA(hOkBtn, gStrings[lang][STR_OK]);
    SetWindowTextA(hCancelBtn, gStrings[lang][STR_CANCEL]);

    RebuildHolidayCombo(lang);
    RebuildNightCombo(lang);
    RebuildScaleCombo(lang);
    RebuildMonitorCombo(lang);
    RebuildMonitorModeCombo(lang);
    RebuildAudioCombo(lang);
    RebuildFilterCombo(lang);
}

static LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

        case WM_CREATE: {
            int y = 15;

            hLblLanguage = AddLabel(hwnd, "", y);
            hLangCombo = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                200, y - 2, 120, 200, hwnd, (HMENU) ID_LANGUAGE, NULL, NULL);
            SendMessageA(hLangCombo, CB_ADDSTRING, 0, (LPARAM) "Portugues");
            SendMessageA(hLangCombo, CB_ADDSTRING, 0, (LPARAM) "English");
            SendMessageA(hLangCombo, CB_ADDSTRING, 0, (LPARAM) "Espanol");
            SendMessageA(hLangCombo, CB_SETCURSEL, gConfig.language, 0);
            y += 30;

            hLblDay = AddLabel(hwnd, "", y);
            hDay = AddEdit(hwnd, ID_DAY, y);
            y += 26;

            hLblSpeed = AddLabel(hwnd, "", y);
            hSpeed = AddEdit(hwnd, ID_SPEED, y);
            y += 30;

            hLblHoliday = AddLabel(hwnd, "", y);
            y += 20;
            hHoliday = AddWideCombo(hwnd, ID_HOLIDAY, y);
            y += 30;

            hLblNight = AddLabel(hwnd, "", y);
            y += 20;
            hNight = AddWideCombo(hwnd, ID_NIGHT, y);
            y += 30;

            hLblScale = AddLabel(hwnd, "", y);
            y += 20;
            hScale = AddWideCombo(hwnd, ID_SCALE, y);
            y += 30;

            hLblMonitor = AddLabel(hwnd, "", y);
            y += 20;
            hMonitor = AddWideCombo(hwnd, ID_MONITOR, y);
            y += 30;

            hLblMonitorMode = AddLabel(hwnd, "", y);
            y += 20;
            hMonitorMode = AddWideCombo(hwnd, ID_MONITORMODE, y);
            y += 30;

            hLblAudioDevice = AddLabel(hwnd, "", y);
            y += 20;
            hAudioDevice = AddWideCombo(hwnd, ID_AUDIODEVICE, y);
            y += 32;

            hLblVolume = AddLabel(hwnd, "", y);
            hVolume = AddEdit(hwnd, ID_VOLUME, y);
            y += 26;

            hLblFilter = AddLabel(hwnd, "", y);
            y += 20;
            hFilterCombo = AddWideCombo(hwnd, ID_CRTFILTER, y);
            y += 32;

            hSkipIntro = CreateWindowA("BUTTON", "",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                15, y, 300, 22, hwnd, (HMENU) ID_SKIPINTRO, NULL, NULL);
            y += 26;

            hExitClick = CreateWindowA("BUTTON", "",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                15, y, 300, 22, hwnd, (HMENU) ID_EXITCLICK, NULL, NULL);
            y += 26;

            hExitMove = CreateWindowA("BUTTON", "",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                15, y, 300, 22, hwnd, (HMENU) ID_EXITMOVE, NULL, NULL);
            y += 30;

            SetIntText(hDay, gConfig.currentDay);
            SetIntText(hSpeed, gConfig.speed);
            SetIntText(hVolume, gConfig.volume);
            SendMessageA(hSkipIntro, BM_SETCHECK, gConfig.skipIntro ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hExitClick, BM_SETCHECK, gConfig.exitOnClick ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageA(hExitMove, BM_SETCHECK, gConfig.exitOnMouseMove ? BST_CHECKED : BST_UNCHECKED, 0);

            hOkBtn = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                90, y, 80, 26, hwnd, (HMENU) ID_OK, NULL, NULL);
            hCancelBtn = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE,
                180, y, 80, 26, hwnd, (HMENU) ID_CANCEL, NULL, NULL);
            y += 40;

            // Fixed height regardless of monitor/audio device count now --
            // those are dropdowns (Windows scrolls the popup list itself
            // when needed), not always-visible boxes, so unlike before,
            // the window doesn't need to grow with the number of devices.
            {
                RECT rc = {0, 0, 335, y};
                AdjustWindowRectEx(&rc, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME);
                SetWindowPos(hwnd, NULL, 0, 0,
                    rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOMOVE | SWP_NOZORDER);
            }

            ApplyLanguage(hwnd, gConfig.language);

            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_LANGUAGE && HIWORD(wParam) == CBN_SELCHANGE) {
                int sel = (int) SendMessageA(hLangCombo, CB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < NUM_LANGS)
                    ApplyLanguage(hwnd, sel);
                return 0;
            }

            if (LOWORD(wParam) == ID_OK) {
                int day     = GetIntText(hDay, gConfig.currentDay);
                int speed   = GetIntText(hSpeed, gConfig.speed);
                int volume  = GetIntText(hVolume, gConfig.volume);

                int holidaySel = (int) SendMessageA(hHoliday, CB_GETCURSEL, 0, 0);
                int nightSel   = (int) SendMessageA(hNight, CB_GETCURSEL, 0, 0);
                int scaleSel   = (int) SendMessageA(hScale, CB_GETCURSEL, 0, 0);
                int monitorSel = (int) SendMessageA(hMonitor, CB_GETCURSEL, 0, 0);
                int modeSel    = (int) SendMessageA(hMonitorMode, CB_GETCURSEL, 0, 0);
                int audioSel   = (int) SendMessageA(hAudioDevice, CB_GETCURSEL, 0, 0);
                int crtFilter  = (int) SendMessageA(hFilterCombo, CB_GETCURSEL, 0, 0);

                int skipIntro   = (SendMessageA(hSkipIntro, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                int exitClick   = (SendMessageA(hExitClick, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                int exitMove    = (SendMessageA(hExitMove, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

                if (day < 1 || day > 11) day = 1;
                if (speed < 1) speed = 100;
                if (volume < 0) volume = 0;
                if (volume > 100) volume = 100;

                int holiday = (holidaySel >= 0 && holidaySel <= 4) ? holidaySel : 0;
                int night   = (nightSel >= 0 && nightSel <= 2) ? nightSel : 0;
                int scale   = (scaleSel >= 0 && scaleSel <= 3) ? gScaleValues[scaleSel] : 0;
                int monitor = (monitorSel >= 0) ? monitorSel : 0;
                int monitorMode = (modeSel >= 0 && modeSel <= 2) ? modeSel : 0;
                int audioDevice = (audioSel >= 1) ? audioSel - 1 : -1;
                if (crtFilter < 0 || crtFilter > 5) crtFilter = 0;

                cfgFileRead(&gConfig);
                gConfig.currentDay  = day;
                gConfig.speed       = speed;
                gConfig.holiday     = holiday;
                gConfig.night       = night;
                gConfig.scale       = scale;
                gConfig.monitor     = monitor;
                gConfig.monitorMode = monitorMode;
                gConfig.audioDevice = audioDevice;
                gConfig.volume      = volume;
                gConfig.crtFilter   = crtFilter;
                gConfig.skipIntro   = skipIntro;
                gConfig.exitOnClick     = exitClick;
                gConfig.exitOnMouseMove = exitMove;
                gConfig.language    = gCurrentLang;
                cfgFileWrite(&gConfig);

                DestroyWindow(hwnd);
                return 0;
            }
            else if (LOWORD(wParam) == ID_CANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void scrShowConfigDialog(void *parentHwnd)
{
    cfgFileRead(&gConfig);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    HINSTANCE hInst = GetModuleHandleA(NULL);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = ConfigWndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "JCRebornConfigDlg";
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR) IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);

    if (!RegisterClassA(&wc)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "RegisterClassA failed: %lu", GetLastError());
        MessageBoxA(NULL, msg, "jc_reborn debug", MB_OK);
        return;
    }

    HWND hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        "JCRebornConfigDlg",
        "Johnny Castaway",
        WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 660,
        (HWND) parentHwnd, NULL, hInst, NULL
    );

    if (hwnd == NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "CreateWindowExA failed: %lu", GetLastError());
        MessageBoxA(NULL, msg, "jc_reborn debug", MB_OK);
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SDL_Quit();
}

#endif
