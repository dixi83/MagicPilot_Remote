#pragma once

// Optional M5Stack Unit Fader input for the manual focus page.
// On the Core2, PORT.A's white analog signal is GPIO33. Other supported boards keep
// their existing focus controls and compile this adapter as a no-op.

#include "config.h"
#include "globals.h"
#include "ble_protocol.h"

#if defined(ARDUINO_M5STACK_Core2) || defined(ARDUINO_M5STACK_CORE2)
#define MAGIC_PILOT_HAS_FOCUS_FADER 1
static const int FOCUS_FADER_PIN = 33;
static const int FOCUS_FADER_ADC_MIN = 0;       // adjust after measuring your unit's endpoints, if needed
static const int FOCUS_FADER_ADC_MAX = 4095;
static const bool FOCUS_FADER_REVERSED = true;  // swap the physical direction to match NEAR -> FAR
static const uint32_t FOCUS_FADER_SAMPLE_MS = 20;
static const uint32_t FOCUS_FADER_SEND_MS = 50; // cap BLE focus writes at 20 per second
static const int FOCUS_FADER_PICKUP_TOLERANCE = FOCUS_MAX * 3 / 100;
static const int FOCUS_FADER_SEND_DEADBAND = 4;

static bool focusFaderPrimed = false;
static bool focusFaderPickedUp = false;
static uint32_t focusFaderLastSampleMs = 0;
static uint32_t focusFaderLastSendMs = 0;
static int32_t focusFaderFilteredRawQ4 = 0; // raw ADC value with four fractional steps for smoothing
static int focusFaderPreviousFocus = 0;
static int focusFaderPickupOrigin = 0;
static int focusFaderLastSent = -1;

static int focusFaderMapRaw(int raw) {
    const int bounded = constrain(raw, FOCUS_FADER_ADC_MIN, FOCUS_FADER_ADC_MAX);
    const int mapped = (bounded - FOCUS_FADER_ADC_MIN) * FOCUS_MAX /
                       (FOCUS_FADER_ADC_MAX - FOCUS_FADER_ADC_MIN);
    return FOCUS_FADER_REVERSED ? FOCUS_MAX - mapped : mapped;
}
#endif // Core2-only pin, state and mapping

static void initializeFocusFader() {
#if MAGIC_PILOT_HAS_FOCUS_FADER
    analogReadResolution(12);
    analogSetPinAttenuation(FOCUS_FADER_PIN, ADC_11db);
    pinMode(FOCUS_FADER_PIN, INPUT);
#endif
}

// Touch controls disarm the fader; it must pass the resulting camera position
// before taking control again, so a stationary fader cannot undo a touch.
static void focusFaderReleaseControl() {
#if MAGIC_PILOT_HAS_FOCUS_FADER
    focusFaderPickedUp = false;
    focusFaderLastSent = -1;
    if (focusFaderPrimed) focusFaderPickupOrigin = focusFaderMapRaw((int)(focusFaderFilteredRawQ4 / 4));
#endif
}

static void updateFocusFader() {
#if MAGIC_PILOT_HAS_FOCUS_FADER
    const bool enabled = screen == SCREEN_FOCUS && cameraConnected && focusMacroPhase == 0;
    if (!enabled) {
        focusFaderPrimed = false;
        focusFaderPickedUp = false;
        focusFaderLastSent = -1;
        return;
    }

    const uint32_t now = millis();
    if (now - focusFaderLastSampleMs < FOCUS_FADER_SAMPLE_MS) return;
    focusFaderLastSampleMs = now;

    const int raw = analogRead(FOCUS_FADER_PIN);
    if (!focusFaderPrimed) {
        focusFaderFilteredRawQ4 = raw * 4;
        focusFaderPreviousFocus = focusFaderMapRaw(raw);
        focusFaderPickupOrigin = focusFaderPreviousFocus;
        focusFaderPrimed = true;
        focusFaderPickedUp = false;
        focusFaderLastSent = -1;
        return;
    }

    // A light EMA removes ADC jitter without making the fader feel laggy.
    focusFaderFilteredRawQ4 += raw - focusFaderFilteredRawQ4 / 4;
    const int faderFocus = focusFaderMapRaw((int)(focusFaderFilteredRawQ4 / 4));

    if (!focusFaderPickedUp) {
        if (cameraFocus >= 0) {
            const int target = cameraFocus;
            const int previousOffset = focusFaderPreviousFocus - target;
            const int currentOffset = faderFocus - target;
            const bool crossedTarget = (previousOffset <= 0 && currentOffset >= 0) ||
                                       (previousOffset >= 0 && currentOffset <= 0);
            if (abs(currentOffset) <= FOCUS_FADER_PICKUP_TOLERANCE || crossedTarget) {
                focusFaderPickedUp = true;
                focusFaderLastSent = -1;
            }
        } else if (abs(faderFocus - focusFaderPickupOrigin) > FOCUS_FADER_PICKUP_TOLERANCE) {
            // If the camera has not reported focus yet, a deliberate fader move arms it.
            focusFaderPickedUp = true;
            focusFaderLastSent = -1;
        }
    }

    focusFaderPreviousFocus = faderFocus;
    if (!focusFaderPickedUp || now - focusFaderLastSendMs < FOCUS_FADER_SEND_MS) return;

    if (focusFaderLastSent < 0 || abs(faderFocus - focusFaderLastSent) >= FOCUS_FADER_SEND_DEADBAND) {
        if (sendFocus(faderFocus)) {
            focusFaderLastSent = faderFocus;
            focusFaderLastSendMs = now;
            focusDirty = true;
        }
    }
#endif
}

