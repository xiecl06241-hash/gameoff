SHIFT / 24 PR #1 playable audio preview

Runtime WAVs use mono 48 kHz, signed 16-bit RIFF PCM.
Door entry, item pickup/drop/throw, paper note, task start, heartbeat, reveal,
phase switching and state-dependent ambience use converted PR #1 candidates.
Ambient beds stop at title/dawn/game-over and switch between normal, Otherworld
and active-monster states. Ordinary footsteps, scanner, light flicker and ward-off
retain their previous CC0 recordings.

Credits: SourceArt/Audio/RUNTIME_CREDITS.md (Windows/Licenses/PR1_AUDIO_CREDITS.md).
Reproduce conversion: python Tools/prepare_pr1_audio.py --ffmpeg <ffmpeg executable>.
Missing or invalid one-shot cues fall back to synthesized sounds.
