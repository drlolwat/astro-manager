# Firmware research boundary

Firmware flashing is deliberately excluded from supported releases.

Before any experimental updater can be enabled, the project must independently
establish all of the following:

- Official update-container structure and signature behavior
- Base and headset bootloader entry/exit commands
- Packet sequencing, checksums, acknowledgements, and retry rules
- Compatibility checks between base and headset images
- Behavior under USB loss, power loss, and an interrupted radio transfer
- A repeatable recovery procedure tested on non-primary hardware

Research should capture the official updater in an isolated Windows VM, build
an offline parser and simulated transport first, and never redistribute vendor
firmware. Experimental work must remain compile-time gated until interruption
and recovery testing succeeds.
