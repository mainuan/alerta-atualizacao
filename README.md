<div align="center">

<img src="https://raw.githubusercontent.com/mainuan/.github/main/profile/assets/mainuan-logo.svg" width="96" alt="Logotipo do Mainuan">

# Alerta de Atualizações

Avisa no início da sessão do Plasma quando há atualizações APT ou Flatpak e abre o Discover se o usuário quiser atualizar.

[![CI](https://github.com/mainuan/alerta-atualizacao/actions/workflows/ci.yml/badge.svg)](https://github.com/mainuan/alerta-atualizacao/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/mainuan/alerta-atualizacao)](https://github.com/mainuan/alerta-atualizacao/releases/latest)
[![Licença](https://img.shields.io/github/license/mainuan/alerta-atualizacao)](LICENSE)
![Plataforma](https://img.shields.io/badge/Kubuntu%2026.04%20%7C%20Plasma%206.6-1d99f3)

**Português** · [English](README.en.md)

</div>

---

## O que é

O `system-upgrade` é um alerta compacto de atualizações escrito em C++ e Qt 6. No login, ele atualiza os índices do APT, procura atualizações APT e Flatpak (instalações de sistema e de usuário) e termina em silêncio se não houver nada. Quando há ao menos uma atualização, mostra uma pequena janela no canto inferior direito do monitor principal.

O programa não instala nada: **Atualizar** abre o KDE Discover na página de atualizações, e a instalação fica com ele. Snap e o backend Snap do Discover não são usados, por decisão do projeto.

```text
Login no Plasma → APT + Flatpak → sem atualizações: encerra
                               → com atualizações: alerta → Discover
```

## Funcionalidades

- Consulta APT por simulação (`apt-get -s upgrade`), contando só operações `Inst`; respeita as *phased updates* do Ubuntu.
- Consulta Flatpak de sistema e de usuário (`remote-ls --updates`), sem executar `flatpak update`.
- Falhas de consulta vão para o stderr e nunca são tratadas como atualização. Se a atualização dos índices falhar (rede ainda indisponível no login, um repositório com erro), a consulta APT usa as listas locais em vez de ser pulada.
- Na sessão Live (`boot=casper`), encerra antes de criar interface, lock ou processos.
- No Wayland, a janela é ancorada no canto inferior direito por LayerShellQt, sem roubar o foco (não é ativada ao aparecer); no X11, usa a área útil da tela e também aparece sem ativar a janela. Ambos com margem de 24 px.
- O Discover é aberto com o ambiente original da sessão Plasma.

## Instalação

O Alerta de Atualizações já vem instalado na ISO do Mainuan. Para instalar manualmente, baixe `system-upgrade_2.0.6_amd64.deb` e `SHA256SUMS` da [última release](https://github.com/mainuan/alerta-atualizacao/releases/latest) e, na pasta dos dois arquivos:

```bash
sha256sum --check --ignore-missing SHA256SUMS
sudo apt install ./system-upgrade_2.0.6_amd64.deb
```

## Uso

Não é preciso fazer nada: o autostart do Plasma executa a verificação a cada login. Para verificar na hora, abra **Atualizações do sistema** no menu de aplicativos (categoria Sistema) ou execute:

```bash
system-upgrade
```

O comando não tem opções. Na janela, **Atualizar** abre `plasma-discover --mode Update` e **Cancelar** fecha o alerta. Página de manual: `man system-upgrade`.

| Arquivo | Função |
|---|---|
| `/etc/xdg/autostart/system-upgrade.desktop` | inicia a verificação com a sessão Plasma (`OnlyShowIn=KDE`) |
| `/etc/sudoers.d/system-upgrade` | permite, sem senha, **apenas** o comando fixo de atualização de índices abaixo |

```text
/usr/bin/apt-get -o Acquire::Retries=3 -o APT::Update::Error-Mode=any update
```

## Informações técnicas

| Item | Valor |
|---|---|
| Linguagem | C++ |
| Toolkit | Qt 6 (Widgets, Svg), LayerShellQt, KF6 KI18n |
| Base | Kubuntu 26.04 LTS, KDE Plasma 6.6 (Wayland e X11) |
| Pacote | `system-upgrade` (Debian, `amd64`) |
| Dependências principais | `apt`, `flatpak`, `plasma-discover`, `plasma-discover-backend-flatpak`, `layer-shell-qt`, `sudo` |
| Arquitetura | amd64 |
| Ícone da janela | `kora-cyan/status/scalable/package-broken.svg`, com `dialog-warning` como alternativa |

O código e o pacote ficam em [`alerta-atualizacoes/`](alerta-atualizacoes). Detalhes de comportamento, testes e empacotamento estão no [README técnico](alerta-atualizacoes/README.md) e a arquitetura em [MIGRATION_PLAN.md](alerta-atualizacoes/MIGRATION_PLAN.md).

## Compilação e testes

Num Kubuntu 26.04, com as dependências de [`debian/control`](alerta-atualizacoes/debian/control). São os comandos que o [CI](.github/workflows/ci.yml) executa, sobre o pipeline Debian compartilhado da organização:

```bash
git clone https://github.com/mainuan/alerta-atualizacao.git
cd alerta-atualizacao/alerta-atualizacoes
sudo apt-get build-dep --no-install-recommends ./

scripts/check-version-consistency.sh       # debian/changelog, CMake e AppStream
po/i18n.sh check-pot && po/i18n.sh validate
/usr/sbin/visudo -cf data/system-upgrade.sudoers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure -V
dpkg-buildpackage -us -uc -b
lintian --fail-on error ../system-upgrade_2.0.6_amd64.deb
```

O teste de tradução `pt_BR` precisa do pacote `locales-all`, e o CI falha se algum teste for pulado. Depois, o CI instala o `.deb` num Ubuntu 26.04 limpo, confere os arquivos instalados e as permissões do sudoers, executa o programa como usuário comum (inclusive a atualização de índices via sudoers), reinstala, remove e expurga. A Release só é publicada por tag `v<versão>` igual à de `debian/changelog` e nunca substitui uma Release existente.

## Traduções

A interface está em 33 idiomas (catálogos em [`alerta-atualizacoes/po/`](alerta-atualizacoes/po), inglês como idioma-fonte): ar, bn, cs, da, de, el, es, fa, fi, fr, he, hi, hu, id, it, ja, ko, nb, nl, pl, pt, pt_BR, ro, ru, sv, sw, th, tr, uk, ur, vi, zh_CN e zh_TW.

Como traduzir: [alerta-atualizacoes/docs/i18n/README.md](alerta-atualizacoes/docs/i18n/README.md).

## Contribuição

Relate problemas em [Issues](https://github.com/mainuan/alerta-atualizacao/issues) e envie pull requests para `main`; cada PR passa pelo CI completo.

## Créditos

- **Rafael Ruscher** ([@ruscher](https://github.com/ruscher))
- **Daigo** ([@daigoasuka](https://github.com/daigoasuka))

Projeto de origem: o pacote `system-upgrade` 1.0.4 de **Elton Fabricio** ([@eltonfabricio10](https://github.com/eltonfabricio10)), um notificador em shell com `yad`, auditado em [AUDIT.md](alerta-atualizacoes/AUDIT.md) e reescrito em Qt 6.

## Licença

[GPL-3.0-or-later](LICENSE).
