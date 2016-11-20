/*
 * fft.cpp
 *
 *  Chord recognition implementation
 */

#include <iostream> // TODO remove when __cutoffFreq is implemented

#include "chord_detector.h"
#include "lmhelpers.h"

#define FREQ_A0     ((amplitude_t)27.5)
#define FREQ_C8     ((amplitude_t)27.5)


using namespace std;

ChordDetector::ChordDetector()
{
    __mFft = new FFT();
    __mPitchDetector = new PitchDetector();
}

void ChordDetector::__cutoffFreq(vector<complex_t> &x, amplitude_t freq, bool cutHigh)
{
    cout << x.size() << freq << cutHigh;
}

chord_t ChordDetector::getChord(amplitude_t *timeDomain, uint32_t samples,
                                uint32_t sampleRate)
{
    chord_t chord;

    if ((timeDomain == NULL) || (samples == 0)) {
        chord.mainNote = note_Unknown;
        return chord;
    }

    vector<complex_t> x;
    freq_hz_t pitchFreq;
    uint32_t fftSize;

    /* TODO: define minimum FFT size for frequency calculation precision */
    fftSize = Helpers::nextPowerOf2(samples);
    x = Helpers::timeDomain2ComplexVector(timeDomain, fftSize);

    __mFft->forward(x);
    __mFft->toPolar(x);
    __cutoffFreq(x, FREQ_A0, false);
    __cutoffFreq(x, FREQ_C8, true);

    pitchFreq = __mPitchDetector->getPitch(x, sampleRate);
    if (pitchFreq == FREQ_INVALID) {
        goto err;
    }

    return chord;

err:
    chord.mainNote = note_Unknown;
    return chord;
}
