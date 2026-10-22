#pragma once

// Focus navigation and placeholder for the future focus preset feature.

#include "config.h"
#include "globals.h"
#include "ui_main_screen.h"
#include "ui_presets_screen.h"
#include "ui_focus_presets.h"

static const Rect FOCUS_MENU_MANUAL =   {8,  33, 304, 40};
static const Rect FOCUS_MENU_PULL =     {8,  76, 304, 40};
static const Rect FOCUS_MENU_PRESETS =  {8, 119, 304, 40};
static const Rect FOCUS_MENU_BACK =     {6, 167, 118, 34};

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
