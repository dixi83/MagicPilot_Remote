#pragma once

// Focus pull editor and non-blocking macro playback state machine.

#include "config.h"
#include "globals.h"
#include "ble_protocol.h"
#include "ui_main_screen.h"
static void openScreen(uint8_t next);  // defined by the platform-specific application entry point

// Screen layout: editable values and +/- controls are on the left; transport controls use the right column.
static const Rect MACRO_START_BUTTON = {255, 79, 63, 63};
static const Rect MACRO_STOP_BUTTON = {255, 148, 63, 54};
// Expand hit targets into the unused right-column space around the visible controls.
static const Rect MACRO_START_HIT = {244, 55, 76, 92};
static const Rect MACRO_STOP_HIT = {244, 148, 76, 56};
static const Rect MACRO_TAKE_BUTTON = {168, 55, 76, 71};
static const Rect MACRO_PROGRESS = {6, 36, 314, 12};
static const Rect MACRO_FIELDS[3] = {{6, 55, 76, 34}, {87, 55, 76, 34}, {6, 92, 76, 34}}; //{168, 56, 76, 36}};
static const Rect MACRO_MINUS = {131, 130, 54, 71}; //  {84, 110, 74, 92};
static const Rect MACRO_PLUS = {190, 130, 54, 71};   // {162, 110, 74, 92};
static const Rect MACRO_REVERSE_BUTTON = {6, 130, 118, 34};
static const Rect MACRO_BACK = {6, 167, 118, 34};
static void drawFocusScreen();

static uint8_t focusPercent(uint16_t value) { return (uint8_t)((value * 100UL + FOCUS_MAX / 2) / FOCUS_MAX); }
static uint8_t macroEditField = 0; // selected start focus, end focus, or pull duration field
static void drawMacroButton(const Rect& r, const char* label, bool enabled, bool filled = false);
static void drawMacroField(uint8_t field);
static int8_t renderedMacroControls = -1;
static int16_t renderedMacroProgress = -1;

// The pad shows play while idle, pause while running, and play again while paused.
static void drawMacroRecordButton() {
    const int centerX = MACRO_START_BUTTON.x + MACRO_START_BUTTON.w / 2;
    const int centerY = MACRO_START_BUTTON.y + MACRO_START_BUTTON.h / 2;
    M5.Lcd.fillRect(MACRO_START_BUTTON.x, MACRO_START_BUTTON.y, MACRO_START_BUTTON.w, MACRO_START_BUTTON.h, BLACK);
    M5.Lcd.drawCircle(centerX, centerY, 30, WHITE);
    M5.Lcd.drawCircle(centerX, centerY, 29, WHITE);
    M5.Lcd.fillCircle(centerX, centerY, 15, RED);
    if (focusMacroRecording && focusMacroPaused) {
        M5.Lcd.fillTriangle(centerX - 5, centerY - 9, centerX - 5, centerY + 9, centerX + 9, centerY, WHITE);
    } else if (focusMacroRecording) {
        M5.Lcd.fillRect(centerX - 7, centerY - 8, 5, 16, WHITE);
        M5.Lcd.fillRect(centerX + 2, centerY - 8, 5, 16, WHITE);
    }
}

static void drawFocusMacroDynamic() {
    // Phase 3 is the timed pull; phase 4 is the two-second settle before recording stops.
    uint32_t progress = 0;
    if (focusMacroPhase == 3) {
        const uint32_t elapsedMs = millis() - focusMacroPhaseStartedMs;
        progress = elapsedMs >= macroFocusDurationMs ? 1000UL : elapsedMs * 1000UL / macroFocusDurationMs;
    } else if (focusMacroPhase >= 4 || (focusMacroPhase == 0 && cameraFocus == macroFocusEnd)) progress = 1000;
    const int innerWidth = MACRO_PROGRESS.w - 4;
    const int nextWidth = innerWidth * progress / 1000;
    if (renderedMacroProgress < 0) {
        M5.Lcd.fillRect(MACRO_PROGRESS.x + 2, MACRO_PROGRESS.y + 2, innerWidth, MACRO_PROGRESS.h - 4, BLACK);
        if (nextWidth > 0) M5.Lcd.fillRect(MACRO_PROGRESS.x + 2, MACRO_PROGRESS.y + 2, nextWidth, MACRO_PROGRESS.h - 4, UI_CONNECT);
    } else if (nextWidth > renderedMacroProgress) {
        M5.Lcd.fillRect(MACRO_PROGRESS.x + 2 + renderedMacroProgress, MACRO_PROGRESS.y + 2,
                        nextWidth - renderedMacroProgress, MACRO_PROGRESS.h - 4, UI_CONNECT);
    } else if (nextWidth < renderedMacroProgress) {
        M5.Lcd.fillRect(MACRO_PROGRESS.x + 2 + nextWidth, MACRO_PROGRESS.y + 2,
                        renderedMacroProgress - nextWidth, MACRO_PROGRESS.h - 4, BLACK);
    }
    renderedMacroProgress = nextWidth;

    // Cache the control mode so animation ticks redraw controls only when state changes.
    const int8_t controls = focusMacroPhase == 0 ? 0 : focusMacroPaused ? 2 : 1;
    if (controls != renderedMacroControls) {
        M5.Lcd.fillRect(MACRO_STOP_BUTTON.x, MACRO_STOP_BUTTON.y, MACRO_STOP_BUTTON.w, MACRO_STOP_BUTTON.h, BLACK);
        M5.Lcd.fillRect(MACRO_REVERSE_BUTTON.x, MACRO_REVERSE_BUTTON.y, MACRO_REVERSE_BUTTON.w, MACRO_REVERSE_BUTTON.h, BLACK);
        M5.Lcd.fillRect(MACRO_BACK.x, MACRO_BACK.y, MACRO_BACK.w, MACRO_BACK.h, BLACK);
        drawMacroRecordButton();
        drawMacroButton(MACRO_STOP_BUTTON, "STOP", focusMacroPhase != 0);
        drawMacroButton(MACRO_REVERSE_BUTTON, "REVERSE", controls == 0);
        drawMacroButton(MACRO_BACK, "BACK", controls == 0);
        renderedMacroControls = controls;
    }
}

static void drawMacroButton(const Rect& r, const char* label, bool enabled, bool filled) {
    const uint16_t color = enabled ? (filled ? UI_CONNECT : GREY) : DARK_GREY;
    if (filled && enabled) M5.Lcd.fillRoundRect(r.x, r.y, r.w, r.h, 5, color);
    else M5.Lcd.drawRoundRect(r.x, r.y, r.w, r.h, 5, color);
    M5.Lcd.setTextColor(enabled ? WHITE : GREY, filled && enabled ? color : BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);

    char text[64];
    snprintf(text, sizeof(text), "%s", label);
    // <BR> separates lines without relying on the display font's newline handling.
    char* lines[4] = {text, nullptr, nullptr, nullptr};
    uint8_t lineCount = 1;
    char* breakAt = text;
    while (lineCount < 4 && (breakAt = strstr(breakAt, "<BR>")) != nullptr) {
        *breakAt = '\0';
        breakAt += 4;
        lines[lineCount++] = breakAt;
    }
    const int lineHeight = lineCount > 1 ? 10 : 0;
    const int firstLineY = r.y + r.h / 2 - ((lineCount - 1) * lineHeight) / 2;
    for (uint8_t line = 0; line < lineCount; ++line) {
        M5.Lcd.drawString(lines[line], r.x + r.w / 2, firstLineY + line * lineHeight, lineCount > 1 ? 1 : 2);
    }
}

static void drawFocusMacroScreen() {
    drawSubHeader("FOCUS PULL");
    drawMacroField(0);
    drawMacroField(1);
    drawMacroField(2);
    drawMacroButton(MACRO_TAKE_BUTTON, "TAKE<BR>CURRENT<BR>FOCUS", cameraConnected && cameraFocus >= 0 && macroEditField < 2);
    M5.Lcd.drawRect(MACRO_PROGRESS.x, MACRO_PROGRESS.y, MACRO_PROGRESS.w, MACRO_PROGRESS.h, GREY);
    drawStepperPair(MACRO_MINUS, MACRO_PLUS, 24, 5);
    drawMacroButton(MACRO_REVERSE_BUTTON, "REVERSE", focusMacroPhase == 0);
    drawMacroButton(MACRO_BACK, "BACK", focusMacroPhase == 0);
    // Invalidate partial-draw caches after painting the whole page.
    renderedMacroControls = -1;
    renderedMacroProgress = -1;
    drawFocusMacroDynamic();
    drawStatus();
}

static void drawMacroField(uint8_t field) {
    char label[12];
    char value[16];
    if (field == 0) {
        snprintf(label, sizeof(label), "START");
        snprintf(value, sizeof(value), "%u%%", focusPercent(macroFocusStart));
    } else if (field == 1) {
        snprintf(label, sizeof(label), "END");
        snprintf(value, sizeof(value), "%u%%", focusPercent(macroFocusEnd));
    } else {
        snprintf(label, sizeof(label), "PULL TIME");
        snprintf(value, sizeof(value), "%lu:%02lu", (unsigned long)(macroFocusDurationMs / 60000), (unsigned long)(macroFocusDurationMs / 1000 % 60));
    }
    const Rect& box = MACRO_FIELDS[field];
    drawCell(box.x, box.y, box.w, box.h, label, value, macroEditField == field);
}

static void stopFocusMacro(bool stopRecording) {
    // Used for both a user stop and the normal end of the macro sequence.
    if (stopRecording && focusMacroRecording && cameraConnected) sendPacket(TRANSPORT_PACKET, 0, 1);
    focusMacroRecording = false;
    focusMacroPaused = false;
    focusMacroPhase = 0;
    focusMacroDirty = true;
}

static bool startFocusMacro() {
    // Check camera and recording state before changing focus or starting a recording.
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
    // Called from the main loop so waits and focus moves never block touch or BLE handling.
    if (focusMacroPhase == 0 || focusMacroPaused) return;
    if (!cameraConnected) { stopFocusMacro(false); setStatus("Macro stopped: camera disconnected"); return; }
    const uint32_t now = millis();
    if (now - focusMacroLastUpdateMs < 100) return;
    focusMacroLastUpdateMs = now;
    const uint32_t elapsed = now - focusMacroPhaseStartedMs;
    if (focusMacroPhase == 2 && elapsed >= 2000) {
        // Hold the start focus for two seconds after recording begins.
        focusMacroPhase = 3; focusMacroPhaseStartedMs = now;
        sendFocus(macroFocusStart); focusMacroDirty = true;
    } else if (focusMacroPhase == 3) {
        // Interpolate focus from start to end over the configured duration.
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
    if (inside(x, y, MACRO_BACK) && focusMacroPhase == 0) {
        openScreen(SCREEN_FOCUS_MENU);
    } else if (inside(x, y, MACRO_REVERSE_BUTTON) && focusMacroPhase == 0) {
        const uint16_t temp = macroFocusStart; macroFocusStart = macroFocusEnd; macroFocusEnd = temp;
        saveFocusMacroSettings();
        drawMacroField(0); drawMacroField(1);
        if (cameraConnected && !sendFocus(macroFocusStart)) setStatus("Focus update failed");
    } else if (inside(x, y, MACRO_START_HIT)) {
        if (focusMacroPhase == 0) startFocusMacro();
        else if (focusMacroPaused) { focusMacroPhaseStartedMs += millis() - focusMacroPausedAtMs; focusMacroPaused = false; }
        else { focusMacroPaused = true; focusMacroPausedAtMs = millis(); }
        drawFocusMacroDynamic();
    } else if (focusMacroPhase != 0 && inside(x, y, MACRO_STOP_HIT)) {
        stopFocusMacro(true); setStatus("Focus pull macro stopped"); drawFocusMacroDynamic();
    } else if (focusMacroPhase == 0 && inside(x, y, MACRO_TAKE_BUTTON) && cameraConnected && cameraFocus >= 0 && macroEditField < 2) {
        if (macroEditField == 0) macroFocusStart = cameraFocus; else macroFocusEnd = cameraFocus;
        saveFocusMacroSettings();
        drawMacroField(macroEditField);
    } else if (focusMacroPhase == 0 && (inside(x, y, MACRO_MINUS) || inside(x, y, MACRO_PLUS))) {
        // Focus values step by one percent; pull duration steps by one second.
        const int direction = inside(x, y, MACRO_PLUS) ? 1 : -1;
        if (macroEditField == 0) {
            macroFocusStart = (uint16_t)((constrain((int)focusPercent(macroFocusStart) + direction, 0, 100) * FOCUS_MAX + 50) / 100);
            if (cameraConnected && !sendFocus(macroFocusStart)) setStatus("Focus update failed");
        } else if (macroEditField == 1) {
            macroFocusEnd = (uint16_t)((constrain((int)focusPercent(macroFocusEnd) + direction, 0, 100) * FOCUS_MAX + 50) / 100);
            if (cameraConnected && !sendFocus(macroFocusEnd)) setStatus("Focus update failed");
        } else {
            macroFocusDurationMs = (uint32_t)constrain((int32_t)macroFocusDurationMs + direction * 1000, 1000, 600000);
        }
        saveFocusMacroSettings();
        drawMacroField(macroEditField);
    } else if (y < 28 && x < 80) {
        stopFocusMacro(true); openScreen(SCREEN_MAIN);
    } else if (focusMacroPhase == 0) {
        for (uint8_t field = 0; field < 3; ++field) {
            if (inside(x, y, MACRO_FIELDS[field]) && macroEditField != field) {
                const uint8_t previousField = macroEditField;
                macroEditField = field;
                drawMacroField(previousField);
                drawMacroField(field);
                M5.Lcd.fillRect(MACRO_TAKE_BUTTON.x, MACRO_TAKE_BUTTON.y, MACRO_TAKE_BUTTON.w, MACRO_TAKE_BUTTON.h, BLACK);
                drawMacroButton(MACRO_TAKE_BUTTON, "TAKE<BR>CURRENT<BR>FOCUS", cameraConnected && cameraFocus >= 0 && macroEditField < 2);
                break;
            }
        }
    }
}
