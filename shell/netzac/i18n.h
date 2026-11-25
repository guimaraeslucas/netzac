#pragma once

#include <clocale>
#include <cstdlib>

#include <QCoreApplication>
#include <QString>

#if defined(Q_OS_WIN)
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QStringList>
#include <QTranslator>
#else
#include <filesystem>
#include <libintl.h>
#include <string>
#endif

#ifndef ZACOS9_TEXTDOMAIN
#define ZACOS9_TEXTDOMAIN "zacos9-netzac"
#endif

#ifndef ZACOS9_LOCALEDIR_DEFAULT
#define ZACOS9_LOCALEDIR_DEFAULT "/usr/share/locale"
#endif

inline void initI18n()
{
    std::setlocale(LC_ALL, "");
#if !defined(Q_OS_WIN)
    const char *envDir = std::getenv("ZACOS9_LOCALEDIR");
    std::string chosenDir;
    if (envDir && *envDir) {
        chosenDir = envDir;
    } else {
        const char *xdgData = std::getenv("XDG_DATA_HOME");
        const char *home = std::getenv("HOME");
        std::string userLocale;
        if (xdgData && *xdgData)
            userLocale = std::string(xdgData) + "/locale";
        else if (home && *home)
            userLocale = std::string(home) + "/.local/share/locale";

        std::error_code ec;
        if (!userLocale.empty() && std::filesystem::exists(userLocale, ec))
            chosenDir = userLocale;
        else
            chosenDir = ZACOS9_LOCALEDIR_DEFAULT;
    }
    bindtextdomain(ZACOS9_TEXTDOMAIN, chosenDir.c_str());
    bind_textdomain_codeset(ZACOS9_TEXTDOMAIN, "UTF-8");
#endif
}

#if defined(Q_OS_WIN)
// Must run after the QCoreApplication instance exists.
inline void installI18nTranslations(QCoreApplication &app)
{
    QStringList tags;
    const QLocale system = QLocale::system();
    QStringList candidates = system.uiLanguages();
    candidates << system.name();
    candidates << QLocale(system.language(), QLocale::AnyCountry).name();
    for (QString tag : candidates) {
        tag.replace(QLatin1Char('-'), QLatin1Char('_'));
        if (!tag.isEmpty() && !tags.contains(tag))
            tags.append(tag);
    }

    const QString base = QCoreApplication::applicationDirPath();
    const QStringList dirs = {
        base + QStringLiteral("/translations"),
        base,
    };

    for (const QString &tag : tags) {
        for (const QString &dir : dirs) {
            const QString path = dir + QLatin1Char('/')
                + QStringLiteral(ZACOS9_TEXTDOMAIN "_") + tag + QStringLiteral(".qm");
            if (!QFileInfo::exists(path))
                continue;
            auto *translator = new QTranslator(&app);
            if (translator->load(path)) {
                app.installTranslator(translator);
                return;
            }
            delete translator;
        }
    }
}
#else
inline void installI18nTranslations(QCoreApplication &) {}
#endif

inline QString ztr(const char *msgid)
{
    if (!msgid || !*msgid)
        return QString();
#if defined(Q_OS_WIN)
    return QCoreApplication::translate("", msgid);
#else
    return QString::fromUtf8(dcgettext(ZACOS9_TEXTDOMAIN, msgid, LC_MESSAGES));
#endif
}

inline QString ztrn(const char *singular, const char *plural, unsigned long n)
{
    if (!singular || !plural)
        return QString();
#if defined(Q_OS_WIN)
    return QCoreApplication::translate("", singular, plural, static_cast<int>(n));
#else
    return QString::fromUtf8(dcngettext(ZACOS9_TEXTDOMAIN, singular, plural, n, LC_MESSAGES));
#endif
}
