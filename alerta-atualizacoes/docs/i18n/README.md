# Internacionalização (i18n)

* **API:** KF6 KI18n (`i18n()`, `i18nc()`), com
  `KLocalizedString::setApplicationDomain("system-upgrade")` antes do
  `QApplication` (`src/main.cpp`).
* **Domínio gettext:** `system-upgrade` (nome do pacote Debian).
* **Idioma-fonte:** inglês. A interface brasileira está em `po/pt_BR.po`, com o
  texto português original.
* **Idioma em execução:** o do Plasma (`LANGUAGE`/`LANG`); nada é forçado. Os
  comandos `apt-get`/`flatpak` continuam com `LC_ALL=C`/`LANG=C` porque a saída
  deles é analisada (diagnóstico, não interface).

## Layout

```
po/
  i18n.sh              script padrão Mainuan (extract/update/validate/…)
  i18n.conf            DOMAIN=system-upgrade
  POTFILES.in          arquivos com textos traduzíveis
  LINGUAS              idiomas instalados (hoje: pt_BR)
  system-upgrade.pot   modelo gerado (versionado)
  pt_BR.po
data/applications/system-upgrade.desktop.in   modelos: o build gera os
data/autostart/system-upgrade.desktop.in       arquivos finais com Name[xx]…
data/metainfo/io.github.aratie_os.system_upgrade.metainfo.xml.in
```

O CMake compila cada `po/<lang>.po` listado em `LINGUAS` para
`<build>/locale/<lang>/LC_MESSAGES/system-upgrade.mo`, instala em
`/usr/share/locale/<lang>/LC_MESSAGES/` e gera `.desktop`/metainfo traduzidos
com `msgfmt --desktop` / `msgfmt --xml` (alvo `translations`). Dependências de
build: `gettext`, `libkf6i18n-dev`.

## Tarefas comuns

```sh
po/i18n.sh extract      # depois de mudar textos no código ou nos .in
po/i18n.sh update       # mescla o POT em todos os catálogos
po/i18n.sh validate     # msgfmt --check + placeholders (CI)
po/i18n.sh check-pot    # falha se o POT estiver desatualizado (CI)
po/i18n.sh coverage
po/i18n.sh add es       # novo idioma (Plural-Forms verificado) + LINGUAS
```

## Testar

```sh
cmake -S . -B build -G Ninja && cmake --build build
ctest --test-dir build -R i18n --output-on-failure
```

`i18n_tests_pt_BR`, `i18n_tests_en` e `i18n_tests_C` constroem o alerta real
(`QT_QPA_PLATFORM=offscreen`) e conferem título, mensagem, botões e nome
acessível, além do `.desktop`/metainfo gerados. O teste pt_BR usa
`LANG=en_US.UTF-8 LANGUAGE=pt_BR`; o gettext ignora `LANGUAGE` quando o locale
efetivo é `C`, então, num chroot sem `en_US.UTF-8`, a verificação portuguesa
é pulada (SKIP) em vez de falhar.

Para ver o programa traduzido a partir do build, sem instalar:

```sh
SYSTEM_UPGRADE_LOCALEDIR=$PWD/build/locale LANGUAGE=pt_BR build/system-upgrade
```

## Particularidades

* Notas de versão (`<release><description translate="no">`) e o nome do
  desenvolvedor no metainfo não são traduzidos.
* `organizationName` (`Aratie OS`) e o ID AppStream
  `io.github.aratie_os.system_upgrade` são marcas legadas mantidas de propósito:
  mudar o primeiro altera o caminho de configuração do usuário; mudar o segundo
  quebra o AppStream.
* A página de manual (`docs/system-upgrade.1`) não é traduzida.
