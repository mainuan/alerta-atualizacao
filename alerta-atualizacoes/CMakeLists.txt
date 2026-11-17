cmake_minimum_required(VERSION 3.22)

project(system-upgrade VERSION 2.0.5 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(LayerShellQt REQUIRED)
find_package(KF6I18n 6.0 REQUIRED)
find_package(Gettext REQUIRED)

add_library(system_upgrade_logic STATIC
    src/discover_launcher.cpp
    src/discover_launcher.h
    src/startup.cpp
    src/startup.h
    src/update_checker.cpp
    src/update_checker.h
    src/update_state.cpp
    src/update_state.h
    src/window_placement.cpp
    src/window_placement.h
)
target_include_directories(system_upgrade_logic PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(system_upgrade_logic PUBLIC Qt6::Core Qt6::Gui)

add_executable(system-upgrade
    src/catalog_layout_direction.h
    src/main.cpp
    src/update_notification.cpp
    src/update_notification.h
)
target_compile_definitions(system-upgrade PRIVATE
    LAYERSHELLQTINTERFACE_NO_DEPRECATED_WARNINGS
)
target_link_libraries(system-upgrade PRIVATE
    system_upgrade_logic
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    LayerShellQt::Interface
    KF6::I18n
)

# Translations: gettext domain = Debian package name. po/<lang>.po is compiled
# into <build>/locale/<lang>/LC_MESSAGES/system-upgrade.mo (usable from the
# build tree through SYSTEM_UPGRADE_LOCALEDIR) and installed to
# ${CMAKE_INSTALL_LOCALEDIR}. The .desktop and metainfo files are generated
# from their .in templates with msgfmt so they carry every translation.
set(SYSTEM_UPGRADE_DOMAIN system-upgrade)
set(SYSTEM_UPGRADE_PO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/po")
set(SYSTEM_UPGRADE_LOCALE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale")
file(STRINGS "${SYSTEM_UPGRADE_PO_DIR}/LINGUAS" system_upgrade_linguas REGEX "^[^#]")
set(system_upgrade_po_files)
set(system_upgrade_translation_outputs)
foreach(language IN LISTS system_upgrade_linguas)
    string(STRIP "${language}" language)
    if(language STREQUAL "")
        continue()
    endif()
    set(po_file "${SYSTEM_UPGRADE_PO_DIR}/${language}.po")
    set(mo_directory "${SYSTEM_UPGRADE_LOCALE_BUILD_DIR}/${language}/LC_MESSAGES")
    set(mo_file "${mo_directory}/${SYSTEM_UPGRADE_DOMAIN}.mo")
    add_custom_command(
        OUTPUT "${mo_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${mo_directory}"
        COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" --check -o "${mo_file}" "${po_file}"
        DEPENDS "${po_file}"
        VERBATIM)
    list(APPEND system_upgrade_po_files "${po_file}")
    list(APPEND system_upgrade_translation_outputs "${mo_file}")
    install(FILES "${mo_file}"
            DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${language}/LC_MESSAGES")
endforeach()

set(system_upgrade_localized_files
    data/applications/system-upgrade.desktop
    data/autostart/system-upgrade.desktop
    data/metainfo/io.github.aratie_os.system_upgrade.metainfo.xml)
foreach(localized_file IN LISTS system_upgrade_localized_files)
    if(localized_file MATCHES "\\.desktop$")
        set(msgfmt_mode --desktop)
    else()
        set(msgfmt_mode --xml)
    endif()
    get_filename_component(output_directory "${CMAKE_CURRENT_BINARY_DIR}/${localized_file}" DIRECTORY)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${localized_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
        COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" ${msgfmt_mode}
                "--template=${CMAKE_CURRENT_SOURCE_DIR}/${localized_file}.in"
                -d "${SYSTEM_UPGRADE_PO_DIR}"
                -o "${CMAKE_CURRENT_BINARY_DIR}/${localized_file}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${localized_file}.in"
                "${SYSTEM_UPGRADE_PO_DIR}/LINGUAS"
                ${system_upgrade_po_files}
        VERBATIM)
    list(APPEND system_upgrade_translation_outputs "${CMAKE_CURRENT_BINARY_DIR}/${localized_file}")
endforeach()
add_custom_target(translations ALL DEPENDS ${system_upgrade_translation_outputs})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(system_upgrade_logic PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(system-upgrade PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS system-upgrade RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/data/applications/system-upgrade.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/data/autostart/system-upgrade.desktop"
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/data/metainfo/io.github.aratie_os.system_upgrade.metainfo.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
install(FILES data/system-upgrade.sudoers
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/sudoers.d"
        RENAME system-upgrade
        PERMISSIONS OWNER_READ GROUP_READ)
install(FILES docs/system-upgrade.1
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

if(BUILD_TESTING)
    find_package(Qt6 6.6 REQUIRED COMPONENTS Test)
    add_executable(updatechecker_tests tests/updatechecker_tests.cpp)
    target_link_libraries(updatechecker_tests PRIVATE
        system_upgrade_logic
        Qt6::Core
        Qt6::Gui
        Qt6::Test
    )
    add_test(NAME updatechecker_tests COMMAND updatechecker_tests)

    # Runs the real alert widget under both languages to prove the catalog
    # is used (pt_BR) and that English is the fallback (en / C).
    add_executable(i18n_tests
        tests/i18n_tests.cpp
        src/update_notification.cpp
        src/update_notification.h)
    target_compile_definitions(i18n_tests PRIVATE
        LAYERSHELLQTINTERFACE_NO_DEPRECATED_WARNINGS
        SYSTEM_UPGRADE_LOCALE_BUILD_DIR="${SYSTEM_UPGRADE_LOCALE_BUILD_DIR}"
        SYSTEM_UPGRADE_DESKTOP_FILE="${CMAKE_CURRENT_BINARY_DIR}/data/applications/system-upgrade.desktop"
        SYSTEM_UPGRADE_METAINFO_FILE="${CMAKE_CURRENT_BINARY_DIR}/data/metainfo/io.github.aratie_os.system_upgrade.metainfo.xml")
    target_link_libraries(i18n_tests PRIVATE
        system_upgrade_logic
        Qt6::Widgets
        Qt6::Svg
        Qt6::Test
        LayerShellQt::Interface
        KF6::I18n)
    add_dependencies(i18n_tests translations)
    add_test(NAME i18n_tests_pt_BR COMMAND i18n_tests)
    set_tests_properties(i18n_tests_pt_BR PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;LANGUAGE=pt_BR;LANG=en_US.UTF-8;LC_ALL=;LC_MESSAGES=;SYSTEM_UPGRADE_EXPECTED_LANGUAGE=pt_BR")
    add_test(NAME i18n_tests_en COMMAND i18n_tests)
    set_tests_properties(i18n_tests_en PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;LANGUAGE=en;LANG=en_US.UTF-8;LC_ALL=;LC_MESSAGES=;SYSTEM_UPGRADE_EXPECTED_LANGUAGE=en")
    add_test(NAME i18n_tests_C COMMAND i18n_tests)
    set_tests_properties(i18n_tests_C PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;LANGUAGE=;LANG=C;LC_ALL=C;LC_MESSAGES=;SYSTEM_UPGRADE_EXPECTED_LANGUAGE=en")
endif()
