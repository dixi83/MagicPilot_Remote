#pragma once

// Three saved focus positions, stored independently from camera-control presets.

#include "config.h"
#include "globals.h"

static const uint8_t FOCUS_PRESET_COUNT = 3;
static const char* FOCUS_PRESETS_PATH = "/focus_presets.cfg";
// Defaults span the full focus range; the selected slot starts at 1 and is not persisted.
static uint16_t focusPresetValues[FOCUS_PRESET_COUNT] = {0, FOCUS_MAX / 2, FOCUS_MAX};
static uint8_t selectedFocusPreset = 0;

static void saveFocusPresets() {
    // Persist all three raw focus values together as one CSV line.
    if (!sdReady) return;
    SD.remove(FOCUS_PRESETS_PATH);
    File file = SD.open(FOCUS_PRESETS_PATH, FILE_WRITE);
    if (!file) { setStatus("Could not save focus presets"); return; }
    file.printf("%u,%u,%u\n", (unsigned)focusPresetValues[0], (unsigned)focusPresetValues[1],
                (unsigned)focusPresetValues[2]);
    file.close();
}

static void loadFocusPresets() {
    if (!sdReady) return;
    File file = SD.open(FOCUS_PRESETS_PATH, FILE_READ);
    if (!file) return;
    char line[40] = {};
    const size_t count = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[count] = '\0';
    file.close();
    unsigned values[FOCUS_PRESET_COUNT] = {};
    // Accept the file only when all values are present and within the camera's focus range.
    if (sscanf(line, "%u,%u,%u", &values[0], &values[1], &values[2]) == FOCUS_PRESET_COUNT &&
        values[0] <= FOCUS_MAX && values[1] <= FOCUS_MAX && values[2] <= FOCUS_MAX) {
        for (uint8_t i = 0; i < FOCUS_PRESET_COUNT; ++i) focusPresetValues[i] = (uint16_t)values[i];
    }
}
