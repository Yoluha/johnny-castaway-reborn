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

#include <SDL2/SDL.h>
#include "mytypes.h"
#include "graphics.h"
#include "events.h"
#include "config.h"


static uint32 lastTicks = 0x00ffffff;
static int paused   = 0;
static int maxSpeed = 0;
static int oneFrame = 0;

int evHotKeysEnabled = 0;
int evExitOnInputEnabled = 1;

// Clone/extend mode creates several fullscreen windows back-to-back right
// at startup, and Windows can synthesize a mouse-move event as focus shifts
// between them while that happens -- with no grace period, that alone was
// enough to trip "exit on mouse move" a fraction of a second after launch,
// making the whole session look like it "doesn't work" (it started, then
// immediately vanished). Ignore dismiss-worthy input for a brief window
// right after startup so residual/synthetic events from window creation
// can't be mistaken for the user actually touching the mouse.
#define STARTUP_GRACE_MS 1200
static uint32 startupTicks = 0;


static void eventsProcessEvents()
{
    SDL_Event event;
    int inStartupGrace = (SDL_GetTicks() - startupTicks) < STARTUP_GRACE_MS;

    while (SDL_PollEvent(&event)) {

        switch(event.type) {

            case SDL_KEYDOWN:

                if (evHotKeysEnabled) {

                    switch (event.key.keysym.sym) {

                        case SDLK_SPACE:
                            paused = !paused;
                            break;

                        case SDLK_m:
                            maxSpeed = !maxSpeed;
                            break;

                        case SDLK_RETURN:
                            if (event.key.keysym.mod & KMOD_LALT) {
                                grToggleFullScreen();
                                oneFrame = 1;   // to redraw the window // TODO
                            }
                            else {
                                oneFrame = 1;
                            }
                            break;

                        case SDLK_ESCAPE:
                            graphicsEnd();
                            exit(255);
                            break;
                    }
                }
                else if (evExitOnInputEnabled && !inStartupGrace) {
                    // Normal behaviour : no hot keys, the screen saver
                    // terminates if any key is pressed
                    graphicsEnd();
                    exit(255);
                }
                break;

            case SDL_MOUSEMOTION:
                // A real screensaver normally exits on mouse movement too,
                // not just keyboard - regardless of the hotkeys setting
                // above (those are for pause/step/fullscreen-toggle
                // convenience while testing, not for suppressing the
                // dismiss gesture). Preview mode turns this off entirely
                // via evExitOnInputEnabled, since it's meant to keep
                // running while the surrounding dialog is used; on top of
                // that, the user can independently disable "exit on move" (e.g.
                // to avoid an unwanted dismiss from a twitchy/wireless
                // mouse) while still keeping "exit on click". inStartupGrace
                // additionally swallows the first instant after launch, so
                // a synthetic move event from clone/extend mode creating
                // several fullscreen windows back-to-back can't dismiss a
                // session the user hasn't actually touched yet.
                if (evExitOnInputEnabled && gConfig.exitOnMouseMove && !inStartupGrace) {
                    graphicsEnd();
                    exit(255);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (evExitOnInputEnabled && gConfig.exitOnClick && !inStartupGrace) {
                    graphicsEnd();
                    exit(255);
                }
                break;

            case SDL_WINDOWEVENT:
                grRefreshDisplay();
                break;

            case SDL_QUIT:
                graphicsEnd();
                exit(255);
                break;
        }
    }
}


void eventsInit()
{
    lastTicks = SDL_GetTicks();
    startupTicks = SDL_GetTicks();
}


void eventsWaitTick(uint16 delay)
{
    int speed = gConfig.speed > 0 ? gConfig.speed : 100;

    delay = (uint16) ((uint32) delay * 20 * 100 / speed);
    oneFrame = 0;

    eventsProcessEvents();

    while ((paused && !oneFrame)
            || (!maxSpeed && (SDL_GetTicks() - lastTicks < delay))) {
        SDL_Delay(5);
        eventsProcessEvents();
    }

    lastTicks = SDL_GetTicks();
}

