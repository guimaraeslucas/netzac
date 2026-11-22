#pragma once

#include <clocale>
#include <cstdlib>
#include <libintl.h>

#include <QString>

#ifndef ZACOS9_TEXTDOMAIN
#define ZACOS9_TEXTDOMAIN "zacos9-netzac"
#endif

#ifndef ZACOS9_LOCALEDIR_DEFAULT
#define ZACOS9_LOCALEDIR_DEFAULT "/usr/share/locale"
#endif

inline void initI18n()
{
    std::setlocale(LC_ALL, "");
    const char *envDir = std::getenv("ZACOS9_LOCALEDIR");
    const char *localeDir = (envDir && *envDir) ? envDir : ZACOS9_LOCALEDIR_DEFAULT;
    bindtextdomain(ZACOS9_TEXTDOMAIN, localeDir);
    bind_textdomain_codeset(ZACOS9_TEXTDOMAIN, "UTF-8");
}

inline QString ztr(const char *msgid)
{
    if (!msgid || !*msgid)
        return QString();
    return QString::fromUtf8(dcgettext(ZACOS9_TEXTDOMAIN, msgid, LC_MESSAGES));
}

inline QString ztrn(const char *singular, const char *plural, unsigned long n)
{
    if (!singular || !plural)
        return QString();
    return QString::fromUtf8(dcngettext(ZACOS9_TEXTDOMAIN, singular, plural, n, LC_MESSAGES));
}
