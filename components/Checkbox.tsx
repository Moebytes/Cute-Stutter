import React from "react"
import withJuceToggleButton, {WithJUCEToggleButtonProps} from "./withJuceToggleButton"
import "./styles/checkbox.scss"

interface CheckboxProps {
    parameterID: string
    label: string
}

const Checkbox: React.FunctionComponent<CheckboxProps & WithJUCEToggleButtonProps> = ({label, value, onChange}) => {
    return (
        <div className="checkbox-container">
            <span className="checkbox-text">{label}</span>
            <div className={`checkbox ${value ? "checked" : ""}`} onClick={() => onChange(!value)}></div>
        </div>
    )
}

export default withJuceToggleButton(Checkbox)