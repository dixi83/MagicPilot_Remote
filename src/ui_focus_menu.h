#pragma once

// Focus navigation to manual focus, focus pull, and saved focus presets.

#include "config.h"
#include "globals.h"
#include "ui_main_screen.h"
#include "ui_presets_screen.h"
#include "ui_focus_presets.h"

static const Rect FOCUS_MENU_MANUAL =   {8,  33, 304, 40};
static const Rect FOCUS_MENU_PULL =     {8,  76, 304, 40};
static const Rect FOCUS_MENU_PRESETS =  {8, 119, 304, 40};
static const Rect FOCUS_MENU_BACK =     {6, 167, 118, 34};

// Each item opens its own page; BACK returns to the main camera controls.
static void drawFocusMenuScreen() {
    drawSubHeader("FOCUS MENU");
    drawOutlineButton(FOCUS_MENU_MANUAL, "MANUAL FOCUS", 5);
    drawOutlineButton(FOCUS_MENU_PULL, "FOCUS PULL", 5);
    drawOutlineButton(FOCUS_MENU_PRESETS, "FOCUS PRESETS", 5);
    drawOutlineButton(FOCUS_MENU_BACK, "BACK", 4);
    drawStatus();
}

static void handleFocusMenuTouch(int x, int y) {
    if (inside(x, y, FOCUS_MENU_MANUAL)) openScreen(SCREEN_FOCUS);
    else if (inside(x, y, FOCUS_MENU_PULL)) openScreen(SCREEN_FOCUS_MACRO);
    else if (inside(x, y, FOCUS_MENU_PRESETS)) openScreen(SCREEN_FOCUS_PRESETS);
    else if (inside(x, y, FOCUS_MENU_BACK)) openScreen(SCREEN_MAIN);
}
