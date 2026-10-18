#pragma once

// Focus pull macro pages and non-blocking playback state machine.

#include "config.h"
#include "globals.h"
#include "ble_protocol.h"
#include "ui_main_screen.h"
static void openScreen(uint8_t next);  // defined by the platform-specific application entry point

static const Rect MACRO_SETTINGS_BUTTON = {6, 160, 112, 36};
static const Rect MACRO_REVERSE_BUTTON = {124, 160, 90, 36};
static const Rect MACRO_START_BUTTON = {220, 160, 94, 36};
static const Rect MACRO_STOP_BUTTON = {6, 160, 90, 36};
static const Rect MACRO_BACK_BUTTON = {6, 188, 308, 18};
static const Rect MACRO_PROGRESS = {82, 131, 226, 14};
static const Rect MACRO_BACK = {8, 116, 66, 25};
static void drawFocusScreen();

static uint8_t focusPercent(uint16_t value) { return (uint8_t)((value * 100UL + FOCUS_MAX / 2) / FOCUS_MAX); }

static void drawMacroButton(const Rect& r, const char* label, bool enabled, bool filled = false) {
    const uint16_t color = enabled ? (filled ? UI_CONNECT : GREY) : DARK_GREY;
    if (filled && enabled) M5.Lcd.fillRoundRect(r.x, r.y, r.w, r.h, 5, color);
    else M5.Lcd.drawRoundRect(r.x, r.y, r.w, r.h, 5, color);
    M5.Lcd.setTextColor(enabled ? WHITE : GREY, filled && enabled ? color : BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString(label, r.x + r.w / 2, r.y + r.h / 2, 2);
}

static void drawFocusMacroScreen() {
    drawSubHeader("FOCUS PULL");
    char line[40];
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextColor(GREY, BLACK);
    M5.Lcd.drawString("START", 18, 43, 2);
    M5.Lcd.drawString("END", 18, 72, 2);
    M5.Lcd.drawString("PULL TIME", 18, 101, 2);
    M5.Lcd.setTextColor(WHITE, BLACK);
    snprintf(line, sizeof(line), "%u%%", focusPercent(macroFocusStart)); M5.Lcd.drawString(line, 176, 43, 2);
    snprintf(line, sizeof(line), "%u%%", focusPercent(macroFocusEnd)); M5.Lcd.drawString(line, 176, 72, 2);
    snprintf(line, sizeof(line), "%lu:%02lu", (unsigned long)(macroFocusDurationMs / 60000), (unsigned long)(macroFocusDurationMs / 1000 % 60));
    M5.Lcd.drawString(line, 176, 101, 2);
    M5.Lcd.drawRect(MACRO_PROGRESS.x, MACRO_PROGRESS.y, MACRO_PROGRESS.w, MACRO_PROGRESS.h, GREY);
    uint32_t progress = 0;
    if (focusMacroPhase == 3) {
        const uint32_t elapsedMs = millis() - focusMacroPhaseStartedMs;
        progress = elapsedMs >= macroFocusDurationMs ? 1000UL : elapsedMs * 1000UL / macroFocusDurationMs;
    }
    else if (focusMacroPhase >= 4 || (focusMacroPhase == 0 && cameraFocus == macroFocusEnd)) progress = 1000;
    M5.Lcd.fillRect(MACRO_PROGRESS.x + 2, MACRO_PROGRESS.y + 2, MACRO_PROGRESS.w - 4, MACRO_PROGRESS.h - 4, BLACK);
    M5.Lcd.fillRect(MACRO_PROGRESS.x + 2, MACRO_PROGRESS.y + 2, (MACRO_PROGRESS.w - 4) * progress / 1000, MACRO_PROGRESS.h - 4, UI_CONNECT);
    drawMacroButton(MACRO_BACK, "BACK", focusMacroPhase == 0);
    if (focusMacroPhase == 0) {
        drawMacroButton(MACRO_SETTINGS_BUTTON, "SETTINGS", true);
        drawMacroButton(MACRO_REVERSE_BUTTON, "REVERSE", true);
    }
    drawMacroButton(MACRO_START_BUTTON, focusMacroPhase == 0 ? "START" : focusMacroPaused ? "RESUME" : "PAUSE", cameraConnected, true);
    if (focusMacroPhase != 0) {
        drawMacroButton(MACRO_STOP_BUTTON, "STOP", true);
        M5.Lcd.setTextColor(focusMacroPaused ? UI_SEL : UI_GREEN, BLACK);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.drawString(focusMacroPaused ? "PAUSED" : "RUNNING", 160, 149, 1);
    }
    drawStatus();
}

static uint8_t macroEditField = 0; // start, end, duration
static const Rect MACRO_MINUS = {36, 143, 72, 44};
static const Rect MACRO_PLUS = {212, 143, 72, 44};
static const Rect MACRO_TAKE = {108, 143, 104, 44};

static void drawFocusMacroSettings() {
    drawSubHeader("MACRO SETTINGS");
    static const char* labels[] = {"START FOCUS", "END FOCUS", "PULL TIME"};
    char value[24];
    M5.Lcd.setTextDatum(MC_DATUM);
    for (uint8_t i = 0; i < 3; ++i) {
        const int y = 42 + i * 34;
        M5.Lcd.setTextColor(i == macroEditField ? UI_SEL : GREY, BLACK);
        M5.Lcd.drawString(labels[i], 78, y, 2);
    }
    if (macroEditField == 0) snprintf(value, sizeof(value), "%u%%", focusPercent(macroFocusStart));
    else if (macroEditField == 1) snprintf(value, sizeof(value), "%u%%", focusPercent(macroFocusEnd));
    else snprintf(value, sizeof(value), "%lu:%02lu", (unsigned long)(macroFocusDurationMs / 60000), (unsigned long)(macroFocusDurationMs / 1000 % 60));
    M5.Lcd.setTextColor(WHITE, BLACK); M5.Lcd.drawString(value, 240, 42 + macroEditField * 34, 2);
    drawStepperPair(MACRO_MINUS, MACRO_PLUS, 18, 4);
    drawMacroButton(MACRO_TAKE, "TAKE", cameraConnected);
    drawOutlineButton(MACRO_BACK_BUTTON, "DONE", 4);
    M5.Lcd.setTextDatum(MC_DATUM); M5.Lcd.setTextColor(GREY, BLACK);
    M5.Lcd.drawString("Select a row, then use - / +", 160, 124, 1);
    drawStatus();
}

static void stopFocusMacro(bool stopRecording) {
    if (stopRecording && focusMacroRecording && cameraConnected) sendPacket(TRANSPORT_PACKET, 0, 1);
    focusMacroRecording = false;
    focusMacroPaused = false;
    focusMacroPhase = 0;
    focusMacroDirty = true;
}

static bool startFocusMacro() {
    if (!cameraConnected) { setStatus("Connect camera to run macro"); return false; }
    if (transportMode == 2) { setStatus("Stop recording before macro"); return false; }
    if (!sendFocus(macroFocusStart)) { setStatus("Could not set start focus"); return false; }
    if (!sendPacket(TRANSPORT_PACKET, 2, 1)) { setStatus("Could not start recording"); return false; }
    focusMacroRecording = true;
    focusMacroPaused = false;
    focusMacroPhase = 2;
    focusMacroPhaseStartedMs = millis();
    setStatus("Focus pull macro started");
    focusMacroDirty = true;
    return true;
}

static void updateFocusMacro() {
    if (focusMacroPhase == 0 || focusMacroPaused) return;
    if (!cameraConnected) { stopFocusMacro(false); setStatus("Macro stopped: camera disconnected"); return; }
    const uint32_t now = millis();
    if (now - focusMacroLastUpdateMs < 100) return;
    focusMacroLastUpdateMs = now;
    const uint32_t elapsed = now - focusMacroPhaseStartedMs;
    if (focusMacroPhase == 2 && elapsed >= 2000) {
        focusMacroPhase = 3; focusMacroPhaseStartedMs = now;
        sendFocus(macroFocusStart); focusMacroDirty = true;
    } else if (focusMacroPhase == 3) {
        const uint32_t done = min(elapsed, macroFocusDurationMs);
        const int32_t delta = (int32_t)macroFocusEnd - macroFocusStart;
        sendFocus((int32_t)macroFocusStart + delta * (int32_t)done / (int32_t)macroFocusDurationMs);
        if (screen == SCREEN_FOCUS_MACRO) focusMacroDirty = true;
        if (done >= macroFocusDurationMs) { focusMacroPhase = 4; focusMacroPhaseStartedMs = now; focusMacroDirty = true; }
    } else if (focusMacroPhase == 4 && elapsed >= 2000) {
        stopFocusMacro(true);
        setStatus("Focus pull macro complete");
    }
}

static void handleFocusMacroTouch(int x, int y) {
    if (inside(x, y, MACRO_BACK) && focusMacroPhase == 0) { screen = SCREEN_FOCUS; drawFocusScreen(); }
    else if (inside(x, y, MACRO_SETTINGS_BUTTON) && focusMacroPhase == 0) { screen = SCREEN_FOCUS_MACRO_SETTINGS; drawFocusMacroSettings(); }
    else if (inside(x, y, MACRO_REVERSE_BUTTON) && focusMacroPhase == 0) {
        const uint16_t temp = macroFocusStart; macroFocusStart = macroFocusEnd; macroFocusEnd = temp; drawFocusMacroScreen();
    } else if (inside(x, y, MACRO_START_BUTTON)) {
        if (focusMacroPhase == 0) startFocusMacro();
        else if (focusMacroPaused) { focusMacroPhaseStartedMs += millis() - focusMacroPausedAtMs; focusMacroPaused = false; }
        else { focusMacroPaused = true; focusMacroPausedAtMs = millis(); }
        drawFocusMacroScreen();
    } else if (focusMacroPhase != 0 && inside(x, y, MACRO_STOP_BUTTON)) {
        stopFocusMacro(true); setStatus("Focus pull macro stopped"); drawFocusMacroScreen();
    } else if (y < 28 && x < 80) { stopFocusMacro(true); openScreen(SCREEN_MAIN); }
}

static void handleFocusMacroSettingsTouch(int x, int y) {
    if (inside(x, y, MACRO_BACK_BUTTON)) { screen = SCREEN_FOCUS_MACRO; drawFocusMacroScreen(); return; }
    if (y >= 34 && y < 68) macroEditField = 0;
    else if (y >= 68 && y < 102) macroEditField = 1;
    else if (y >= 102 && y < 136) macroEditField = 2;
    else if (inside(x, y, MACRO_TAKE) && cameraConnected && cameraFocus >= 0 && macroEditField < 2) {
        if (macroEditField == 0) macroFocusStart = cameraFocus; else macroFocusEnd = cameraFocus;
    } else if (inside(x, y, MACRO_MINUS) || inside(x, y, MACRO_PLUS)) {
        const int direction = inside(x, y, MACRO_PLUS) ? 1 : -1;
        if (macroEditField == 0) macroFocusStart = (uint16_t)((constrain((int)focusPercent(macroFocusStart) + direction, 0, 100) * FOCUS_MAX + 50) / 100);
        else if (macroEditField == 1) macroFocusEnd = (uint16_t)((constrain((int)focusPercent(macroFocusEnd) + direction, 0, 100) * FOCUS_MAX + 50) / 100);
        else macroFocusDurationMs = (uint32_t)constrain((int32_t)macroFocusDurationMs + direction * 1000, 1000, 600000);
    }
    drawFocusMacroSettings();
}
