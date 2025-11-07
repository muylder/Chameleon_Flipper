#include "sound_effects.h"
#include <notification/notification.h>
#include <notification/notification_messages.h>

// Custom sequences for better feedback

// Success sequence: short beep + single vibration
static const NotificationSequence sequence_chameleon_success = {
    &message_green_255,
    &message_vibro_on,
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,
    &message_delay_50,
    &message_green_0,
    NULL,
};

// Error sequence: buzz + double vibration
static const NotificationSequence sequence_chameleon_error = {
    &message_red_255,
    &message_vibro_on,
    &message_note_c4,
    &message_delay_100,
    &message_vibro_off,
    &message_delay_50,
    &message_vibro_on,
    &message_delay_100,
    &message_sound_off,
    &message_vibro_off,
    &message_red_0,
    NULL,
};

// Click sequence: short tick
static const NotificationSequence sequence_chameleon_click = {
    &message_note_c6,
    &message_delay_10,
    &message_sound_off,
    NULL,
};

// Scan sequence: ascending notes
static const NotificationSequence sequence_chameleon_scan = {
    &message_blue_255,
    &message_vibro_on,
    &message_note_c5,
    &message_delay_50,
    &message_note_e5,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,
    &message_blue_0,
    NULL,
};

// Progress sequence: short blip
static const NotificationSequence sequence_chameleon_progress = {
    &message_note_d5,
    &message_delay_25,
    &message_sound_off,
    NULL,
};

// Complete sequence: success melody
static const NotificationSequence sequence_chameleon_complete = {
    &message_green_255,
    &message_vibro_on,
    &message_note_c6,
    &message_delay_100,
    &message_note_e6,
    &message_delay_100,
    &message_note_g6,
    &message_delay_150,
    &message_sound_off,
    &message_vibro_off,
    &message_green_0,
    NULL,
};

// Warning sequence: double beep
static const NotificationSequence sequence_chameleon_warning = {
    &message_yellow_255,
    &message_note_a5,
    &message_delay_100,
    &message_sound_off,
    &message_delay_50,
    &message_note_a5,
    &message_delay_100,
    &message_sound_off,
    &message_yellow_0,
    NULL,
};

// Haptic-only sequences
static const NotificationSequence sequence_haptic_light = {
    &message_vibro_on,
    &message_delay_25,
    &message_vibro_off,
    NULL,
};

static const NotificationSequence sequence_haptic_medium = {
    &message_vibro_on,
    &message_delay_50,
    &message_vibro_off,
    NULL,
};

static const NotificationSequence sequence_haptic_strong = {
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    &message_delay_50,
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    NULL,
};

void sound_effects_play(SoundEffectType type, bool with_haptic) {
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);

    const NotificationSequence* sequence = NULL;

    switch(type) {
    case SoundEffectSuccess:
        sequence = with_haptic ? &sequence_chameleon_success : &sequence_success;
        break;
    case SoundEffectError:
        sequence = with_haptic ? &sequence_chameleon_error : &sequence_error;
        break;
    case SoundEffectClick:
        sequence = &sequence_chameleon_click;
        break;
    case SoundEffectScan:
        sequence = &sequence_chameleon_scan;
        break;
    case SoundEffectProgress:
        sequence = &sequence_chameleon_progress;
        break;
    case SoundEffectComplete:
        sequence = &sequence_chameleon_complete;
        break;
    case SoundEffectWarning:
        sequence = &sequence_chameleon_warning;
        break;
    default:
        sequence = &sequence_click;
        break;
    }

    if(sequence) {
        notification_message(notifications, sequence);
    }

    furi_record_close(RECORD_NOTIFICATION);
}

void sound_effects_success(void) {
    sound_effects_play(SoundEffectSuccess, true);
}

void sound_effects_error(void) {
    sound_effects_play(SoundEffectError, true);
}

void sound_effects_click(void) {
    sound_effects_play(SoundEffectClick, false);
}

void sound_effects_scan(void) {
    sound_effects_play(SoundEffectScan, true);
}

void sound_effects_complete(void) {
    sound_effects_play(SoundEffectComplete, true);
}

void sound_effects_warning(void) {
    sound_effects_play(SoundEffectWarning, false);
}

void sound_effects_haptic_light(void) {
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_haptic_light);
    furi_record_close(RECORD_NOTIFICATION);
}

void sound_effects_haptic_medium(void) {
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_haptic_medium);
    furi_record_close(RECORD_NOTIFICATION);
}

void sound_effects_haptic_strong(void) {
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_haptic_strong);
    furi_record_close(RECORD_NOTIFICATION);
}
