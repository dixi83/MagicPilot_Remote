#pragma once

// UI: three selectable focus positions with immediate camera updates.

#include "config.h"
#include "globals.h"
#include "ble_protocol.h"
#include "ui_main_screen.h"
#include "ui_presets_screen.h"
#include "focus_presets_storage.h"

// Keep the three choices in the left content area; the right column holds the shared record pad.
static const Rect FOCUS_PRESET_BUTTONS[FOCUS_PRESET_COUNT] = {
    {6, 56, 76, 68}, {87, 56, 76, 68}, {168, 56, 76, 68}
};
static const Rect FOCUS_PRESET_MINUS = {131, 130, 54, 71};
static const Rect FOCUS_PRESET_PLUS = {190, 130, 54, 71};
static const Rect FOCUS_PRESET_BACK = {6, 167, 118, 34};

static void drawFocusPresetButton(uint8_t index) {
    const Rect& box = FOCUS_PRESET_BUTTONS[index];
    const bool selected = index == selectedFocusPreset;
    const uint16_t outline = selected ? UI_SEL : GREY;
    M5.Lcd.fillRect(box.x, box.y, box.w, box.h, BLACK);
    M5.Lcd.drawRoundRect(box.x, box.y, box.w, box.h, 5, outline);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    char number[4];
    char value[8];
    snprintf(number, sizeof(number), "%u", (unsigned)(index + 1));
    snprintf(value, sizeof(value), "%u%%", (unsigned)((focusPresetValues[index] * 100UL + FOCUS_MAX / 2) / FOCUS_MAX));
    M5.Lcd.drawString(number, box.x + box.w / 2, box.y + 22, 4);
    M5.Lcd.setTextColor(selected ? UI_SEL : GREY, BLACK);
    M5.Lcd.drawString(value, box.x + box.w / 2, box.y + 51, 2);
}

static void drawFocusPresetsScreen() {
    drawSubHeader("FOCUS PRESETS");
    drawnTimecodeColor = BLACK;
    drawTimecode();
    for (uint8_t i = 0; i < FOCUS_PRESET_COUNT; ++i) drawFocusPresetButton(i);
    drawRecordButton();
    drawStepperPair(FOCUS_PRESET_MINUS, FOCUS_PRESET_PLUS, 24, 5);
    drawOutlineButton(FOCUS_PRESET_BACK, "BACK", 4);
    drawStatus();
}

static bool focusPresetTouchRepeats(int x, int y) {
    // Holding either stepper button repeats the change to the selected preset.
    return inside(x, y, FOCUS_PRESET_MINUS) || inside(x, y, FOCUS_PRESET_PLUS);
}

static void handleFocusPresetsTouch(int x, int y) {
    if (inside(x, y, RECORD_PAD)) {
        if (cameraConnected) sendPacket(TRANSPORT_PACKET, transportMode == 2 ? 0 : 2, 1);
        return;
    }
    if (inside(x, y, FOCUS_PRESET_BACK)) {
        openScreen(SCREEN_FOCUS_MENU);
        return;
    }
    for (uint8_t i = 0; i < FOCUS_PRESET_COUNT; ++i) {
        if (!inside(x, y, FOCUS_PRESET_BUTTONS[i])) continue;
        const uint8_t previous = selectedFocusPreset;
        selectedFocusPreset = i;
        if (previous != i) drawFocusPresetButton(previous);
        drawFocusPresetButton(i);
        // Selecting a preset immediately applies it to the camera when connected.
        if (cameraConnected) {
            if (!sendFocus(focusPresetValues[i])) setStatus("Focus preset update failed");
        } else {
            setStatus("Connect camera to set focus");
        }
        return;
    }
    if (!inside(x, y, FOCUS_PRESET_MINUS) && !inside(x, y, FOCUS_PRESET_PLUS)) return;

    // Edit in one-percent steps, then persist the values and send the new focus to the camera.
    const int direction = inside(x, y, FOCUS_PRESET_PLUS) ? 1 : -1;
    const int currentPercent = (focusPresetValues[selectedFocusPreset] * 100UL + FOCUS_MAX / 2) / FOCUS_MAX;
    const int nextPercent = constrain(currentPercent + direction, 0, 100);
    focusPresetValues[selectedFocusPreset] = (uint16_t)((nextPercent * FOCUS_MAX + 50) / 100);
    saveFocusPresets();
    if (cameraConnected) {
        if (!sendFocus(focusPresetValues[selectedFocusPreset])) setStatus("Focus preset update failed");
    } else {
        setStatus("Connect camera to set focus");
    }
    drawFocusPresetButton(selectedFocusPreset);
}
