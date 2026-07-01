#include "Parameters.h"
#include "Functions.hpp"

template<typename T>
static auto castParameter(const AudioProcessorValueTreeState& tree, 
    const ParameterID* id, T*& dest) -> void {
    dest = dynamic_cast<T*>(tree.getParameter(id->getParamID()));
    jassert(dest != nullptr);
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterFloat* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterBool* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterChoice* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = static_cast<T>(paramObj->getDefaultValue());
}

ParameterIDs Parameters::paramIDs = ParameterIDs::loadFromJSON();

Parameters::Parameters(AudioProcessorValueTreeState& tree) : tree(tree) {
    using FloatPair = std::pair<AudioParameterFloat*&, const ParameterID*>;
    using BoolPair = std::pair<AudioParameterBool*&, const ParameterID*>;
    using ChoicePair = std::pair<AudioParameterChoice*&, const ParameterID*>;

    auto floatParameters = std::vector<FloatPair>{
        {lengthParam, &paramIDs.length}
    };

    auto boolParameters = std::vector<BoolPair>{
        {reverseParam, &paramIDs.reverse}
    };

    auto choiceParameters = std::vector<ChoicePair>{
        {rateParam, &paramIDs.rate},
        {holdParam, &paramIDs.hold}
    };

    for (const auto& [param, paramID] : floatParameters) {
        castParameter(tree, paramID, param);
    }

    for (const auto& [param, paramID] : boolParameters) {
        castParameter(tree, paramID, param);
    }

    for (auto& [param, paramID] : choiceParameters) {
        castParameter(tree, paramID, param);
    }
}

auto Parameters::createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout {
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.rate, "Rate", StringArray{"Off", "1/1", "1/2", "1/3", "1/4", "1/6", "1/8", "1/12", "1/16", "1/24", "1/32"}, 6
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.hold, "Hold", StringArray{"1/4", "1/2", "1/1", "2/1", "4/1", "8/1"}, 3
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.length, "Length", NormalisableRange<float>{0.1f, 1.0f, 0.01f}, 0.7f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    layout.add(std::make_unique<AudioParameterBool>(
        paramIDs.reverse, "Reverse", false
    ));

    return layout;
}

auto Parameters::getDefaultParameter(const Array<var>& args,
    WebBrowserComponent::NativeFunctionCompletion completion) -> void {

    auto paramID = args[0].toString();
    auto* param = this->tree.getParameter(paramID);
    float defaultValue = param->convertFrom0to1(param->getDefaultValue());

    completion(defaultValue);
}

auto Parameters::prepareToPlay(double sampleRate, int blockSize) noexcept -> void {
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;

    double duration = 0.001;

    auto smoothers = std::vector{
        &lengthSmoother
    };

    for (const auto& smoother : smoothers) {
        smoother->reset(this->sampleRate, duration);
    }
}

auto Parameters::reset() noexcept -> void {
    auto paramFloats = std::vector{
        std::pair{lengthParam, &length}
    };

    for (auto& [param, value] : paramFloats) {
        resetParameter(tree, param, value);
    }
    
    auto smoothers = std::vector{
        std::pair{lengthParam, &lengthSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setCurrentAndTargetValue(param->get());
    }
}

auto Parameters::setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void {
    this->bpm = bpm;
    this->ppq = ppq;
    this->timeSignature = timeSignature;

    if (ppq > 0.0) {
        this->ppq = ppq;
        this->internalPPQ = ppq;
    } else {
        double ppqPerSample = (this->bpm / 60.0) / this->sampleRate;
        this->internalPPQ += ppqPerSample * this->blockSize; 
        this->ppq = this->internalPPQ;
    }
}

auto Parameters::blockUpdate() noexcept -> void {
    auto smoothers = std::vector{
        std::pair{lengthParam, &lengthSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setTargetValue(param->get());
    }

    this->rate = this->rateParam->getCurrentChoiceName();
    this->hold = this->holdParam->getCurrentChoiceName();
    this->reverse = this->reverseParam->get();
}

auto Parameters::update() noexcept -> void {
    this->length = this->lengthSmoother.getNextValue();
}