// Proves that the real alert widget is translated through KI18n:
// LANGUAGE=pt_BR shows the original Portuguese interface, LANGUAGE=en or the
// C locale shows the English source strings. Run through CTest (see
// CMakeLists.txt), which sets SYSTEM_UPGRADE_EXPECTED_LANGUAGE.

#include "catalog_layout_direction.h"
#include "update_notification.h"

#include <KLocalizedString>

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTest>

#include <clocale>

class I18nTests final : public QObject
{
    Q_OBJECT

private slots:
    void alertUsesExpectedLanguage();
    void desktopFileCarriesTranslations();
    void metainfoCarriesTranslations();
    void rightToLeftCatalogMirrorsLayout();

private:
    static bool expectPortuguese()
    {
        return qEnvironmentVariable("SYSTEM_UPGRADE_EXPECTED_LANGUAGE") == QLatin1String("pt_BR");
    }
    static QString readFile(const char *path)
    {
        QFile file(QString::fromUtf8(path));
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }
};

void I18nTests::alertUsesExpectedLanguage()
{
    const bool pt = expectPortuguese();
    // gettext (and therefore KI18n) ignores LANGUAGE while LC_MESSAGES is the
    // C locale. CTest requests en_US.UTF-8; a build chroot without that locale
    // cannot show translations at all, so the Portuguese check is skipped there.
    const QByteArray messagesLocale = std::setlocale(LC_MESSAGES, nullptr);
    if (pt && (messagesLocale == "C" || messagesLocale == "POSIX" || messagesLocale.startsWith("C."))) {
        QSKIP(qPrintable(QStringLiteral("LC_MESSAGES is %1 (locale missing?); install en_US.UTF-8 to test pt_BR")
                             .arg(QString::fromLatin1(messagesLocale))));
    }

    UpdateNotification notification(false);

    QCOMPARE(notification.windowTitle(),
             pt ? QStringLiteral("Atualizações disponíveis") : QStringLiteral("Updates Available"));

    QStringList labels;
    for (const QLabel *label : notification.findChildren<QLabel *>()) {
        labels << label->text();
    }
    QVERIFY2(labels.contains(pt ? QStringLiteral("Existem atualizações disponíveis para o sistema.")
                                : QStringLiteral("Updates are available for the system.")),
             qPrintable(labels.join(QLatin1String(" | "))));

    QStringList buttons;
    for (const QPushButton *button : notification.findChildren<QPushButton *>()) {
        buttons << button->text();
    }
    const QStringList expectedButtons = pt
        ? QStringList{QStringLiteral("Atualizar"), QStringLiteral("Cancelar")}
        : QStringList{QStringLiteral("Update"), QStringLiteral("Cancel")};
    QCOMPARE(buttons, expectedButtons);

    QStringList accessibleNames;
    for (const QWidget *widget : notification.findChildren<QWidget *>()) {
        if (!widget->accessibleName().isEmpty()) {
            accessibleNames << widget->accessibleName();
        }
    }
    QVERIFY(accessibleNames.contains(pt ? QStringLiteral("Alerta de atualização")
                                        : QStringLiteral("Update alert")));
}

void I18nTests::desktopFileCarriesTranslations()
{
    const QString desktop = readFile(SYSTEM_UPGRADE_DESKTOP_FILE);
    QVERIFY(desktop.contains(QStringLiteral("\nName=System Updates\n")));
    QVERIFY(desktop.contains(QStringLiteral("\nName[pt_BR]=Atualizações do sistema\n")));
    QVERIFY(desktop.contains(QStringLiteral("\nKeywords[pt_BR]=atualização;pacotes;programas;sistema;\n")));
}

void I18nTests::metainfoCarriesTranslations()
{
    const QString metainfo = readFile(SYSTEM_UPGRADE_METAINFO_FILE);
    QVERIFY(metainfo.contains(QStringLiteral("<id>io.github.aratie_os.system_upgrade</id>")));
    QVERIFY(metainfo.contains(QStringLiteral(">Atualizações do sistema</name>")));
    QVERIFY(metainfo.contains(QStringLiteral(">Verifique atualizações do sistema e do Flatpak</summary>")));
}

void I18nTests::rightToLeftCatalogMirrorsLayout()
{
    auto *application = qApp;
    // No qtbase_ar translator in this process (LANGUAGE is pt_BR, en or unset):
    // Qt by itself keeps the layout left to right.
    QVERIFY(QCoreApplication::translate("QGuiApplication", "QT_LAYOUT_DIRECTION") != QStringLiteral("RTL"));
    QCOMPARE(QGuiApplication::layoutDirection(), Qt::LeftToRight);

    // The application's own Arabic catalog is used: mirror (same call as main()).
    KLocalizedString::setLanguages({QStringLiteral("ar"), QStringLiteral("en")});
    QVERIFY(KLocalizedString::isApplicationTranslatedInto(QStringLiteral("ar")));
    QCOMPARE(CatalogLayoutDirection::effectiveLanguage(), QStringLiteral("ar"));
    CatalogLayoutDirection::apply(*application);
    QCOMPARE(QGuiApplication::layoutDirection(), Qt::RightToLeft);
    UpdateNotification notification(false);
    QVERIFY(notification.isRightToLeft());

    // Yiddish is written right to left, but there is no system-upgrade catalog
    // for it: the interface falls back to English and stays left to right.
    application->setLayoutDirection(Qt::LeftToRight);
    KLocalizedString::setLanguages({QStringLiteral("yi"), QStringLiteral("en")});
    QCOMPARE(QLocale(QStringLiteral("yi")).textDirection(), Qt::RightToLeft);
    QVERIFY(!KLocalizedString::isApplicationTranslatedInto(QStringLiteral("yi")));
    QCOMPARE(CatalogLayoutDirection::effectiveLanguage(), QString());
    CatalogLayoutDirection::apply(*application);
    QCOMPARE(QGuiApplication::layoutDirection(), Qt::LeftToRight);

    KLocalizedString::clearLanguages();
    application->setLayoutDirection(Qt::LayoutDirectionAuto);
}

int main(int argc, char *argv[])
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("system-upgrade"));
    KLocalizedString::addDomainLocaleDir(QByteArrayLiteral("system-upgrade"),
                                         QStringLiteral(SYSTEM_UPGRADE_LOCALE_BUILD_DIR));
    QApplication application(argc, argv);
    I18nTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "i18n_tests.moc"
