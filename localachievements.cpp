#include "localachievements.h"
#include "achievementmodel.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QSet>

QList<AchievementInfo> LocalAchievements::parseFile(const QString& filePath, QString* errorMessage)
{
    QList<AchievementInfo> result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errorMessage)
            *errorMessage = QString("Could not open local achievement file: %1").arg(filePath);
        return result;
    }

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif

    QSet<unsigned int> seenIds;
    int skipped = 0;

    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;

        AchievementInfo info;
        if (!parseLine(line, info))
        {
            ++skipped;
            continue;
        }

        if (seenIds.contains(info.id))
        {
            // Duplicate achievement ID in the file - keep the first
            // definition and ignore the rest rather than fail the whole load.
            ++skipped;
            continue;
        }

        seenIds.insert(info.id);
        result.append(info);
    }

    file.close();

    if (result.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QString("No valid achievements found in %1").arg(filePath);
    }
    else if (skipped > 0 && errorMessage)
    {
        *errorMessage = QString("Loaded %1 local achievement(s); skipped %2 line(s) that could not be parsed")
                            .arg(result.size())
                            .arg(skipped);
    }

    return result;
}

bool LocalAchievements::parseLine(const QString& rawLine, AchievementInfo& out)
{
    QString line = rawLine.trimmed();
    if (line.isEmpty() || line.startsWith('#'))
        return false;

    // Field 0: numeric achievement ID. By RetroAchievements convention,
    // locally-authored/unofficial achievements use IDs starting at
    // 111000001 so they never collide with real, server-assigned IDs -
    // but we don't require that here, we just flag everything from this
    // file as isLocal so it's unambiguous regardless of numbering.
    int firstColon = line.indexOf(':');
    if (firstColon <= 0)
        return false;

    bool idOk = false;
    unsigned int id = line.left(firstColon).trimmed().toUInt(&idOk);
    if (!idOk || id == 0)
        return false;

    QString rest = line.mid(firstColon + 1);
    QString memAddr;

    // Field 1: the trigger logic string. It is wrapped in double quotes
    // because it can itself contain colons (e.g. ResetIf clauses like
    // "R:0x 000242!=10096_..."), which would otherwise break a naive split.
    if (rest.startsWith('"'))
    {
        int closeQuote = rest.indexOf('"', 1);
        if (closeQuote < 0)
            return false;
        memAddr = rest.mid(1, closeQuote - 1);
        rest = rest.mid(closeQuote + 1);
        if (rest.startsWith(':'))
            rest = rest.mid(1);
    }
    else
    {
        int nextColon = rest.indexOf(':');
        if (nextColon < 0)
            return false;
        memAddr = rest.left(nextColon);
        rest = rest.mid(nextColon + 1);
    }

    if (memAddr.isEmpty())
        return false;

    // Remaining fields: Title:Description:Progress:ProgressMax:ProgressFormat:
    // Author:Points:Created:Modified:Upvotes:Downvotes:BadgeName
    QStringList fields = rest.split(':');
    auto fieldAt = [&fields](int index) -> QString {
        return (index >= 0 && index < fields.size()) ? fields.at(index) : QString();
    };

    QString title = fieldAt(0).trimmed();
    if (title.isEmpty())
        return false;

    QString badgeName = fieldAt(11).trimmed();

    out = AchievementInfo();
    out.id = id;
    out.mem_addr = memAddr;
    out.title = title;
    out.description = fieldAt(1);
    out.points = fieldAt(6).toUInt();
    out.badge_name = badgeName;
    out.badge_url = badgeUrl(badgeName, false);
    out.badge_locked_url = badgeUrl(badgeName, true);
    out.flags = 3; // treated the same as an official "Core" achievement for gameplay/trigger purposes
    out.type = "";
    out.unlocked = false;
    out.isLocal = true;
    out.time_unlocked_string = "";
    out.time_unlocked = QDateTime(QDate(1990, 11, 21), QTime(0, 0, 0));
    out.achievement_link = QUrl();
    out.primed = false;
    out.value = 0;
    out.target = 0;
    out.percent = 0;

    return true;
}

void LocalAchievements::applyUnlocks(AchievementModel* model, const QString& unlockFilePath)
{
    if (!model)
        return;

    QFile file(unlockFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return; // No saved unlocks for this set yet - nothing to do.

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif

    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList parts = line.split('|');
        if (parts.size() < 2)
            continue;

        bool ok = false;
        unsigned int id = parts.at(0).toUInt(&ok);
        if (!ok)
            continue;

        QDateTime time = QDateTime::fromString(parts.at(1), Qt::ISODate);
        if (!time.isValid())
            time = QDateTime::currentDateTime();

        // unlockAchievement() looks the row up by achievement ID, updates
        // its unlocked/value/percent state, and emits the same
        // dataChanged/unlockedChanged signals a live unlock would - so
        // restored unlocks show up in the UI exactly like a fresh one.
        model->unlockAchievement(id, time);
    }

    file.close();
}

bool LocalAchievements::saveUnlock(const QString& unlockFilePath, unsigned int id, const QDateTime& time)
{
    QFileInfo info(unlockFilePath);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath("."))
        return false;

    QFile file(unlockFilePath);
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream << id << '|' << time.toString(Qt::ISODate) << '\n';
    file.close();
    return true;
}

QUrl LocalAchievements::badgeUrl(const QString& badgeName, bool locked)
{
    if (badgeName.isEmpty())
        return QUrl();

    QString url = "https://media.retroachievements.org/Badge/" + badgeName;
    url += (locked ? "_lock.png" : ".png");
    return QUrl(url);
}
