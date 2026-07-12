# Astro Spatial

Open-source binaural 5.1/7.1 virtualization for the Astro A50 on PipeWire.

Astro Spatial is a CachyOS/Arch desktop controller for an Astro A50 dock. It creates a real PipeWire 5.1 or 7.1 sink, spatializes each speaker with a SOFA HRTF, and sends the resulting stereo signal to either **A50 Chat** or **A50 Game**. It also provides a direct stereo mode with no application DSP.

Surround defaults to the **Bass Heavy** equalizer: a strong low shelf and sub-bass lift combined with reduced upper-mid/treble energy. **Balanced Warm** and **Flat / EQ Off** are available in the GUI. EQ runs before the true-peak limiter.

This is binaural virtualization for conventional multichannel PCM. It is not a Dolby Atmos decoder and does not consume proprietary Atmos object metadata.

## Features

- Real 5.1 and 7.1 PipeWire sinks for games and media players
- SOFA/KEMAR HRTF spatialization to stereo headphones
- Selectable A50 Chat or A50 Game physical output
- Bass Heavy, Balanced Warm, and Flat equalizer presets
- LFE filtering, latency alignment, and true-peak limiting
- Direct stereo mode without application DSP
- KDE/Kirigami controller, tray integration, channel tests, and reconnect restoration
- Communication-role stream exclusion so voice chat and the microphone stay outside the surround DSP

## Requirements

- CachyOS or Arch Linux with PipeWire and WirePlumber 0.5
- KDE Plasma/Wayland with Qt 6 and Kirigami
- Astro A50 dock exposed as USB device `9886:002c`
- `libmysofa` and `lsp-plugins-lv2`

Install the build and runtime dependencies:

```bash
sudo pacman -S --needed cmake ninja qt6-base qt6-declarative kirigami \
  pipewire pipewire-audio pipewire-pulse wireplumber libmysofa lsp-plugins-lv2
```

## Build and run

Clone the repository:

```bash
git clone https://github.com/drlolwat/astro-spatial.git
cd astro-spatial
```

Build and test:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/astro-spatial
```

Modes may also be selected without opening the window:

```bash
astro-spatial --background --output chat --mode 7.1
astro-spatial --background --output game --mode direct
```

To install it system-wide:

```bash
sudo cmake --install build
```

The first launch creates:

- `~/.config/astro-spatial/filter-chain.conf` when surround is selected
- `~/.config/systemd/user/astro-spatial-dsp.service`
- `~/.config/autostart/io.github.drlolwat.AstroSpatial.desktop`

No root privileges are used at runtime.

## Use

1. Put the dock in PC mode.
2. Select **A50 Chat** (the first-run default) or **A50 Game** as the physical destination.
3. Select Direct Stereo, Spatial 5.1, or Spatial 7.1.
4. For surround, set the headset's Dolby button to white-star/source-passthrough mode to prevent double spatialization.
5. Use the channel calibration buttons to confirm direction.

Communication-role playback streams are excluded from automatic DSP moves, and the microphone is never processed. If A50 Chat is selected, spatialized program audio and untouched communication audio may share the physical Chat endpoint.

## Lossless boundary

The observed A50 endpoints accept stereo S16LE at 48 kHz. Direct mode stops the Astro Spatial service, targets the selected hardware endpoint, and sets its PipeWire volume to unity. A 48 kHz/16-bit source can therefore remain bit-identical through the Linux software conversion path. Other rates or bit depths require conversion.

Surround is intentionally not bit-perfect: it performs HRTF convolution, LFE filtering, mixing, true-peak limiting, and final 16-bit dithering. The proprietary radio link and onboard headset processing after the USB dock cannot be inspected by Linux, so the application does not claim end-to-end wireless losslessness.

## Troubleshooting

Inspect the DSP service with:

```bash
systemctl --user status astro-spatial-dsp
journalctl --user -u astro-spatial-dsp -b
wpctl status
```

Returning to Direct Stereo stops the DSP service and routes eligible playback streams back to the selected physical endpoint.

## Current hardware scope

The initial release matches the Astro A50 dock with USB vendor/product ID `9886:002c` and PipeWire's standard `stereo-chat` / `stereo-game` node names. Other A50 generations may expose different identifiers and are not yet automatically detected.

## License

[MIT](LICENSE)
