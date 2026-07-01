#pragma once
#include <JuceHeader.h>

class Stutter {
public:
    Stutter() = default;
    ~Stutter() = default;

    auto prepareToPlay(double sampleRate, double maxLengthSeconds = 4.0) -> void {
        this->sampleRate = sampleRate;
        this->bufferSize = static_cast<int>(sampleRate * maxLengthSeconds);

        this->buffer.setSize(2, bufferSize);
        this->buffer.clear();
        
        this->reset();
    }

    auto reset() -> void {
        this->writePos = 0;
        this->playPos = 0;
        this->playLength = 0;
        this->sliceStart = 0;
        this->sliceLength = 0;
        this->captureGrid = -1;
    }

    auto setBPM(double bpm) -> void {
        this->bpm = bpm;
        this->updateTiming();
    }

    auto setPPQ(double ppq) -> void {
        this->ppq = ppq;
    }

    auto setRate(const String& rate) -> void {
        this->rate = rate;
        this->updateTiming();
    }

    auto setHold(const String& hold) -> void {
        this->hold = hold;
        this->updateTiming();
    }

    auto setLength(float length) -> void {
        this->length = length;
        this->updateTiming();
    }

    auto setReverse(bool reverse) -> void {
        this->reverse = reverse;
    }

    auto getBeats(const String& rate) const -> double {
        if (rate == "8/1") return 32.0;
        if (rate == "4/1") return 16.0;
        if (rate == "2/1") return 8.0;
        if (rate == "1/1") return 4.0;
        if (rate == "1/2") return 2.0;
        if (rate == "1/3") return 4.0 / 3.0;
        if (rate == "1/4") return 1.0;
        if (rate == "1/6") return 2.0 / 3.0;
        if (rate == "1/8") return 0.5;
        if (rate == "1/12") return 1.0 / 3.0;
        if (rate == "1/16") return 0.25;
        if (rate == "1/24") return 1.0 / 6.0;
        if (rate == "1/32") return 0.125;
        return 1.0;
    }

    auto updateTiming() -> void {
        int quarterSamples = static_cast<int>(60.0 * this->sampleRate / this->bpm);

        this->sliceLength = jmax(1, static_cast<int>(quarterSamples * this->getBeats(this->rate)));
        this->playLength = jmax(1, static_cast<int>(static_cast<float>(sliceLength) * this->length));

        this->holdGrids = jmax(1, static_cast<int>(std::round(getBeats(this->hold) / getBeats(this->rate))));
    }

    auto process(float inL, float inR, float& outL, float& outR) -> void {
        this->buffer.setSample(0, this->writePos, inL);
        this->buffer.setSample(1, this->writePos, inR);

        if (this->rate == "Off") {
            outL = inL;
            outR = inR;
            return;
        }

        int grid = static_cast<int>(std::floor(this->ppq / this->getBeats(this->rate)));
        bool jumpedBack = this->lastGrid != -1 && grid < this->lastGrid;

        if (jumpedBack) {
            this->captureGrid = -1;
            this->playPos = 0;
        }
        this->lastGrid = grid;
        
        if (this->captureGrid == -1 || (grid - this->captureGrid) >= this->holdGrids) {
            this->captureGrid = grid; 
            this->playPos = 0; 
            this->sliceStart = writePos;
        }

        int pos = this->reverse ? (this->playLength - 1) - this->playPos : this->playPos;
        int readPos = (this->sliceStart + pos) % this->bufferSize;

        if (this->playPos < this->playLength) {
            outL = this->buffer.getSample(0, readPos);
            outR = this->buffer.getSample(1, readPos);
        } else {
            outL = 0.0f;
            outR = 0.0f;
        }
        
        this->writePos = (this->writePos + 1) % this->bufferSize;
        this->playPos++;
        if (this->playPos >= this->sliceLength) {
            this->playPos = 0;
        }
    }

private:
    double sampleRate = 44100.0;
    double bpm = 150.0;
    double ppq = 0.0;

    String rate = "1/8";
    String hold = "2/1";
    float length = 0.5f;
    bool reverse = false;

    AudioBuffer<float> buffer;
    int bufferSize = 0;
    int writePos = 0;

    int playPos = 0;
    int playLength = 0;
    int sliceStart = 0;
    int sliceLength = 0;

    int captureGrid = -1;
    int lastGrid = -1;
    int holdGrids = 16;
};