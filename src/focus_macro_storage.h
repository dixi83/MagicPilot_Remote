#pragma once

// SD-card persistence for the focus pull macro values.

#include "config.h"
#include "globals.h"

static const char* FOCUS_MACRO_SETTINGS_PATH = "/focus_pull.cfg";

static void saveFocusMacroSettings() {
    if (!sdReady) return;
    SD.remove(FOCUS_MACRO_SETTINGS_PATH);
    File file = SD.open(FOCUS_MACRO_SETTINGS_PATH, FILE_WRITE);
    if (!file) { setStatus("Could not save macro settings"); return; }
    file.printf("%u,%u,%lu\n", (unsigned)macroFocusStart, (unsigned)macroFocusEnd,
                (unsigned long)macroFocusDurationMs);
    file.close();
}

static void loadFocusMacroSettings() {
    if (!sdReady) return;
    File file = SD.open(FOCUS_MACRO_SETTINGS_PATH, FILE_READ);
    if (!file) return;
    char line[40] = {};
    const size_t count = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[count] = '\0';
    file.close();
    unsigned start = 0, end = 0;
    unsigned long duration = 0;
    if (sscanf(line, "%u,%u,%lu", &start, &end, &duration) == 3 &&
        start <= FOCUS_MAX && end <= FOCUS_MAX && duration >= 1000 && duration <= 600000) {
        macroFocusStart = (uint16_t)start;
        macroFocusEnd = (uint16_t)end;
        macroFocusDurationMs = (uint32_t)duration;
    }
}
