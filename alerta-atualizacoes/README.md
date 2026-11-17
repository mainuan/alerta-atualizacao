# System Upgrade

`system-upgrade` e um alerta compacto de atualizacoes para KDE Plasma 6. No
inicio da sessao ele atualiza os indices APT, consulta atualizacoes APT e
Flatpak (system e user) e termina sem interface quando nao encontra nada.

Quando existe ao menos uma atualizacao, uma janela Qt 6 aparece no canto
inferior direito do monitor principal. `Atualizar` abre
`/usr/bin/plasma-discover --mode Update`; `Cancelar` encerra o programa.

## Comportamento

- Sessao Live Casper (`boot=casper` em `/proc/cmdline`): encerra antes de criar
  `QApplication`, lock, processos ou interface.
- APT: executa o refresh autorizado e consulta separadamente
  `apt-get -s -o Debug::NoLocking=1 upgrade` com `LC_ALL=C` e `LANG=C`.
  Apenas linhas iniciadas por `Inst ` representam atualizacoes; isso respeita
  pacotes adiados pelo mecanismo de phased updates.
- Flatpak: consulta `remote-ls --updates --columns=ref` nos escopos system e
  user, sem executar `flatpak update`.
- Falhas sao registradas em stderr e nunca sao tratadas como atualizacoes.
- Wayland: LayerShellQt ancora a janela em `Bottom | Right`, com margem de 24 px,
  layer `Top`, foco sob demanda e zona exclusiva zero.
- X11: a posicao usa `QScreen::availableGeometry()` com margem de 24 px.

O SVG preferencial da janela e
`/usr/share/icons/kora-cyan/status/scalable/package-broken.svg`, renderizado em
64 x 64 pelo Qt SVG. `dialog-warning` e usado somente quando esse arquivo nao
esta disponivel.

## Dependencias de build

No Kubuntu 26.04:

```sh
sudo apt install build-essential cmake ninja-build debhelper devscripts \
  lintian qt6-base-dev qt6-svg-dev liblayershellqtinterface-dev \
  libkf6i18n-dev gettext
```

## Compilacao e testes

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Os testes cobrem simulacao APT vazia, operacoes `Inst`, pacotes retidos por
phased updates, referencias Flatpak, combinacao final dos estados, parsing de
`boot=casper`, bloqueio integrado da inicializacao Live, posicionamento e o
ambiente de sessao repassado ao Discover, alem da maquina de estados completa
do verificador com comandos substitutos (inclusive comando que nao pode ser
iniciado). Os testes `i18n_tests_*` exibem o
alerta real em `pt_BR` e em ingles (ver `docs/i18n/README.md`).

## Pacote Debian

```sh
dpkg-buildpackage -us -uc -b
scripts/validate-package.sh ../system-upgrade_2.0.5_amd64.deb
lintian --fail-on error ../system-upgrade_2.0.5_amd64.changes
```

O CI (`.github/workflows/ci.yml`) usa o pipeline compartilhado
`mainuan/.github`: compila e testa em Ubuntu 26.04, gera o pacote, roda
lintian, instala num Ubuntu 26.04 limpo, executa o programa como usuario comum
(inclusive o refresh APT via sudoers) e remove/expurga o pacote. Releases sao
publicadas somente a partir de tags `v<versao>` iguais a `debian/changelog`.

O sudoers instalado permite exclusivamente o comando:

```text
/usr/bin/apt-get -o Acquire::Retries=3 -o APT::Update::Error-Mode=any update
```

O programa nao instala atualizacoes diretamente. A instalacao continua sob
responsabilidade do Discover.
