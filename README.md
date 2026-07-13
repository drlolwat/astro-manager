# Astro Manager for Linux

An open-source Astro A50 Gen 4 control center and binaural surround manager for
Arch Linux, CachyOS, PipeWire, and KDE Plasma.

Astro Manager combines native dock/headset controls with the existing Astro
Spatial 5.1/7.1 HRTF pipeline. It can route direct or spatialized program audio
to either **A50 Chat** or **A50 Game**; Chat remains the default. Spatial audio
defaults to the bass-forward **Bass Heavy** Linux EQ.

This is conventional multichannel PCM virtualization, not a Dolby Atmos object
decoder. It provides the headphone result people generally want from Atmos for
Headphones without accepting proprietary Atmos metadata.

## Current hardware scope

The supported device is the Astro A50 Gen 4 base station with USB ID
`9886:002c`. The native manager uses only HID interface 6; the USB audio
interfaces remain owned by PipeWire/ALSA.

Firmware versions are readable. **Firmware flashing is not implemented** because
the update bootloader, recovery process, and firmware container have not been
safely decoded.

## Features

- Battery, charging, headset-on, and docked status
- Base and headset firmware versions
- All three onboard EQ slots, names, and five bands
- Microphone level, sidetone, noise gate, and microphone EQ
- Stream Port Mix for Mic, Chat, Game, and Aux
- Live and startup Game/Voice balance
- Live preview with explicit **Save to Headset** and **Revert**
- Named unified profiles with versioned JSON import/export
- Real PipeWire 5.1 and 7.1 sinks with SOFA/KEMAR HRTF processing
- Selectable A50 Chat or A50 Game physical destination
- Bass Heavy, Balanced Warm, and Flat Linux EQ presets
- LFE filtering, latency alignment, and true-peak limiting
- Direct Stereo without application DSP
- KDE/Kirigami GUI, tray, systemd user service, D-Bus API, and CLI
- Channel tests, reconnect restoration, and host-side lossless checks

## Linux EQ priority

When Linux 5.1/7.1 processing is active, Astro Manager temporarily runs the
active onboard output EQ with zero gain. The user's real onboard preset remains
in memory and is restored in Direct mode or when the service exits cleanly.
Temporary neutralization never invokes the headset save command.

Before neutralizing, the manager writes a recovery journal under
`~/.local/state/astro-manager/`. If the process or desktop crashes, the next
launch recovers the desired gains only when both firmware versions still match.

Saving while spatial mode is active restores the desired values, sends one
headset save command, verifies the persistent values, and reapplies temporary
neutralization.

## Requirements

- Arch Linux or CachyOS with PipeWire and WirePlumber 0.5
- Qt 6 and Kirigami 6
- libusb 1.0
- `libmysofa` and `lsp-plugins-lv2`
- Astro A50 Gen 4 dock in PC mode

```bash
sudo pacman -S --needed cmake ninja qt6-base qt6-declarative kirigami \
  pipewire pipewire-audio pipewire-pulse wireplumber libusb libmysofa \
  lsp-plugins-lv2
```

## Build and install

```bash
git clone https://github.com/drlolwat/astro-manager.git
cd astro-manager
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
systemctl --user enable --now astro-manager.service
```

Reload the udev rule and reconnect the dock after the first install:

```bash
sudo udevadm control --reload-rules
```

The `astro-spatial` command remains as a compatibility alias during the 1.x
series. Existing mode, output, HRTF, and Linux EQ preferences migrate
automatically.

## Use

1. Put the dock in PC mode.
2. Open **Astro Manager for Linux**.
3. Select **A50 Chat** or **A50 Game** on the Spatial Audio page.
4. Select Direct Stereo, Spatial 5.1, or Spatial 7.1.
5. For spatial modes, set the headset Dolby button to white-star/source
   passthrough to prevent double spatialization.
6. Adjust hardware controls freely. Use **Save to Headset** only when the
   preview should survive a dock power cycle.

The background service keeps device monitoring, spatial routing, physical
button synchronization, and EQ restoration active with the window closed.

## CLI

```bash
astro-managerctl status
astro-managerctl output chat
astro-managerctl mode 7.1
astro-managerctl refresh
astro-managerctl profile PROFILE_UUID
astro-managerctl capture "My profile"
astro-managerctl delete-profile PROFILE_UUID
astro-managerctl save
astro-managerctl revert
```

Legacy commands also work:

```bash
astro-spatial --background --output chat --mode 7.1
```

## Lossless boundary

The observed A50 playback endpoints accept stereo S16LE at 48 kHz. Direct mode
stops the HRTF service, targets the selected physical endpoint, and sets its
PipeWire volume to unity. A 48 kHz/16-bit source can therefore remain
bit-identical through the host-side Linux conversion path. Other rates or bit
depths require conversion.

Hardware EQ and proprietary processing occur after the USB boundary. The radio
link between dock and headset cannot be inspected by Linux. Astro Manager never
claims that the complete wireless path is proven bit-perfect.

Spatial modes intentionally change samples through HRTF convolution, LFE
filtering, mixing, EQ, limiting, and final format conversion.

## Troubleshooting

```bash
systemctl --user status astro-manager.service
systemctl --user status astro-manager-dsp.service
journalctl --user -u astro-manager.service -b
wpctl status
astro-managerctl status
```

If hardware controls report a permission error, reconnect the dock after
installing `60-astro-manager.rules`. If another manager owns interface 6, close
it before starting Astro Manager.

## Protocol credit

The Gen 4 command framing and decoded settings are based on the MIT-licensed
[tdryer/eh-fifty](https://github.com/tdryer/eh-fifty) project. The implementation
here is a native C++/libusb port with additional validation, state separation,
runtime EQ neutralization, recovery, UI, profiles, and PipeWire integration.

See [NOTICE](NOTICE) for attribution.

## License

[MIT](LICENSE)
