# PR #1 playable audio preview

Base: released v0.6-preview (1075c73). Includes PR #1 head 671fe36 in an isolated
integration branch; main is not changed.

15 candidate sources become 17 runtime cues. Entering customers, pickup/drop/throw,
rule paper, cleaning/tidying/cooking, threat heartbeat, reveal and Q phase switching
now use PR recordings. Quiet ambient beds change between normal store, Otherworld
and active-monster states and stop outside active gameplay.

Ordinary-footstep and background-music candidates have unresolved source/processing
mappings and are not included in the playable runtime. Existing ordinary footsteps,
scanner, electrical flicker and ward-off audio remain.

Conversion uses mono 48 kHz PCM16, bounded clips, loudness normalization and fades.
CC BY credits, links and modification notes accompany the Windows package.

Source attribution fixes on this integration branch correct the reveal/UI pair,
the mop/wipe pair and the vocal-heartbeat filename. The original PR branch is untouched.

## Test focus

- Start a new game; listen to the paper and first shelf task, then the first customer chime.
- Pick up, drop and throw an item; compare their separate cues.
- Hold E to mop, tidy and cook; each action has its own work sound.
- Press Q during business and return to reality; compare the ambient beds.
- Approach the abnormal customer and trigger a reveal; check heartbeat/stinger balance.
- Pause, return to title or finish the night; check that ambience does not continue.

This is an audio integration preview, not a final mix or a full manual playthrough certification.
