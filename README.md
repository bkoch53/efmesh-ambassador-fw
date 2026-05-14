# EFMesh Ambassador Firmware

Customized Meshtastic firmware for the **Electric Forest Meshtastic
Community** ([efmesh.com](https://efmesh.com)). Flash this on a spare
Heltec V3 or V4 and it acts as a permanent "front door" beacon: it
introduces itself to every newly-discovered neighbour node and DMs them
the EFMesh invite. Other users see it in their node list as `⚠ Join 100+
EFMesh.com nodes on MF!`, which doubles as a self-explanatory CTA.

> **Not officially endorsed by Electric Forest, Insomniac, or any
> Electric Forest organizer.** EFMesh is a community-run Meshtastic mesh
> for festival attendees. The warning-triangle short name is a
> deliberate eye-catcher in the node list, not a safety alert.

## Compatibility

**Heltec V3 and Heltec V4 only.** Other Meshtastic boards (T-Beam,
Station G2, RAK, nRF52 family, etc.) have not been tested and the
prebuilt binaries below will brick them. If you want this on another
board, build from source per the instructions below at your own risk.

## What it does

Built on top of [ascendr's ambassador
module](https://github.com/ascendr/meshtastic/tree/main/ambassador) plus
three EFMesh-specific customizations:

| What | Value |
|------|-------|
| Owner short name | `⚠` (U+26A0 warning sign) |
| Owner long name | `Join 100+ EFMesh.com nodes on MF!` |
| First canned message (= ambassador invite) | `Hello! I'd like to invite you to join the Electric Forest Meshtastic Community (efmesh.com) over on MediumFast preset.` |

When a new node joins the mesh, the ambassador module exchanges node
info to get the new node's public key, then DMs them the invite
(prefixed with `Hello <their-shortname>, `).

The invite is sent **once per node** and **as a direct message** (not on
a channel) — this is non-spammy by design.

## Flash a Heltec V3 or V4

You have two options.

### Option A — Web flasher (easiest)

1. Plug your Heltec V3 / V4 into your computer via USB-C.
2. Build the binary locally (see **Build from source** below) or grab a
   community-built binary from the
   [releases](https://github.com/efmesh/efmesh-ambassador-fw/releases)
   page once available.
3. Open the [Meshtastic Web
   Flasher](https://flasher.meshtastic.org/).
4. Choose **"Custom firmware"** in the firmware dropdown.
5. Upload the `.bin` file you got in step 2.
6. Click **Connect** and pick the USB serial device that appears.
7. Click **Flash**. Wait for the progress bar; the device will reboot.

After flashing, the node should boot up identifying itself as `⚠ Join
100+ EFMesh.com nodes on MF!` on whatever frequency / preset your region
uses. Use the Meshtastic phone app to switch it onto **MediumFast** if
that isn't already the regional default.

### Option B — esptool CLI (terminal)

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
# output: ./build-out/heltec-v3/firmware.bin
```

The build script:

1. Clones the upstream Meshtastic firmware ambassador branch
   (`ascendr/firmware @ ambassador`) into a working directory.
2. Overlays the EFMesh canned-message string into
   `src/modules/CannedMessageModule.cpp`.
3. Copies our [`src/userPrefs.jsonc`](./src/userPrefs.jsonc) over the
   upstream one to bake in owner short/long name defaults.
4. Runs `pio run -e heltec-v<n>` to compile.
5. Copies the resulting `firmware.bin` into `./build-out/<board>/`.

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
"USERPREFS_CONFIG_OWNER_LONG_NAME": "Join 100+ EFMesh.com nodes on MF!"
```

33 bytes including the closing `!`, fits well inside the 39-byte
display budget of the 40-byte protobuf field. Format choice notes:

- `EFMesh.com` not `efmesh.org` — `efmesh.com` is the canonical EFMesh
  community website. `.org` does not resolve.
- `MF` instead of `MediumFast` — saves 8 bytes and matches the Forest
  community's existing slang for the preset.

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
