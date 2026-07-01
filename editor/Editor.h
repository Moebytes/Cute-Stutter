#pragma once
#pragma clang diagnostic ignored "-Wshadow-field"
#include <JuceHeader.h>
#include "Processor.h"
#include "EventEmitter.hpp"

class Editor : public AudioProcessorEditor, public EventEmitter::Listener {
public:
    Editor(Processor& p);
    ~Editor() override;
    
    auto resized() -> void override;

    auto getResource(const String& url) -> std::optional<WebBrowserComponent::Resource>;
    auto webviewOptions() -> WebBrowserComponent::Options;
    auto getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte>;

    auto handleEvent(const String& name, const var& payload) -> void override;
    auto handleThemeChange(const String& theme) -> void;
        
private:
    Processor& processor;
    ComponentBoundsConstrainer constrainer;

    WebComboBoxRelay rateRelay {Parameters::paramIDs.rate.getParamID()};
    WebComboBoxParameterAttachment rateAttachment {*this->processor.parameters.rateParam, rateRelay, nullptr};

    WebComboBoxRelay holdRelay {Parameters::paramIDs.hold.getParamID()};
    WebComboBoxParameterAttachment holdAttachment {*this->processor.parameters.holdParam, holdRelay, nullptr};

    WebSliderRelay lengthRelay {Parameters::paramIDs.length.getParamID()};
    WebSliderParameterAttachment lengthAttachment {*this->processor.parameters.lengthParam, lengthRelay, nullptr};

    WebToggleButtonRelay reverseRelay {Parameters::paramIDs.reverse.getParamID()};
    WebToggleButtonParameterAttachment reverseAttachment {*this->processor.parameters.reverseParam, reverseRelay, nullptr};

    WebBrowserComponent webview;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};