#include "Processor.h"
#include "Editor.h"
#include "Functions.hpp"

Processor::Processor() : AudioProcessor(
    BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true)
    ), parameters(tree), presetManager(tree) {
}

Processor::~Processor() {}

auto Processor::isBusesLayoutSupported(const BusesLayout& layouts) const -> bool {
    auto mono = AudioChannelSet::mono();
    auto stereo = AudioChannelSet::stereo();
    auto mainIn = layouts.getMainInputChannelSet();
    auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn == mono && mainOut == mono) return true;
    if (mainIn == mono && mainOut == stereo) return true;
    if (mainIn == stereo && mainOut == stereo) return true;
    return false;
}

auto Processor::getNumPrograms() -> int {
    return static_cast<int>(this->presetManager.factoryPresets.size());
}

auto Processor::getCurrentProgram() -> int {
    return this->presetManager.presetIndex;
}

auto Processor::setCurrentProgram(int index) -> void {
    this->presetManager.presetFolder = "factory";
    this->presetManager.setPreset(index);
}

auto Processor::getProgramName(int index) -> const String {
    int safeIndex = jlimit(0, this->getNumPrograms() - 1, index);
    auto presetName = this->presetManager.factoryPresetNames[static_cast<size_t>(safeIndex)];
    return Functions::replaceChar(presetName, '/', '-');
}

auto Processor::changeProgramName([[maybe_unused]] int index, [[maybe_unused]] const String& newName) -> void {}

auto Processor::createEditor() -> AudioProcessorEditor* {
    return new Editor(*this);
}

auto Processor::getStateInformation(MemoryBlock& destData) -> void {
    auto jsonStr = this->presetManager.savePreset();
    destData.replaceAll(jsonStr.toUTF8(), jsonStr.getNumBytesAsUTF8());
}

auto Processor::setStateInformation(const void* data, int sizeInBytes) -> void {
    auto jsonStr = String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    this->presetManager.loadPreset(jsonStr);
}

auto JUCE_CALLTYPE createPluginFilter() -> AudioProcessor* {
    return new Processor();
}

auto Processor::prepareToPlay(double sampleRate, int samplesPerBlock) -> void {
    this->parameters.prepareToPlay(sampleRate, samplesPerBlock);
    this->parameters.reset();

    this->stutter.prepareToPlay(sampleRate);
}

auto Processor::releaseResources() -> void {}

auto Processor::getHostInfo() noexcept -> std::tuple<double, double, TimeSignature, bool> {
    double bpm = 150.0;
    double ppq = 0.0;
    TimeSignature timeSignature{4, 4};
    bool isPlaying = false;

    if (auto* playhead = this->getPlayHead()) {
        auto info = playhead->getPosition().orFallback(AudioPlayHead::PositionInfo{});
        bpm = info.getBpm().orFallback(150.0);
        ppq = info.getPpqPosition().orFallback(0.0);
        timeSignature = info.getTimeSignature().orFallback(TimeSignature{4, 4});
        isPlaying = info.getIsPlaying();
    }

    return {bpm, ppq, timeSignature, isPlaying};
}

auto Processor::processBlock(AudioBuffer<float>& buffer, [[maybe_unused]] MidiBuffer& midiMessages) -> void {
    ScopedNoDenormals noDenormals;

    auto mainInput = this->getBusBuffer(buffer, true, 0);
    auto mainOutput = this->getBusBuffer(buffer, false, 0);

    const float* inputL = mainInput.getReadPointer(0);
    const float* inputR = mainInput.getNumChannels() > 1 ? mainInput.getReadPointer(1) : inputL;

    float* outputL = mainOutput.getWritePointer(0);
    float* outputR = mainOutput.getNumChannels() > 1 ? mainOutput.getWritePointer(1) : outputL;

    auto [bpm, ppq, timeSignature, isPlaying] = this->getHostInfo();
    this->parameters.setHostInfo(bpm, ppq, timeSignature);
    this->parameters.blockUpdate();

    this->stutter.setBPM(bpm);
    this->stutter.setRate(this->parameters.rate);
    this->stutter.setHold(this->parameters.hold);
    this->stutter.setLength(this->parameters.length);
    this->stutter.setReverse(this->parameters.reverse);

    double ppqPerSample = (bpm / 60.0) / getSampleRate();

    if (!isPlaying) {
        this->stutter.reset();

        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            outputL[sample] = inputL[sample];
            outputR[sample] = inputR[sample];
        }
        return;
    }

    for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
        double samplePPQ = ppq + sample * ppqPerSample;
        this->stutter.setPPQ(samplePPQ);

        this->parameters.update();

        const float dryL = inputL[sample];
        const float dryR = inputR[sample];
        float wetL = 0.0f;
        float wetR = 0.0f;

        this->stutter.process(dryL, dryR, wetL, wetR);

        outputL[sample] = wetL;
        outputR[sample] = wetR;
    }
 
    #if JUCE_DEBUG
        Functions::checkAudioSafety(buffer);
    #endif
}