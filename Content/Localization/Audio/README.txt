SHIFT / 24 external sound cues

Use standard RIFF WAVE, PCM format 1, signed 16-bit little-endian samples,
mono or stereo, 8000–192000 Hz. Each file must be smaller than 64 MiB.

Supported filenames
door_chime.wav
scanner_beep.wav
light_flicker.wav
jumpscare_reveal.wav
ward_off.wav

For development, place files in this directory. Packaging stages this directory.
For the Windows build, files can also be placed in
Windows/ShiftStore/Content/Localization/Audio/ before launching the game.
Adding a previously absent cue requires no editor or Python installation.
If the filename was already included in a packaged PAK, rebuild the package to
replace that cue reliably. Restart the game after changing files.

Missing, unsupported or truncated files use the existing synthesized sounds.
No recorded WAV clips are included in this implementation.
