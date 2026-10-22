#pragma once

// UI: focus page.

#include "config.h"
#include "globals.h"
#include "ble_protocol.h"
#include "ui_main_screen.h"
#include "ui_presets_screen.h"  // openScreen(SCREEN_MAIN)

// Focus page geometry. FOCUS_SLIDER_PAD is the slack around the slider that still counts as a drag.
//{x, y, w, h}
enum FocusKey : uint8_t { FOCUS_KEY_MINUS, FOCUS_KEY_AF, FOCUS_KEY_PLUS };
static const Rect FOCUS_SLIDER = {20, 34, 280, 30};       // drawn track
static const Rect FOCUS_SLIDER_PAD = {14, 26, 292, 46};   // slack around the track that still counts as a drag
static const Rect FOCUS_MINUS_BUTTON = {131, 130, 54, 71};
static const Rect FOCUS_AF_BUTTON = {255, 154, 63, 48};
static const Rect FOCUS_PLUS_BUTTON = {190, 130, 54, 71};
static const Rect FOCUS_BACK = {6, 167, 118, 34};
static const int FOCUS_PERCENT_X = 160;                    // centered above the +/- controls
static const int FOCUS_PERCENT_Y = 111;
static const int FOCUS_TIMER_LEFT = 83;                     // main-screen layout shifted to center below the slider
static const int FOCUS_TIMER_Y = 84;
static const Rect FOCUS_TIMER_BADGE = {213, 78, 24, 13};
static const int FOCUS_NEAR_LABEL_X = 10;                  // rotated label centers at each end of the slider
static const int FOCUS_FAR_LABEL_X = 310;
static const int FOCUS_LABEL_Y = 49;
static const uint32_t FOCUS_REPEAT_MS = 100;              // repeat interval while a focus key or the slider is held

static M5Canvas focusLabelCanvas(&M5.Lcd);
static bool focusLabelCanvasReady = false;
static char drawnFocusTimecode[20] = {};
static uint16_t drawnFocusTimecodeColor = BLACK;

static void drawRotatedFocusLabel(const char* label, int centerX, int centerY) {
    if (!focusLabelCanvasReady) {
        focusLabelCanvas.setColorDepth(8);
        if (!focusLabelCanvas.createSprite(40, 18)) return;
        focusLabelCanvasReady = true;
    }
    focusLabelCanvas.fillSprite(BLACK);
    focusLabelCanvas.setTextColor(GREY, BLACK);
    focusLabelCanvas.setTextDatum(MC_DATUM);
    focusLabelCanvas.drawString(label, 20, 9, 1);
    focusLabelCanvas.pushRotateZoom(centerX, centerY, 270.0f, 1.0f, 1.0f, BLACK);
}

static void drawFocusTimecodeBadge() {
    M5.Lcd.fillRect(FOCUS_TIMER_BADGE.x, FOCUS_TIMER_BADGE.y,
                    FOCUS_TIMER_BADGE.w, FOCUS_TIMER_BADGE.h, BLACK);
    if (timecodeSource != 0) return;
    M5.Lcd.fillRect(FOCUS_TIMER_BADGE.x, FOCUS_TIMER_BADGE.y,
                    FOCUS_TIMER_BADGE.w, FOCUS_TIMER_BADGE.h, WHITE);
    M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString("TC", FOCUS_TIMER_BADGE.x + FOCUS_TIMER_BADGE.w / 2,
                       FOCUS_TIMER_BADGE.y + FOCUS_TIMER_BADGE.h / 2 + 1, 1);
}

static void drawFocusTimecode() {
    char text[20];
    timecodeText(text, sizeof(text));
    const uint16_t color = transportMode == 2 ? RED : WHITE;
    if (color != drawnFocusTimecodeColor) {
        memset(drawnFocusTimecode, 0, sizeof(drawnFocusTimecode));
        drawnFocusTimecodeColor = color;
    }
    M5.Lcd.setTextColor(color, BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextPadding(0);
    M5.Lcd.setFreeFont(&fonts::FreeSans9pt7b);
    int cellX = FOCUS_TIMER_LEFT;
    for (int index = 0; index < TIMECODE_CHARS && text[index] != '\0'; ++index) {
        const int cellWidth = text[index] == ':' ? TIMECODE_COLON_CELL : TIMECODE_DIGIT_CELL;
        if (text[index] != drawnFocusTimecode[index]) {
            const char glyph[2] = {text[index], '\0'};
            M5.Lcd.fillRect(cellX, FOCUS_TIMER_Y - 7, cellWidth, 20, BLACK);
            M5.Lcd.drawString(glyph, cellX + cellWidth / 2, FOCUS_TIMER_Y);
            drawnFocusTimecode[index] = text[index];
        }
        cellX += cellWidth;
    }
    M5.Lcd.setFreeFont(nullptr);
    M5.Lcd.setTextFont(1);
    drawFocusTimecodeBadge();
}

// Redraws only the percentage and slider, so dragging doesn't flicker.
static void drawFocusDynamic() {
    char text[8] = "--";
    if (cameraFocus >= 0) snprintf(text, sizeof(text), "%d%%", ((int)cameraFocus * 100 + FOCUS_MAX / 2) / FOCUS_MAX);
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextPadding(100);
    M5.Lcd.drawString(text, FOCUS_PERCENT_X, FOCUS_PERCENT_Y, 4);
    M5.Lcd.setTextPadding(0);

    M5.Lcd.fillRect(FOCUS_SLIDER.x - 4, FOCUS_SLIDER.y - 2, FOCUS_SLIDER.w + 8, FOCUS_SLIDER.h + 4, BLACK);
    M5.Lcd.drawRect(FOCUS_SLIDER.x, FOCUS_SLIDER.y, FOCUS_SLIDER.w, FOCUS_SLIDER.h, GREY);
    if (cameraFocus < 0) return;
    const int position = (int)cameraFocus * (FOCUS_SLIDER.w - 1) / FOCUS_MAX;
    if (position > 0) M5.Lcd.fillRect(FOCUS_SLIDER.x + 1, FOCUS_SLIDER.y + 1, position, FOCUS_SLIDER.h - 2, UI_CONNECT);
    M5.Lcd.fillRect(FOCUS_SLIDER.x + position - 2, FOCUS_SLIDER.y - 2, 5, FOCUS_SLIDER.h + 4, WHITE);
}

// Draws the manual focus page: slider, - / AF / + keys and the BACK button.
static void drawFocusScreen() {
    drawSubHeader("FOCUS");
    drawRotatedFocusLabel("NEAR", FOCUS_NEAR_LABEL_X, FOCUS_LABEL_Y);
    drawRotatedFocusLabel("FAR", FOCUS_FAR_LABEL_X, FOCUS_LABEL_Y);
    drawFocusDynamic();
    drawnFocusTimecode[0] = '\0';
    drawnFocusTimecodeColor = BLACK;
    drawFocusTimecode();

    const Rect& af = FOCUS_AF_BUTTON;
    drawStepperPair(FOCUS_MINUS_BUTTON, FOCUS_PLUS_BUTTON, 20, 4);
    M5.Lcd.fillRoundRect(af.x, af.y, af.w, af.h, 8, UI_CONNECT);
    M5.Lcd.setTextColor(WHITE, UI_CONNECT);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString("AF", af.x + af.w / 2, af.y + af.h / 2, 4);

    drawRecordButton();
    drawOutlineButton(FOCUS_BACK, "BACK", 4);
    drawStatus();
}

// The slider and the -/+ keys repeat while held; AF does not.
static bool focusTouchRepeats(int x, int y) {
    return inside(x, y, FOCUS_SLIDER_PAD) || inside(x, y, FOCUS_MINUS_BUTTON) ||
           inside(x, y, FOCUS_PLUS_BUTTON);
}

// BACK leaves the page; the slider jumps to the tapped position and the keys nudge by one percent.
static void handleFocusTouch(int x, int y) {
    if (inside(x, y, RECORD_PAD)) {
        if (cameraConnected) sendPacket(TRANSPORT_PACKET, transportMode == 2 ? 0 : 2, 1);
        return;
    }
    if (inside(x, y, FOCUS_BACK)) {
        openScreen(SCREEN_FOCUS_MENU);
        return;
    }
    const bool onSlider = inside(x, y, FOCUS_SLIDER_PAD);
    int key = -1;
    if (inside(x, y, FOCUS_MINUS_BUTTON)) key = FOCUS_KEY_MINUS;
    else if (inside(x, y, FOCUS_AF_BUTTON)) key = FOCUS_KEY_AF;
    else if (inside(x, y, FOCUS_PLUS_BUTTON)) key = FOCUS_KEY_PLUS;
    if (!onSlider && key < 0) return;
    if (!cameraConnected) {
        setStatus("Connect camera to set focus");
        return;
    }
    if (key == FOCUS_KEY_AF) {
        // One-shot autofocus; the camera reports the new focus afterwards.
        setStatus(writeCameraPacket(AUTOFOCUS_PACKET, sizeof(AUTOFOCUS_PACKET)) ? "Autofocus triggered" : "Autofocus failed");
        return;
    }
    if (onSlider) {
        sendFocus((x - FOCUS_SLIDER.x) * FOCUS_MAX / FOCUS_SLIDER.w);
    } else {
        if (cameraFocus < 0) {
            setStatus("Focus unknown, use the slider");
            return;
        }
        // Step in whole percent so the readout changes by exactly 1 per tap.
        const int percent = ((int)cameraFocus * 100 + FOCUS_MAX / 2) / FOCUS_MAX + (key == FOCUS_KEY_PLUS ? 1 : -1);
        sendFocus((constrain(percent, 0, 100) * FOCUS_MAX + 50) / 100);
    }
    drawFocusDynamic();
}
