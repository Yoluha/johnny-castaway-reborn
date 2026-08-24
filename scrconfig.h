#ifndef SCRCONFIG_H
#define SCRCONFIG_H

// Shows the native Windows Screensaver configuration dialog.
// parentHwnd may be NULL. No-op on non-Windows builds.
void scrShowConfigDialog(void *parentHwnd);

#endif
