import QtQuick
import QtQuick.Controls.Basic
import Los

// Basic uses window for the popup and light/midlight for highlighted rows.
// Set the complete palette so both string models and textRole models inherit it.
ComboBox {
    palette.window: Theme.dropdownBackground
    palette.base: Theme.dropdownBackground
    palette.button: Theme.dropdownBackground
    palette.mid: Theme.dropdownPressed
    palette.dark: Theme.dropdownText
    palette.text: Theme.dropdownText
    palette.windowText: Theme.dropdownText
    palette.buttonText: Theme.dropdownText
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.light: Theme.accent
    palette.midlight: Theme.accent
    palette.disabled.button: Theme.dropdownDisabled
    palette.disabled.buttonText: Theme.dropdownDisabledText
    palette.disabled.text: Theme.dropdownDisabledText
}
