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

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "mytypes.h"
#include "utils.h"
#include "sound.h"
#include "config.h"


// Always-on diagnostic log (unlike debugMsg(), which prints to a console
// that doesn't exist when launched from a desktop/screensaver shortcut).
// Written once at startup so a "volume doesn't seem to do anything"
// report can be checked against what the engine actually resolved, instead
// of guessing.
static void soundLogInit(int volume, SDL_AudioFormat format, const char *deviceName)
{
#ifdef _WIN32
    char *appData = getenv("APPDATA");
    if (appData == NULL || !strlen(appData))
        return;

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s\\JohnnyCastawayReborn", appData);
    CreateDirectoryA(dir, NULL);

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\audio.log", dir);

    FILE *f = fopen(path, "a");
    if (f == NULL)
        return;

    fprintf(f, "soundInit: gConfig.volume=%d format=0x%04x device=%s\n",
        volume, format, deviceName ? deviceName : "(system default)");
    fclose(f);
#endif
}


#define NUM_OF_SOUNDS  25


struct TSound {
    uint32  length;
    uint8   *data;
};


int soundDisabled = 0;


static struct TSound sounds[NUM_OF_SOUNDS];
static struct TSound *currentSound;

static uint8  *currentPtr;
static uint32 currentRemaining;

static SDL_AudioDeviceID audioDevId = 0;
static SDL_AudioFormat gAudioFormat = AUDIO_U8;
static uint8 gAudioSilence = 128;   // correct "silence" byte value depends on format (0 for signed, 128 for U8)


void soundPrintDevices()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("\n Nao foi possivel consultar dispositivos de audio: %s\n", SDL_GetError());
        return;
    }

    int numDevices = SDL_GetNumAudioDevices(0);
    printf("\n Dispositivos de audio (reproducao) disponiveis:\n\n");

    for (int i = 0; i < numDevices; i++) {
        printf("   audiodevice=%d  ->  %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }

    printf("\n Usa: jc_reborn audiodevice=<indice> [window|...]\n\n");

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}


static void soundCallback(void *userdata, uint8 *stream, int rqdLen)
{
    // Start from silence and mix the sample in at the configured volume,
    // instead of memcpy'ing it straight into the output buffer.
    memset(stream, gAudioSilence, rqdLen);

    int volume = gConfig.volume;
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    int sdlVolume = volume * SDL_MIX_MAXVOLUME / 100;

    if (currentRemaining > rqdLen) {
        SDL_MixAudioFormat(stream, currentPtr, gAudioFormat, rqdLen, sdlVolume);
        currentPtr += rqdLen;
        currentRemaining -= rqdLen;
    }
    else if (currentRemaining > 0) {
        SDL_MixAudioFormat(stream, currentPtr, gAudioFormat, currentRemaining, sdlVolume);
        currentRemaining = 0;
        //SDL_PauseAudio(1);
    }
}


void soundInit()
{
    if (soundDisabled)
        return;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        debugMsg("SDL init audio error: %s", SDL_GetError());
        soundDisabled = 1;
        return;
    }

    SDL_AudioSpec audioSpec;

    for (int i=0; i < NUM_OF_SOUNDS; i++) {

        char filename[20];

        sprintf(filename, "sound%d.wav", i);

        if (SDL_LoadWAV(filename, &audioSpec, &sounds[i].data, &sounds[i].length) == NULL) {
            sounds[i].data   = NULL;
            sounds[i].length = 0;
            debugMsg("SDL_LoadWAV() warning: %s", SDL_GetError());
        }
    }

    audioSpec.callback = soundCallback;
    audioSpec.userdata = NULL;
    audioSpec.samples  = 1024;

    // No SDL_AUDIO_ALLOW_*_CHANGE flag is passed below, so the device is
    // forced to match audioSpec exactly -- safe to derive the mixing
    // format/silence value from it now, before opening the device.
    gAudioFormat  = audioSpec.format;
    gAudioSilence = SDL_AUDIO_ISSIGNED(gAudioFormat) ? 0 : 0x80;

    const char *deviceName = NULL;
    int numDevices = SDL_GetNumAudioDevices(0);
    if (gConfig.audioDevice >= 0 && gConfig.audioDevice < numDevices)
        deviceName = SDL_GetAudioDeviceName(gConfig.audioDevice, 0);

    audioDevId = SDL_OpenAudioDevice(deviceName, 0, &audioSpec, NULL, 0);

    if (audioDevId == 0) {
        debugMsg("SDL_OpenAudioDevice() error: %s", SDL_GetError());
        soundLogInit(-1, gAudioFormat, deviceName);   // -1 = never got this far, device open failed
        soundDisabled = 1;
        return;
    }

    soundLogInit(gConfig.volume, gAudioFormat, deviceName);

    currentRemaining = 0;
    SDL_PauseAudioDevice(audioDevId, 0);
}


void soundEnd()
{
    if (soundDisabled)
        return;

    SDL_CloseAudioDevice(audioDevId);

    for (int i=0; i < NUM_OF_SOUNDS; i++)
        if (sounds[i].data != NULL)
            SDL_FreeWAV(sounds[i].data);
}


void soundPlay(int nb)
{
    if (soundDisabled)
        return;

    if (nb < 0 || NUM_OF_SOUNDS <= nb) {
        debugMsg("soundPlay(): wrong sound sample index #%d", nb);
        return;
    }

    if (sounds[nb].length) {

        SDL_LockAudioDevice(audioDevId);

        currentSound     = &sounds[nb];
        currentPtr       = currentSound->data;
        currentRemaining = currentSound->length;

        SDL_UnlockAudioDevice(audioDevId);
    }
    else {
        debugMsg("Non-existent sound sample #%d", nb);
    }
}

