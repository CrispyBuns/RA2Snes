#ifndef LOCALACHIEVEMENTS_H
#define LOCALACHIEVEMENTS_H

#include <QList>
#include <QString>
#include <QDateTime>
#include "rastructs.h"

class AchievementModel;

// LocalAchievements loads and persists "local"/unofficial achievement sets -
// the kind shared on the RetroAchievements forums for hacks and homebrew
// that don't have an official set on the server (e.g. a game's own forum
// topic, in the classic colon-delimited achievement definition format:
//
//   ID:"MemAddr":Title:Description:Progress:ProgressMax:ProgressFormat:Author:Points:Created:Modified:Upvotes:Downvotes:BadgeName
//
// One achievement per line. Lines starting with '#' or blank lines are
// ignored. MemAddr is quoted because the trigger logic itself may contain
// colons (e.g. "R:0x 000242!=10096_...").
//
// These sets are identified locally by the ROM's MD5 hash, exactly the way
// the online "patch" flow is (see RAClient::loadGame), so they slot into
// the existing MemoryReader / AchievementModel pipeline unchanged.
class LocalAchievements
{
public:
    // Parses a local achievement definition file. Returns the parsed
    // achievements (all flagged isLocal = true, unlocked = false). On
    // failure returns an empty list and, if errorMessage is non-null, fills
    // it with a human readable explanation.
    static QList<AchievementInfo> parseFile(const QString& filePath, QString* errorMessage = nullptr);

    // Parses a single achievement definition line. Returns false (and does
    // not modify 'out') if the line could not be parsed.
    static bool parseLine(const QString& line, AchievementInfo& out);

    // Reads a previously-saved local unlock file (id + ISO timestamp per
    // line, one per unlock) and marks the matching entries in 'model' as
    // unlocked. Safe to call with a non-existent path (no-op).
    static void applyUnlocks(AchievementModel* model, const QString& unlockFilePath);

    // Appends a single unlock record to the local unlock file, creating the
    // file/directory if necessary. Local unlocks are never sent to the
    // RetroAchievements server, so this is the only persistence they get.
    static bool saveUnlock(const QString& unlockFilePath, unsigned int id, const QDateTime& time);

    // Badge art for local sets is still served from RetroAchievements' CDN
    // (authors reference existing badge IDs from the site's badge library),
    // so we can build the same badge URLs the server's "patch" response
    // would have given us.
    static QUrl badgeUrl(const QString& badgeName, bool locked);
};

#endif // LOCALACHIEVEMENTS_H
