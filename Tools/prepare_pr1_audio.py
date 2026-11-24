"""Convert selected PR #1 sources into attributed, bounded runtime PCM cues."""
import argparse
from array import array
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'SourceArt/Audio'
DEST = ROOT / 'Content/Localization/Audio'

# Source name, Freesound ID, creator, work title, license.
SOURCES = {
    'door': ('门铃-顾客进入.wav', 879804, 'Qvantor', 'Shop Door Chime', 'CC0'),
    'open': ('开门.wav', 15419, 'pagancow', 'Dorm Door Opening', 'CC0'),
    'cloth': ('整理时布料沙沙作响.wav', 435289, 'zazz.sound.design', 'Cloth Rustle 3', 'CC0'),
    'pickup': ('物品获取.wav', 878310, 'Qvantor', '8-bit Item Pickup', 'CC0'),
    'drop': ('盒子掉落可商用.wav', 762533, 'oscaraudiogeek', 'Boxes Fall on Wood Surface', 'CC0'),
    'throw': ('丢东西.mp3', 683101, 'florianreichelt', 'Quick Woosh', 'CC0'),
    'mop': ('拖地.wav', 726273, 'monosfera', 'ZED Wipe 013', 'CC0'),
    'tidy': ('擦拭.wav', 505835, 'jedg', 'Wipe Feet on Doormat', 'CC0'),
    'cook': ('咕噜噜沸腾.wav', 521149, 'maaaks', 'Boiling and Stirring', 'CC0'),
    'heartbeat': ('心跳-需加强.wav', 879917, 'WoodBetweenStudio', 'Heartbeat Tense', 'CC BY 4.0'),
    'reveal': ('突然的尖叫.wav', 817894, 'Daftrio', 'Demonic noise succeded by TV static', 'CC0'),
    'texture': ('一阵诡异音效.mp3', 570725, 'Infinita08', 'Horror Texture5', 'CC0'),
    'night': ('夜间环境音.wav', 522299, 'Defelozedd94', 'Crickets at Night', 'CC0'),
    'wind': ('风呼啸声.wav', 877970, 'Qvantor', 'Wind Howling Synthesized', 'CC0'),
    'music': ('替换和弦可商用需署名Daniel_Brooks.mp3', 876810, 'Daniel_Brooks', 'Dark Horror Soundscape', 'CC BY 4.0'),
}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def active_start(ffmpeg, source):
    """Use the first audible onset, retaining a short lead-in instead of silence."""
    result = subprocess.run([ffmpeg, '-v', 'error', '-i', str(source), '-ac', '1',
                             '-ar', '8000', '-f', 'f32le', '-'], capture_output=True, check=True)
    samples = array('f', result.stdout)
    energies = [sum(v * v for v in samples[i:i + 160]) / 160
                for i in range(0, len(samples) - 159, 160)]
    threshold = max(energies) * 0.06
    index = next((i for i, value in enumerate(energies) if value >= threshold), 0)
    return max(0., index * 0.02 - 0.03)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ffmpeg', default=shutil.which('ffmpeg'))
    options = parser.parse_args()
    if not options.ffmpeg:
        parser.error('Pass --ffmpeg with the FFmpeg executable path')
    ffmpeg = options.ffmpeg
    DEST.mkdir(parents=True, exist_ok=True)
    offsets = {key: active_start(ffmpeg, SOURCE / data[0]) for key, data in SOURCES.items()}
    # Cue, source, duration, optional processing. Ambient beds use longer crossfades.
    cues = [
        ('door_chime.wav', 'door', 1.58, ''),
        ('door_open.wav', 'open', 1.79, ''),
        ('cloth_rustle.wav', 'cloth', 0.70, ''),
        ('item_pickup.wav', 'pickup', 0.45, ''),
        ('item_drop.wav', 'drop', 0.80, ''),
        ('item_throw.wav', 'throw', 0.50, ''),
        ('task_mop.wav', 'mop', 0.90, ''),
        ('task_tidy.wav', 'tidy', 0.80, ''),
        ('task_cook.wav', 'cook', 2.20, ''),
        ('heartbeat.wav', 'heartbeat', 0.75, ''),
        ('heartbeat_fast.wav', 'heartbeat', 0.75, 'atempo=1.3'),
        ('heartbeat_reverb.wav', 'heartbeat', 0.75, 'aecho=0.8:0.5:110|230:0.25|0.14'),
        ('jumpscare_reveal.wav', 'reveal', 1.50, ''),
        ('phase_shift.wav', 'texture', 1.20, ''),
        ('ambient_night.wav', 'night', 20., ''),
        ('ambient_otherworld.wav', 'wind', 20., ''),
        ('ambient_threat.wav', 'music', 20., ''),
    ]
    manifest = {'pull_request': 1, 'pr_revision': '671fe366cda6294c2fe90d2ccc63e49b6c5d7427',
                'base_revision': '1075c73', 'format': '48000 Hz mono PCM signed 16-bit', 'cues': []}
    for filename, key, duration, effect in cues:
        original, number, author, title, license_name = SOURCES[key]
        source = SOURCE / original
        ambient = filename.startswith('ambient_')
        start = 0. if ambient else offsets[key]
        filters = [f'atrim=start={start:.3f}:duration={duration}', 'asetpts=PTS-STARTPTS',
                   'highpass=f=30', f'loudnorm=I={-26 if ambient else -20}:TP=-3:LRA=7']
        if effect:
            filters.append(effect)
        final_duration = duration / 1.3 if effect == 'atempo=1.3' else duration
        filters += [f'apad=whole_dur={final_duration}', f'atrim=duration={final_duration}',
                    'aresample=48000', f'afade=t=in:d={0.8 if ambient else 0.008}',
                    f'afade=t=out:st={final_duration - (0.8 if ambient else 0.04):.4f}:d={0.8 if ambient else 0.04}']
        target = DEST / filename
        subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(source), '-af', ','.join(filters),
                        '-map_metadata', '-1', '-ac', '1', '-ar', '48000', '-c:a', 'pcm_s16le',
                        str(target)], check=True)
        with wave.open(str(target)) as wav:
            assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 2, 48000)
            assert wav.getnframes() > 0
            assert max(abs(v) for v in array('h', wav.readframes(wav.getnframes()))) > 100
        manifest['cues'].append({'runtime_file': filename, 'source_file': original,
                                'source_sha256': digest(source), 'runtime_sha256': digest(target),
                                'title': title, 'creator': author, 'source_url': f'https://freesound.org/s/{number}/',
                                'license': license_name,
                                'license_url': 'https://creativecommons.org/licenses/by/4.0/' if license_name == 'CC BY 4.0'
                                               else 'https://creativecommons.org/publicdomain/zero/1.0/',
                                'offset_seconds': round(start, 3), 'duration_seconds': final_duration,
                                'modifications': 'Trimmed, mono 48 kHz PCM16, high-pass, loudness normalization, fades'
                                                 + (', ' + effect if effect else '')})
    (SOURCE / 'runtime_manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    credits = ['# PR #1 runtime audio credits', '', 'These converted cues are included in the playable PR audio preview.',
               'Source audio was trimmed, converted to mono 48 kHz PCM16, high-pass filtered, loudness normalized and faded.',
               'Fast heartbeat additionally changes tempo; Otherworld heartbeat adds echo.', '']
    for cue in manifest['cues']:
        credits += [f"- `{cue['runtime_file']}` — {cue['title']} by {cue['creator']}. {cue['source_url']} "
                    f"{cue['license']}: {cue['license_url']}"]
    credits += ['', 'Existing scanner, light flicker, ward-off and ordinary footsteps retain the credits in THIRD_PARTY_NOTICES.md.',
                'The unverified ordinary-footstep and background-music candidates from PR #1 are not used in this playable package.', '']
    (SOURCE / 'RUNTIME_CREDITS.md').write_text('\n'.join(credits), encoding='utf-8')
    print(f'Prepared {len(cues)} runtime cues from {len(SOURCES)} PR sources')


if __name__ == '__main__':
    main()
