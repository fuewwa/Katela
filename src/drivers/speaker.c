#include "speaker.h"
#include "../../include/scheduler.h"

#define PIT_CH2_PORT    0x42
#define PIT_CMD_PORT    0x43
#define SPEAKER_PORT    0x61

#define PIT_CMD_CH2_MODE3 0xB6

unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char value);

static void speaker_enable(void) {
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp | 0x03);
}

static void speaker_disable(void) {
    unsigned char tmp = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, tmp & 0xFC);
}

static void pit_set_frequency(unsigned int frequency) {
    if (frequency == 0) return;
    
    unsigned int divisor = 1193181 / frequency;
    
    outb(PIT_CMD_PORT, PIT_CMD_CH2_MODE3);
    
    outb(PIT_CH2_PORT, divisor & 0xFF);
    outb(PIT_CH2_PORT, (divisor >> 8) & 0xFF);
}

void init_speaker(void) {
    speaker_disable();
}

void speaker_tone(unsigned int frequency) {
    if (frequency == 0) {
        speaker_off();
        return;
    }
    pit_set_frequency(frequency);
    speaker_enable();
}

void speaker_off(void) {
    speaker_disable();
}

void speaker_beep(void) {
    speaker_tone(1000);
    task_sleep(200);
    speaker_off();
}
