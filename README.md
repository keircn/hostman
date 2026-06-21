# Hostman

A command line image host manager

[![Build and Release](https://github.com/keircn/hostman/actions/workflows/build.yml/badge.svg)](https://github.com/keircn/hostman/actions/workflows/build.yml)

## Installation

### Package managers

- **Arch Linux** - Install from the AUR: `paru -S hostman/hostman-bin`
- **Gentoo** - Available via the [roxy-overlay](https://codeberg.org/key/roxy-overlay) portage overlay: `eselect repository enable roxy-overlay && emerge -a hostman`

### Pre-built binaries

Download the latest binary from the [releases page](https://github.com/keircn/hostman/releases/latest).

### Build from source

See [BUILD.md](./BUILD.md) for build instructions.

### Quick Start

1. Run `hostman add-host` to configure your first hosting service
2. Use `hostman upload image.png` to upload a file
3. See `hostman list-uploads` to view your upload history

Built-in presets are available for popular hosts via `hostman add-preset`.

## License

This project is licensed under the MIT License.
