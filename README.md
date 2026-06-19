# EFMesh Ambassador Firmware

Customized Meshtastic firmware for the **Electric Forest Meshtastic
Community** ([efmesh.com](https://efmesh.com)). Flash this on a spare
Heltec V3/V4 or a Seeed SenseCAP T1000-E and it acts as a permanent "front door" beacon: it
introduces itself to every newly-discovered neighbour node and DMs them
the EFMesh invite. Other users see it in their node list as `⚠ Join 100+
EFMesh.com on MediumFast!`, which doubles as a self-explanatory CTA.

> **Not officially endorsed by Electric Forest, Insomniac, or any
> Electric Forest organizer.** EFMesh is a community-run Meshtastic mesh
> for festival attendees. The warning-triangle short name is a
> deliberate eye-catcher in the node list, not a safety alert.

## Compatibility

**Heltec V3, Heltec V4, and Seeed SenseCAP T1000-E only.** Other Meshtastic boards (T-Beam,
Station G2, RAK, other nRF52 family boards, etc.) have not been tested and the
prebuilt binaries below may brick them. If you want this on another
board, build from source per the instructions below at your own risk.

The T1000-E is different from the Heltec boards: it is an nRF52 UF2
target, so the build uses Meshtastic's `seeed_wio_tracker_L1` PlatformIO
environment and produces `firmware.uf2` instead of an ESP32-S3
`firmware.bin`.

## What it does

Built on top of [ascendr's ambassador
module](https://github.com/ascendr/meshtastic/tree/main/ambassador) plus
three EFMesh-specific customizations:

| What | Value |
|------|-------|
| Owner short name | `⚠` (U+26A0 warning sign) |
| Owner long name | `Join 100+ EFMesh.com on MediumFast!` |
| First canned message (= ambassador invite) | `Hello! I'd like to invite you to join the Electric Forest Meshtastic Community (efmesh.com) over on MediumFast preset.` |

When a new node joins the mesh, the ambassador module exchanges node
info to get the new node's public key, then DMs them the invite
(prefixed with `Hello <their-shortname>, `).

The invite is sent **once per node** and **as a direct message** (not on
a channel) — this is non-spammy by design.

## Flash a Heltec V3/V4 or T1000-E

You have three options.

### Option A — GitHub Actions / Releases (recommended)

Every commit builds all supported firmware artifacts in GitHub Actions. Commits merged to `main` or `master` automatically update the rolling `latest` release, while version tags publish stable releases. This is the easiest way to build the T1000-E UF2 when a local machine cannot clone or compile the upstream firmware.

1. Merge or push a commit to run the **Build firmware and release** workflow automatically.
2. For a release build, merge to `main`/`master` to update the rolling `latest` release, push a `v*` tag for a stable release, or run the workflow manually with **Create or update a GitHub Release** enabled.
3. Download the generated release or workflow artifact for your board:
   - `efmesh-ambassador-heltec-v3.bin` for Heltec V3
   - `efmesh-ambassador-heltec-v4.bin` for Heltec V4
   - `efmesh-ambassador-t1000-e.uf2` for Seeed SenseCAP T1000-E
4. Flash the downloaded `.bin` or `.uf2` using one of the options below.

Pushing any branch commit builds all three boards and stores workflow artifacts. Merging to `main` or `master` additionally updates the rolling `latest` GitHub Release, and pushing a tag that starts with `v`, for example `v1.0.0`, publishes a stable release with the same assets.

### Option B — Web flasher (easiest local flashing)

1. Plug your Heltec V3/V4 into your computer via USB-C, or put the T1000-E into DFU mode so it appears as a `T1000-E` drive.
2. Build the firmware locally (see **Build from source** below) or grab a
   community-built binary from the
   [releases](https://github.com/efmesh/efmesh-ambassador-fw/releases)
   page.
3. Open the [Meshtastic Web
   Flasher](https://flasher.meshtastic.org/).
4. Choose **"Custom firmware"** in the firmware dropdown.
5. Upload the `.bin` file for Heltec or the `.uf2` file for T1000-E that you got in step 2.
6. Click **Connect** and pick the USB serial device that appears.
7. Click **Flash**. Wait for the progress bar; the device will reboot.

For T1000-E UF2 flashing, copy `build-out/t1000-e/firmware.uf2` to the mounted DFU drive if you are not using the web flasher. After flashing, the node should boot up identifying itself as `⚠ Join
100+ EFMesh.com on MediumFast!` on whatever frequency / preset your
region uses. Use the Meshtastic phone app to switch it onto
**MediumFast** if that isn't already the regional default.

### Option C — esptool CLI (terminal)

For folks who prefer not to touch a web flasher.

```bash
# 1. install esptool
pip install esptool

# 2. find the serial port your Heltec shows up as
ls /dev/cu.usbserial-* /dev/ttyUSB*   # mac / linux
# windows: check Device Manager for the COMx port

# 3. erase the flash (clears any prior Meshtastic config)
esptool.py --chip esp32s3 --port /dev/cu.usbserial-XXXX erase_flash

# 4. flash the EFMesh ambassador binary
esptool.py --chip esp32s3 --port /dev/cu.usbserial-XXXX \
  --baud 921600 write_flash 0x0000 firmware.bin
```

Replace `firmware.bin` with the actual filename. Replace
`/dev/cu.usbserial-XXXX` with your port. After flashing, unplug + replug
the device.

> If your Heltec V3/V4 isn't recognised, you may need the [CP210x
> drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

## Build from source

PlatformIO does the work. From a clean machine:

```bash
git clone https://github.com/efmesh/efmesh-ambassador-fw.git
cd efmesh-ambassador-fw
pip install platformio
./scripts/build.sh heltec-v3   # or: ./scripts/build.sh heltec-v4
./scripts/build.sh t1000-e     # Seeed SenseCAP T1000-E
# outputs: ./build-out/heltec-v3/firmware.bin or ./build-out/t1000-e/firmware.uf2
```

The build script:

1. Clones the upstream Meshtastic firmware ambassador branch
   (`ascendr/firmware @ ambassador`) into a working directory.
2. Overlays the EFMesh canned-message string into
   `src/modules/CannedMessageModule.cpp`.
3. Copies our [`src/userPrefs.jsonc`](./src/userPrefs.jsonc) over the
   upstream one to bake in owner short/long name defaults.
4. Runs the matching PlatformIO environment (`heltec-v<n>` for Heltec or
   `seeed_wio_tracker_L1` for T1000-E) to compile.
5. Copies the resulting firmware artifact into `./build-out/<board>/`
   (`firmware.bin` for Heltec, `firmware.uf2` for T1000-E).

## How the customizations work

There are three customization points; each maps to one file in this repo.

### 1. Invite message — `src/modules/CannedMessageModule.cpp`

The ambassador module sends the **first canned message** as the invite.
Default canned messages are seeded by `installDefaultCannedMessageModuleConfig()`
in upstream `CannedMessageModule.cpp`. We replace its `defaultMessages`
string. The exact line is shown in
[`patches/0002-efmesh-canned-message.patch`](./patches/0002-efmesh-canned-message.patch).

Users can override this at runtime via the Meshtastic phone app
(Settings → Canned Messages). Whatever is at slot 0 (or slot 1 if slot 0
is "[-- Free Text --]") wins. We just set a sensible default at flash time.

### 2. Owner short name — `src/userPrefs.jsonc`

```
"USERPREFS_CONFIG_OWNER_SHORT_NAME": "⚠"
```

We use the bare warning sign **U+26A0** (3 utf-8 bytes), NOT
`⚠️` (warning sign + variation selector U+FE0F, 6 utf-8 bytes). The
Meshtastic protobuf `short_name` field is fixed at `char[5]`, which only
fits 4 displayable bytes plus null terminator — the full emoji-style
variant does not fit.

This is **not a downgrade**: the Heltec V3/V4 1-bit monochrome OLED
renders the bare warning glyph from its built-in font regardless of the
variation selector. The phone-app node list also shows the bare warning
sign. The variation selector only matters on full-color emoji renderers
(messaging apps) — none of which the Meshtastic node UI uses.

### 3. Owner long name — `src/userPrefs.jsonc`

```
"USERPREFS_CONFIG_OWNER_LONG_NAME": "Join 100+ EFMesh.com on MediumFast!"
```

35 bytes including the closing `!`, fits well inside the 39-byte
display budget of the 40-byte protobuf field. Format choice notes:

- `EFMesh.com` is the canonical community website; the URL is part of
  the wordmark so anyone scanning the node list can find the guide.
- `MediumFast` spelled out for clarity in the node list (vs. the shorter
  `MF` abbreviation used in chat).

## Configuration after flashing

The bake-in defaults are **defaults**, not lock-ins. Anyone running this
firmware can override every value at runtime via the Meshtastic phone /
desktop app:

- **Node short/long name**: `Settings → User`
- **Invite text**: `Settings → Module Settings → Canned Messages`
  (edit the first message)
- **Region / preset**: `Settings → Radio Config → LoRa` — make sure
  you're set to **MediumFast** (the EFMesh community preset) and your
  correct regional frequency band before the node is useful on the
  EFMesh mesh.

## Credits

- Ambassador module + concept: [ascendr](https://github.com/ascendr).
  Original repo:
  [github.com/ascendr/meshtastic](https://github.com/ascendr/meshtastic/tree/main/ambassador).
  Original firmware fork:
  [github.com/ascendr/firmware @ ambassador](https://github.com/ascendr/firmware/tree/ambassador).
- Underlying firmware:
  [Meshtastic project](https://github.com/meshtastic/firmware).

## License

GPL-3.0-only — matches upstream. See [LICENSE](./LICENSE) and
[NOTICE](./NOTICE) for attribution detail.

## Disclaimer

**Not officially endorsed by, affiliated with, or sponsored by Electric
Forest, Insomniac, Madison House Presents, or any Electric Forest
organizer.** This is a community-run, attendee-organized Meshtastic
mesh. The warning emoji in the node short name is a deliberate
attention-grab for the node list, not a safety or emergency signal.
