# Local Achievements

RA2Snes can load a "local" achievement set for ROMs whose hash isn't
recognized by the RetroAchievements server — most commonly unlicensed
hacks and homebrew that were never (or aren't yet) added to the site,
like sets shared directly on the [forums](https://retroachievements.org/forums)
for a hack's own topic.

## How it works

1. When RA2Snes loads a game and the server doesn't recognize its MD5 hash
   (`GameID` comes back as `0`), it automatically looks for a matching file
   named `<hash>.txt` in a `LocalAchievements` folder next to the RA2Snes
   executable.
2. If found, those achievements are loaded and tracked exactly like a
   normal set — triggers run through the same rcheevos engine, unlocks pop
   up the same way, and they show up in the achievement list — just marked
   as local/unofficial.
3. Local sets are **never submitted to the server** (there's no matching
   record to submit to) and always run in softcore. Hardcore mode is
   automatically disabled while a local set is active, the same way it's
   disabled for cheats, save states, or a patched ROM.
4. Unlocks are saved to `<hash>.unlocks` next to the achievement file so
   your progress survives between sessions — this is purely local
   bookkeeping and isn't sent anywhere.

You can also load a file manually (useful for testing a set before
dropping it into `LocalAchievements`, or pointing at a file that isn't
named after the hash) — this is exposed to the UI as
`loadLocalAchievementsFile(filePath)`.

## Getting the ROM's hash

RA2Snes hashes the ROM the same way the server does: it reads the raw ROM
bytes and, if a 512-byte copier header is present, strips it before taking
the MD5. A quick way to get the same hash outside the app:

```python
import hashlib, sys

with open(sys.argv[1], "rb") as f:
    data = f.read()

if len(data) & 512:
    data = data[512:]

print(hashlib.md5(data).hexdigest())
```

## File format

One achievement per line:

```
ID:"MemAddr":Title:Description:Progress:ProgressMax:ProgressFormat:Author:Points:Created:Modified:Upvotes:Downvotes:BadgeName
```

- `ID` — any unique positive integer. By RetroAchievements convention,
  locally-authored sets use IDs starting at `111000001` so they'll never
  collide with a real, server-assigned ID, but RA2Snes doesn't require
  that numbering — everything loaded from a local file is flagged as local
  regardless of its ID.
- `MemAddr` — the trigger logic string, in quotes (it can contain colons
  itself, e.g. `ResetIf` clauses like `R:0x 000242!=10096_...`).
- `Progress` / `ProgressMax` / `ProgressFormat` — not currently used by
  RA2Snes; leave blank.
- `Author` — not currently displayed; leave blank or fill in for your own
  records.
- `Points` — achievement point value.
- `Created` / `Modified` / `Upvotes` / `Downvotes` — not used; leave blank.
- `BadgeName` — an existing RetroAchievements badge ID (badge art is
  pulled from the same public badge library the server uses, so you don't
  need to host your own images — pick any existing badge ID).

Blank lines and lines starting with `#` are ignored, so you can leave
notes or separate sections in the file.

### Example

```
# ~Hack~ MaternalBound Redux - unofficial set
111000001:"R:0x 000242!=10096_0x 000244=31710_0xH00b53b=119.1._0xH00b53b=21_0xH009870=5.1._0xH009870=0_0xH009871=6.1._0xH009871=0":Buzzkill:Mourn the loss of a friend::::staticnation:1:::::07940
111000002:"R:0x 009f8c!=130_0xH00b53b=102_0xH00a22d=0":Jumping the Sharks:Put an end to the Sharks' reign of tyranny::::staticnation:2:::::08280
```

This is the same format used in achievement sets people already post on
the RetroAchievements forums for unofficial hacks, so in many cases you
can copy one straight out of a forum post, save it as `<hash>.txt`, and
drop it in `LocalAchievements`.

## Limitations

- No leaderboards — only achievement triggers are supported for local
  sets.
- No rich presence unless you also patch the corresponding fields in code
  (not exposed through the local file format today).
- Local unlocks are per-install bookkeeping only; they don't sync anywhere
  and aren't reflected on your RetroAchievements profile.
