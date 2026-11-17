<div align="center">

<img src="https://raw.githubusercontent.com/mainuan/.github/main/profile/assets/mainuan-logo.svg" width="96" alt="Mainuan logo">

# Update Alert (system-upgrade)

Tells you at Plasma login when APT or Flatpak updates exist, and opens Discover if you choose to update.

[![CI](https://github.com/mainuan/alerta-atualizacao/actions/workflows/ci.yml/badge.svg)](https://github.com/mainuan/alerta-atualizacao/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/mainuan/alerta-atualizacao)](https://github.com/mainuan/alerta-atualizacao/releases/latest)
[![License](https://img.shields.io/github/license/mainuan/alerta-atualizacao)](LICENSE)
![Platform](https://img.shields.io/badge/Kubuntu%2026.04%20%7C%20Plasma%206.6-1d99f3)

[Português](README.md) · **English**

</div>

---

## About

`system-upgrade` is a compact C++/Qt 6 update alert. At login it refreshes the APT indexes, checks APT and Flatpak (system and user) and exits silently when nothing is found. Otherwise it shows a small window in the bottom-right corner of the primary screen. It never installs anything: **Update** opens KDE Discover's update page. Snap is deliberately not used.

## Features

- APT check by simulation, counting only `Inst` lines (respects Ubuntu phased updates).
- Flatpak check for system and user installations, without running `flatpak update`.
- Query failures are logged and never reported as updates.
- Exits immediately in Casper Live sessions.
- Wayland placement through LayerShellQt; X11 placement from the available screen area.

## Installation

Preinstalled on the Mainuan ISO. To install manually, download `system-upgrade_2.0.5_amd64.deb` and `SHA256SUMS` from the [latest release](https://github.com/mainuan/alerta-atualizacao/releases/latest):

```bash
sha256sum --check --ignore-missing SHA256SUMS
sudo apt install ./system-upgrade_2.0.5_amd64.deb
```

## Usage

Runs automatically at each Plasma login. To check now, open **System Updates** from the menu or run `system-upgrade` (no options). The package installs a sudoers rule that allows, without a password, only `/usr/bin/apt-get -o Acquire::Retries=3 -o APT::Update::Error-Mode=any update`. Manual page: `man system-upgrade`.

## Technical information

| Item | Value |
|---|---|
| Language | C++ |
| Toolkit | Qt 6, LayerShellQt, KF6 KI18n |
| Base | Kubuntu 26.04 LTS, KDE Plasma 6.6 |
| Package | `system-upgrade` (Debian, `amd64`) |

Sources live in [`alerta-atualizacoes/`](alerta-atualizacoes); see its [technical README](alerta-atualizacoes/README.md).

## Build and test

```bash
cd alerta-atualizacoes
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build --output-on-failure
dpkg-buildpackage -us -uc -b
```

Full CI sequence: [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## Translations

33 languages, English as the source language. See [alerta-atualizacoes/docs/i18n/README.md](alerta-atualizacoes/docs/i18n/README.md).

## Contributing

Use [Issues](https://github.com/mainuan/alerta-atualizacao/issues) and pull requests to `main`.

## Credits

Rafael Ruscher ([@ruscher](https://github.com/ruscher)) and Daigo ([@daigoasuka](https://github.com/daigoasuka)). Based on the `system-upgrade` 1.0.4 shell notifier by Elton Fabricio ([@eltonfabricio10](https://github.com/eltonfabricio10)).

## License

[GPL-3.0-or-later](LICENSE).
