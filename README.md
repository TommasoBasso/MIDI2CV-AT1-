# MIDI2CV-AT1-

This project is an Eurorack module (for AT1 Modular) that converts MIDI signals into Control Voltage (CV) signals usable with modular synthesizers. The module supports the following main functions: GATE, CV, CLOCK, RESET, and MOD.

MAIN FEATURES
GATE: Activates and deactivates the Gate signal based on MIDI NoteOn and NoteOff messages.
CV (Control Voltage): Converts MIDI notes into voltage signals with 1V/octave precision for synthesizer pitch control.
CLOCK: Manages MIDI clock for synchronizing external Eurorack modules.
RESET: Performs a system reset when a MIDI Stop message is received, useful for resetting the system's sequence.
MOD (Modulation): Converts MIDI ControlChange messages into CV signals, using the second channel of the DAC.

Libraries used: SPI, MIDI

