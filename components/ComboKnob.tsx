import React, {useId} from "react"
import {KnobHeadless, KnobHeadlessLabel, KnobHeadlessOutput, useKnobKeyboardControls} from "react-knob-headless"
import withJuceComboBox, {WithJUCEComboBoxProps} from "./withJuceComboBox"
import functions from "../structures/Functions"
import "./styles/knob.scss"

interface Props {
    parameterID: string
    label: string
    color: string
    roundFunction?: (value: number) => number
    displayFunction?: (value: number) => string
    style?: React.CSSProperties
}

const ComboKnob: React.FunctionComponent<Props & WithJUCEComboBoxProps> = ({label, parameterID, 
    color, roundFunction, displayFunction, style, value, properties, onChange, reset}) => {
    const knobID = useId()
    const labelID = useId()

    const minAngle = -145
    const maxAngle = 145
    const min = 0
    const max = Math.max(0, properties.choices.length - 1)

    const clampedValue = Math.max(min, Math.min(max, value))
    const angle = functions.remapRange(clampedValue, min, max, minAngle, maxAngle)

    if (!roundFunction) roundFunction = (value: number) => Math.round(value)

    if (!displayFunction) displayFunction = (value: number) => properties.choices[Math.round(value)] ?? ""

    const keyboardHandler = useKnobKeyboardControls({
        valueRaw: value,
        valueMin: min,
        valueMax: max,
        step: 1,
        stepLarger: 1,
        onValueRawChange: onChange,
    })

    const changeIndex = (value: number) => {
        const scaled = Math.max(min, Math.min(max, value))
        onChange(scaled)
    }

    return (
        <div className="knob-container" style={{...style}}>
            <KnobHeadlessLabel className="knob-label" id={labelID}>
                {label}
            </KnobHeadlessLabel>
            <KnobHeadless
                aria-label={parameterID}
                aria-labelledby={labelID}
                dragSensitivity={0.006}
                onValueRawChange={changeIndex}
                onDoubleClick={reset}
                valueMin={min}
                valueMax={max}
                valueRaw={clampedValue}
                valueRawDisplayFn={displayFunction}
                valueRawRoundFn={roundFunction}
                axis="y"
                style={{outline: "none"}}
                {...keyboardHandler}>
                <div className="knob" style={{backgroundColor: color}}>
                    <div className="knob-rotator" style={{transform: `rotate(${angle}deg)`}}>
                        <div className="knob-indicator"/>
                    </div>
                </div>
            </KnobHeadless>
            <KnobHeadlessOutput className="knob-value" htmlFor={knobID} style={{color}}>
                {displayFunction(clampedValue)}
            </KnobHeadlessOutput>
        </div>
    )
}

export default withJuceComboBox(ComboKnob)