/*
 * pitch_detector.h
 *
 * Implementation of the pitch detection mechanism
 */

#include <stdexcept>
#include <iostream>

#include "lmtypes.h"
#include "lmhelpers.h"
#include "pitch_detector.h"

#define FREQ_A4_NOTE            ((freq_hz_t) 440)
#define FREQ_PRECISION          4
#define OCTAVES_CNT             9   /* from 0 to 8 */
#define SEMITONES_PER_OCTAVE    12


PitchDetector::PitchDetector()
{
    __mNotesFromA4[-9] = note_C;
    __mNotesFromA4[-8] = note_C_sharp;
    __mNotesFromA4[-7] = note_D;
    __mNotesFromA4[-6] = note_D_sharp;
    __mNotesFromA4[-5] = note_E;
    __mNotesFromA4[-4] = note_F;
    __mNotesFromA4[-3] = note_F_sharp;
    __mNotesFromA4[-2] = note_G;
    __mNotesFromA4[-1] = note_G_sharp;
    __mNotesFromA4[0] = note_A;
    __mNotesFromA4[1] = note_A_sharp;
    __mNotesFromA4[2] = note_B;

    __initPitches();
}

PitchDetector::~PitchDetector()
{
    delete __mPitches;
}

void PitchDetector::__initPitches()
{
    __mPitches = new freq_hz_t[SEMITONES_TOTAL];

    for (int16_t i = 0; i < SEMITONES_TOTAL; i++) {
        int16_t n = i + SEMITONES_A0_TO_A4;
        __mPitches[i] = pow(2, (n / SEMITONES_PER_OCTAVE)) * FREQ_A4_NOTE;
        __mPitches[i] = Helpers::stdRound<freq_hz_t>(__mPitches[n], FREQ_PRECISION);
    }
}

bool PitchDetector::__isPitch(freq_hz_t freq)
{
    uint16_t start = 0, end = SEMITONES_TOTAL - 1, mid;

    freq = Helpers::stdRound(freq, FREQ_PRECISION);

    while (start <= end) {
        mid = start + (end - start) / 2;

        if (__mPitches[mid] < freq) {
            start = mid + 1;
        } else if (__mPitches[mid] > freq) {
            end = mid - 1;
        } else {
            return true;
        }
    }

    return false;
}

/* TODO: reuse delta mechanism used in getPitch */
freq_hz_t PitchDetector::__getTonic(std::vector<complex_t> x, uint32_t sampleRate)
{
    if (x.size() == 0 || sampleRate == 0) {
        throw std::invalid_argument("Invalid argument");
    }

    amplitude_t max = real(x[0]);
    amplitude_t mag;
    uint32_t i = 0, maxIndex = 0;

    /* TODO: replace with faster algorithm than the one with linear complexity */
    while (i < x.size()) {
        mag = real(x[i]);
        if (mag > max) {
            max = mag;
            maxIndex = i;
        }
        i++;
    }

    return (maxIndex * sampleRate / x.size());
}

freq_hz_t PitchDetector::getPitch(std::vector<complex_t> x, uint32_t sampleRate)
{
    freq_hz_t freqTonic;                // frequency with the highest amplitude
    freq_hz_t freqPitch = FREQ_INVALID; // closest pitch matching freqTonic
    freq_hz_t deltaRight, deltaLeft, deltaMid;
    uint16_t start = 0, end = SEMITONES_TOTAL - 1, mid;

    freqTonic = __getTonic(x, sampleRate);

    if (__isPitch(freqTonic)) {
        freqPitch = freqTonic;
        goto ret;
    }

    while (start <= end) {
        mid = start + (end - start) / 2;
        deltaMid = abs(__mPitches[mid] - freqTonic);
        /* fix index out of range potential bug */
        deltaLeft = abs(__mPitches[mid - 1] - freqTonic);
        deltaRight = abs(__mPitches[mid + 1] - freqTonic);

        if ((deltaLeft < deltaMid) && (deltaMid < deltaRight)) {
            end = mid - 1;
        } else if ((deltaLeft > deltaMid) && (deltaRight < deltaMid)) {
            start = mid + 1;
        } else {
            freqPitch = __mPitches[mid];
            break;
        }
    }

 ret:
    return freqPitch;
}

note_t PitchDetector::pitchToNote(freq_hz_t freq)
{
    if (!__isPitch(freq)) {
        throw std::invalid_argument("Invalid argument");
    }

    uint8_t semitonesFromA4 = Helpers::stdRound(SEMITONES_PER_OCTAVE *
                                                log2(freq / FREQ_A4_NOTE), 0);

    return (__mNotesFromA4.find(semitonesFromA4) != __mNotesFromA4.end() ?
            __mNotesFromA4[semitonesFromA4] : note_Unknown);
}
