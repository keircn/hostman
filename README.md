# Hostman

A command line image host manager

[![Build and Release](https://github.com/keircn/hostman/actions/workflows/build.yml/badge.svg)](https://github.com/keircn/hostman/actions/workflows/build.yml)

## Installation

### Package managers

**Arch Linux** - Install from the AUR: [hostman](https://aur.archlinux.org/packages/hostman)

**Gentoo** - Available via the [roxy-overlay](https://codeberg.org/key/roxy-overlay) portage overlay:
```sh
eselect repository add roxy-overlay git https://codeberg.org/key/roxy-overlay.git
eselect repository enable roxy-overlay
emaint sync -r roxy-overlay
emerge -av net-misc/hostman # masked by ~amd64
```

Or download the latest binary from the [releases page](https://github.com/keircn/hostman/releases/latest).

### Build from source

See [BUILD.md](./BUILD.md) for build instructions.

## License

This project is licensed under the MIT License.
