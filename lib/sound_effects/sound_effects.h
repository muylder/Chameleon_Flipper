#pragma once

#include <furi.h>
#include <notification/notification_messages.h>

// Sound effect types
typedef enum {
    SoundEffectSuccess,        // Operation successful (connection, read, write)
    SoundEffectError,          // Operation failed
    SoundEffectClick,          // Menu click/selection
    SoundEffectScan,           // Scanning started
    SoundEffectProgress,       // Operation in progress
    SoundEffectComplete,       // Task completed
    SoundEffectWarning,        // Warning/attention needed
} SoundEffectType;

// Play sound effect with optional haptic feedback
void sound_effects_play(SoundEffectType type, bool with_haptic);

// Convenience functions
void sound_effects_success(void);       // Success with haptic
void sound_effects_error(void);         // Error with haptic
void sound_effects_click(void);         // Click without haptic
void sound_effects_scan(void);          // Scan start with haptic
void sound_effects_complete(void);      // Complete with haptic
void sound_effects_warning(void);       // Warning with haptic

// Haptic only (no sound)
void sound_effects_haptic_light(void);  // Light vibration
void sound_effects_haptic_medium(void); // Medium vibration
void sound_effects_haptic_strong(void); // Strong vibration
