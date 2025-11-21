#pragma once
#include <Geode/Utils.hpp>

// the custom shared level format ".level" like ".gmd2", saves audio and almost ALL level data.
// created by because of the limitations of ".gmd" format, made same way as that one
namespace level {

    //why
    namespace json = matjson;
    namespace sdk = geode::prelude;
    namespace utils = sdk::utils;
    namespace str = sdk::string;
    namespace log = sdk::log;
    namespace fs {
        using namespace std::filesystem;
        using namespace sdk::file;
        inline static auto err = std::error_code();
        inline auto readb(path p) { return readBinary(p).unwrapOrDefault(); }
        inline auto writeb(path p, auto data = readb("")) { return writeBinarySafe(p, data); }
    }
    namespace cocos {
        using namespace cocos2d;
        using namespace sdk::cocos;
    }
#define ps(...) str::pathToString(__VA_ARGS__)

    inline auto Err(auto str) {
        log::error("{}", str);
        return sdk::Err("{}", str);
    }

    // Null if not imported, call getID to get path of .level file
    // Example: if (auto inf = isImported(level)) { auto path = inf->getID(); }
    inline auto isImported(sdk::Ref<GJGameLevel> level, std::string newPath = "") {
        //log::debug("{}({}, {})", __FUNCTION__, level.data(), json::Value(newPath).dump());
        //sdk::SceneManager::get()->keepAcrossScenes(level);
        auto tag = sdk::hash("is-imported-from-file");

        if (not newPath.empty()) {
            if (cocos::fileExistsInSearchPaths(newPath.c_str())) {
                auto xd = cocos::CCNode::create();
                xd->setID(newPath);
                xd->setTag(tag);
                if (level) {
                    level->removeChildByTag(tag);
                    level->addChild(xd);
                }
            }
            else log::error("file '{}' does not exist", newPath);
        };

        return !level ? nullptr : level->getChildByTag(tag);
    };

    inline auto jsonFromLevel(sdk::Ref<GJGameLevel> level) {
        if (!level) level = GJGameLevel::create();
        auto json = json::Value::object();
        json.set("levelID", level->m_levelID.value()); //["levelID"] = level->m_levelID.value();
        json.set("levelName", std::string_view(level->m_levelName.c_str())); //["levelName"] = std::string_view(level->m_levelName.c_str());
        json.set("levelDesc", std::string_view(level->m_levelDesc.c_str())); //["levelDesc"] = std::string_view(level->m_levelDesc.c_str());
        //json["levelString"] = level->m_levelString.c_str(); moved to end
        json.set("creatorName", std::string_view(level->m_creatorName.c_str())); //["creatorName"] = std::string_view(level->m_creatorName.c_str());
        json.set("recordString", std::string_view(level->m_recordString.c_str())); //["recordString"] = std::string_view(level->m_recordString.c_str());
        json.set("uploadDate", std::string_view(level->m_uploadDate.c_str())); //["uploadDate"] = std::string_view(level->m_uploadDate.c_str());
        json.set("updateDate", std::string_view(level->m_updateDate.c_str())); //["updateDate"] = std::string_view(level->m_updateDate.c_str());
        json.set("lockedEditorLayers", std::string_view(level->m_lockedEditorLayers.c_str())); //["lockedEditorLayers"] = std::string_view(level->m_lockedEditorLayers.c_str());
        json.set("savedCameraPositions", std::string_view(level->m_savedCameraPositions.c_str())); //["savedCameraPositions"] = std::string_view(level->m_savedCameraPositions.c_str());
        { // cocos::CCPoint m_previewLock
            json::Value pt;
            pt.set("x", level->m_previewLock.x); //["x"] = level->m_previewLock.x;
            pt.set("y", level->m_previewLock.y); //["y"] = level->m_previewLock.y;
            json.set("previewLock", pt); //["previewLock"] = pt;
        }
        json.set("userID", level->m_userID.value()); //["userID"] = level->m_userID.value();
        json.set("accountID", level->m_accountID.value()); //["accountID"] = level->m_accountID.value();
        json.set("difficulty", static_cast<int>(level->m_difficulty)); //["difficulty"] = static_cast<int>(level->m_difficulty);
        json.set("audioTrack", level->m_audioTrack); //["audioTrack"] = level->m_audioTrack;
        json.set("songID", level->m_songID); //["songID"] = level->m_songID;
        json.set("levelRev", level->m_levelRev); //"["levelRev"] = level->m_levelRev;
        json.set("unlisted", level->m_unlisted); //"["unlisted"] = level->m_unlisted;
        json.set("friendsOnly", level->m_friendsOnly); //"["friendsOnly"] = level->m_friendsOnly;
        json.set("objectCount", level->m_objectCount.value()); //"["objectCount"] = level->m_objectCount.value();
        json.set("levelIndex", level->m_levelIndex); //"["levelIndex"] = level->m_levelIndex;
        json.set("ratings", level->m_ratings); //"["ratings"] = level->m_ratings;
        json.set("ratingsSum", level->m_ratingsSum); //"["ratingsSum"] = level->m_ratingsSum;
        json.set("downloads", level->m_downloads); //"["downloads"] = level->m_downloads;
        json.set("isEditable", level->m_isEditable); //"["isEditable"] = level->m_isEditable;
        json.set("gauntletLevel", level->m_gauntletLevel); //"["gauntletLevel"] = level->m_gauntletLevel;
        json.set("gauntletLevel2", level->m_gauntletLevel2); //"["gauntletLevel2"] = level->m_gauntletLevel2;
        json.set("workingTime", level->m_workingTime); //"["workingTime"] = level->m_workingTime;
        json.set("workingTime2", level->m_workingTime2); //"["workingTime2"] = level->m_workingTime2;
        json.set("lowDetailMode", level->m_lowDetailMode); //"["lowDetailMode"] = level->m_lowDetailMode;
        json.set("lowDetailModeToggled", level->m_lowDetailModeToggled); //"["lowDetailModeToggled"] = level->m_lowDetailModeToggled;
        json.set("disableShakeToggled", level->m_disableShakeToggled); //"["disableShakeToggled"] = level->m_disableShakeToggled;
        json.set("selected", level->m_selected); //"["selected"] = level->m_selected;
        json.set("localOrSaved", level->m_localOrSaved); //"["localOrSaved"] = level->m_localOrSaved;
        json.set("disableShake", level->m_disableShake); //"["disableShake"] = level->m_disableShake;
        json.set("isVerified", level->m_isVerified.value()); //"["isVerified"] = level->m_isVerified.value();
        json.set("isVerifiedRaw", level->m_isVerifiedRaw); //"["isVerifiedRaw"] = level->m_isVerifiedRaw;
        json.set("isUploaded", level->m_isUploaded); //"["isUploaded"] = level->m_isUploaded;
        json.set("hasBeenModified", level->m_hasBeenModified); //"["hasBeenModified"] = level->m_hasBeenModified;
        json.set("levelVersion", level->m_levelVersion); //"["levelVersion"] = level->m_levelVersion;
        json.set("gameVersion", level->m_gameVersion); //"["gameVersion"] = level->m_gameVersion;
        json.set("attempts", level->m_attempts.value()); //"["attempts"] = level->m_attempts.value();
        json.set("jumps", level->m_jumps.value()); //"["jumps"] = level->m_jumps.value();
        json.set("clicks", level->m_clicks.value()); //"["clicks"] = level->m_clicks.value();
        json.set("attemptTime", level->m_attemptTime.value()); //"["attemptTime"] = level->m_attemptTime.value();
        json.set("chk", level->m_chk); //"["chk"] = level->m_chk;
        json.set("isChkValid", level->m_isChkValid); //"["isChkValid"] = level->m_isChkValid;
        json.set("isCompletionLegitimate", level->m_isCompletionLegitimate); //"["isCompletionLegitimate"] = level->m_isCompletionLegitimate;
        json.set("normalPercent", level->m_normalPercent.value()); //"["normalPercent"] = level->m_normalPercent.value();
        json.set("orbCompletion", level->m_orbCompletion.value()); //"["orbCompletion"] = level->m_orbCompletion.value();
        json.set("newNormalPercent2", level->m_newNormalPercent2.value()); //"["newNormalPercent2"] = level->m_newNormalPercent2.value();
        json.set("practicePercent", level->m_practicePercent); //"["practicePercent"] = level->m_practicePercent;
        json.set("likes", level->m_likes); //"["likes"] = level->m_likes;
        json.set("dislikes", level->m_dislikes); //"["dislikes"] = level->m_dislikes;
        json.set("levelLength", level->m_levelLength); //"["levelLength"] = level->m_levelLength;
        json.set("featured", level->m_featured); //"["featured"] = level->m_featured;
        json.set("isEpic", level->m_isEpic); //"["isEpic"] = level->m_isEpic;
        json.set("levelFavorited", level->m_levelFavorited); //"["levelFavorited"] = level->m_levelFavorited;
        json.set("levelFolder", level->m_levelFolder); //"["levelFolder"] = level->m_levelFolder;
        json.set("dailyID", level->m_dailyID.value()); //"["dailyID"] = level->m_dailyID.value();
        json.set("demon", level->m_demon.value()); //"["demon"] = level->m_demon.value();
        json.set("demonDifficulty", level->m_demonDifficulty); //"["demonDifficulty"] = level->m_demonDifficulty;
        json.set("stars", level->m_stars.value()); //"["stars"] = level->m_stars.value();
        json.set("autoLevel", level->m_autoLevel); //"["autoLevel"] = level->m_autoLevel;
        json.set("coins", level->m_coins); //"["coins"] = level->m_coins;
        json.set("coinsVerified", level->m_coinsVerified.value()); //"["coinsVerified"] = level->m_coinsVerified.value();
        json.set("password", level->m_password.value()); //"["password"] = level->m_password.value();
        json.set("originalLevel", level->m_originalLevel.value()); //"["originalLevel"] = level->m_originalLevel.value();
        json.set("twoPlayerMode", level->m_twoPlayerMode); //"["twoPlayerMode"] = level->m_twoPlayerMode;
        json.set("failedPasswordAttempts", level->m_failedPasswordAttempts); //"["failedPasswordAttempts"] = level->m_failedPasswordAttempts;
        json.set("firstCoinVerified", level->m_firstCoinVerified.value()); //"["firstCoinVerified"] = level->m_firstCoinVerified.value();
        json.set("secondCoinVerified", level->m_secondCoinVerified.value()); //"["secondCoinVerified"] = level->m_secondCoinVerified.value();
        json.set("thirdCoinVerified", level->m_thirdCoinVerified.value()); //"["thirdCoinVerified"] = level->m_thirdCoinVerified.value();
        json.set("starsRequested", level->m_starsRequested); //"["starsRequested"] = level->m_starsRequested;
        json.set("showedSongWarning", level->m_showedSongWarning); //"["showedSongWarning"] = level->m_showedSongWarning;
        json.set("starRatings", level->m_starRatings); //["starRatings"] = level->m_starRatings;
        json.set("starRatingsSum", level->m_starRatingsSum); //"["starRatingsSum"] = level->m_starRatingsSum;
        json.set("maxStarRatings", level->m_maxStarRatings); //"["maxStarRatings"] = level->m_maxStarRatings;
        json.set("minStarRatings", level->m_minStarRatings); //"["minStarRatings"] = level->m_minStarRatings;
        json.set("demonVotes", level->m_demonVotes); //"["demonVotes"] = level->m_demonVotes;
        json.set("rateStars", level->m_rateStars); //"["rateStars"] = level->m_rateStars;
        json.set("rateFeature", level->m_rateFeature); //"["rateFeature"] = level->m_rateFeature;
        json.set("rateUser", std::string_view(level->m_rateUser.c_str())); //"["rateUser"] = std::string_view(level->m_rateUser.c_str());
        json.set("dontSave", level->m_dontSave); //"["dontSave"] = level->m_dontSave;
        json.set("levelNotDownloaded", level->m_levelNotDownloaded); //"["levelNotDownloaded"] = level->m_levelNotDownloaded;
        json.set("requiredCoins", level->m_requiredCoins); //"["requiredCoins"] = level->m_requiredCoins;
        json.set("isUnlocked", level->m_isUnlocked); //"["isUnlocked"] = level->m_isUnlocked;
        { // cocos::CCPoint m_lastCameraPos
            json::Value pt;
            pt.set("x", level->m_lastCameraPos.x); //"["x"] = level->m_lastCameraPos.x;
            pt.set("y", level->m_lastCameraPos.y); //"["y"] = level->m_lastCameraPos.y;
            json.set("lastCameraPos", pt); //["lastCameraPos"] = pt;
        }
        json.set("lastEditorZoom", level->m_lastEditorZoom); //["lastEditorZoom"] = level->m_lastEditorZoom;
        json.set("lastBuildTab", level->m_lastBuildTab); //["lastBuildTab"] = level->m_lastBuildTab;
        json.set("lastBuildPage", level->m_lastBuildPage); //["lastBuildPage"] = level->m_lastBuildPage;
        json.set("lastBuildGroupID", level->m_lastBuildGroupID); //["lastBuildGroupID"] = level->m_lastBuildGroupID;
        json.set("levelType", static_cast<int>(level->m_levelType)); //["levelType"] = static_cast<int>(level->m_levelType);
        json.set("M_ID", level->m_M_ID); //["M_ID"] = level->m_M_ID;
        json.set("tempName", std::string_view(level->m_tempName.c_str())); //["tempName"] = std::string_view(level->m_tempName.c_str());
        json.set("capacityString", std::string_view(level->m_capacityString.c_str())); //["capacityString"] = std::string_view(level->m_capacityString.c_str());
        json.set("highObjectsEnabled", level->m_highObjectsEnabled); //["highObjectsEnabled"] = level->m_highObjectsEnabled;
        json.set("unlimitedObjectsEnabled", level->m_unlimitedObjectsEnabled); //["unlimitedObjectsEnabled"] = level->m_unlimitedObjectsEnabled;
        json.set("personalBests", std::string_view(level->m_personalBests.c_str())); //["personalBests"] = std::string_view(level->m_personalBests.c_str());
        json.set("timestamp", level->m_timestamp); //["timestamp"] = level->m_timestamp;
        json.set("listPosition", level->m_listPosition); //["listPosition"] = level->m_listPosition;
        json.set("songIDs", std::string_view(level->m_songIDs.c_str())); //["songIDs"] = std::string_view(level->m_songIDs.c_str());
        json.set("sfxIDs", std::string_view(level->m_sfxIDs.c_str())); //["sfxIDs"] = std::string_view(level->m_sfxIDs.c_str());"sfxIDs"] = std::string_view(level->m_sfxIDs.c_str());
        json.set("field_54", level->m_54); //["field_54"] = level->m_54;
        json.set("bestTime", level->m_bestTime); //["bestTime"] = level->m_bestTime;
        json.set("bestPoints", level->m_bestPoints); //["bestPoints"] = level->m_bestPoints;
        json.set("platformerSeed", level->m_platformerSeed); //["platformerSeed"] = level->m_platformerSeed;
        json.set("localBestTimes", std::string_view(level->m_localBestTimes.c_str())); //["localBestTimes"] = std::string_view(level->m_localBestTimes.c_str());
        json.set("localBestPoints", std::string_view(level->m_localBestPoints.c_str())); //["localBestPoints"] = std::string_view(level->m_localBestPoints.c_str());

        json.set("levelString", std::string_view(level->m_levelString.c_str())); //["levelString"] = std::string_view(level->m_levelString.c_str());

        return json;
    }

    inline void updateLevelByJson(json::Value json, sdk::Ref<GJGameLevel> level) {
        if (!level) return log::error("lvl upd by json fail, lvl is {}", level.data());
        // for mle, helps store path in level json instead of level object
        auto file = json.get("file").unwrapOr(json).asString().unwrapOr("");
        if (file.size() > 3) isImported(level, file);
        //log::debug("{} update by json: {}", level, json.dump());
#define asInt(member, ...) level->m_##member = __VA_ARGS__(json.get(#member"").unwrapOr(json).asInt().unwrapOr(static_cast<int>(level->m_##member)));
#define asSeed(member) level->m_##member = json.get(#member"").unwrapOr(json).as<int>().unwrapOr(level->m_##member.value());
#define asString(member) level->m_##member = json.get(#member"").unwrapOr(json).asString().unwrapOr(std::string_view(level->m_##member.c_str()).data()).c_str();
#define asDouble(member) level->m_##member = json.get(#member"").unwrapOr(json).asDouble().unwrapOr(level->m_##member);
#define asBool(member) level->m_##member = json.get(#member"").unwrapOr(json).asBool().unwrapOr(level->m_##member);

        asSeed(levelID);
        asString(levelName);// = json["levelName"].().().c_str();
        asString(levelDesc);// = json["levelDesc"].asString().unwrapOr().c_str();
        asString(levelString);// = json["levelString"].asString().unwrapOr().c_str();
        asString(creatorName);// = json["creatorName"].asString().unwrapOr().c_str();
        asString(recordString);// = json["recordString"].asString().unwrapOr().c_str();
        asString(uploadDate);// = json["uploadDate"].asString().unwrapOr().c_str();
        asString(updateDate);// = json["updateDate"].asString().unwrapOr().c_str();
        asString(lockedEditorLayers);// = json["lockedEditorLayers"].asString().unwrapOr().c_str();
        asString(savedCameraPositions);// = json["savedCameraPositions"].asString().unwrapOr().c_str();
        { // cocos::CCPoint m_previewLock
            json::Value pt = json["previewLock"];
            asDouble(previewLock.x);// = pt["x"].asDouble().unwrapOr();
            asDouble(previewLock.y);// = pt["y"].asDouble().unwrapOr();
        }
        asSeed(userID);// = json["userID"].asInt().unwrapOr();
        asSeed(accountID);// = json["accountID"].asInt().unwrapOr();
        asInt(difficulty, static_cast<GJDifficulty>);// = (json["difficulty"].asInt().unwrapOr());
        asInt(audioTrack);// = json["audioTrack"].asInt().unwrapOr();
        asInt(songID);// = json["songID"].asInt().unwrapOr();
        asInt(levelRev);// = json["levelRev"].asInt().unwrapOr();
        asBool(unlisted);// = json["unlisted"].asBool().unwrapOr();
        asBool(friendsOnly);// = json["friendsOnly"].asBool().unwrapOr();
        asSeed(objectCount);// = json["objectCount"].asInt().unwrapOr();
        asInt(levelIndex);// = json["levelIndex"].asInt().unwrapOr();
        asInt(ratings);// = json["ratings"].asInt().unwrapOr();
        asInt(ratingsSum);// = json["ratingsSum"].asInt().unwrapOr();
        asInt(downloads);// = json["downloads"].asInt().unwrapOr();
        asBool(isEditable);// = json["isEditable"].asBool().unwrapOr();
        asBool(gauntletLevel);// = json["gauntletLevel"].asBool().unwrapOr();
        asBool(gauntletLevel2);// = json["gauntletLevel2"].asBool().unwrapOr();
        asInt(workingTime);// = json["workingTime"].asInt().unwrapOr();
        asInt(workingTime2);// = json["workingTime2"].asInt().unwrapOr();
        asBool(lowDetailMode);// = json["lowDetailMode"].asBool().unwrapOr();
        asBool(lowDetailModeToggled);// = json["lowDetailModeToggled"].asBool().unwrapOr();
        asBool(disableShakeToggled);// = json["disableShakeToggled"].asBool().unwrapOr();
        asBool(selected);// = json["selected"].asBool().unwrapOr();
        asBool(localOrSaved);// = json["localOrSaved"].asBool().unwrapOr();
        asBool(disableShake);// = json["disableShake"].asBool().unwrapOr();
        asSeed(isVerified);// = json["isVerified"].asInt().unwrapOr();
        asBool(isVerifiedRaw);// = json["isVerifiedRaw"].asBool().unwrapOr();
        asBool(isUploaded);// = json["isUploaded"].asBool().unwrapOr();
        asBool(hasBeenModified);// = json["hasBeenModified"].asBool().unwrapOr();
        asInt(levelVersion);// = json["levelVersion"].asInt().unwrapOr();
        asInt(gameVersion);// = json["gameVersion"].asInt().unwrapOr();
        asSeed(attempts);// = json["attempts"].asInt().unwrapOr();
        asSeed(jumps);// = json["jumps"].asInt().unwrapOr();
        asSeed(clicks);// = json["clicks"].asInt().unwrapOr();
        asSeed(attemptTime);// = json["attemptTime"].asInt().unwrapOr();
        asInt(chk);// = json["chk"].asInt().unwrapOr();
        asBool(isChkValid);// = json["isChkValid"].asBool().unwrapOr();
        asBool(isCompletionLegitimate);// = json["isCompletionLegitimate"].asBool().unwrapOr();
        asSeed(normalPercent);// = json["normalPercent"].asInt().unwrapOr();
        asSeed(orbCompletion);// = json["orbCompletion"].asInt().unwrapOr();
        asSeed(newNormalPercent2);// = json["newNormalPercent2"].asInt().unwrapOr();
        asInt(practicePercent);// = json["practicePercent"].asInt().unwrapOr();
        asInt(likes);// = json["likes"].asInt().unwrapOr();
        asInt(dislikes);// = json["dislikes"].asInt().unwrapOr();
        asInt(levelLength);// = json["levelLength"].asInt().unwrapOr();
        asInt(featured);// = json["featured"].asInt().unwrapOr();
        asInt(isEpic);// = json["isEpic"].asInt().unwrapOr();
        asBool(levelFavorited);// = json["levelFavorited"].asBool().unwrapOr();
        asInt(levelFolder);// = json["levelFolder"].asInt().unwrapOr();
        asSeed(dailyID);// = json["dailyID"].asInt().unwrapOr();
        asSeed(demon);// = json["demon"].asInt().unwrapOr();
        asInt(demonDifficulty);// = json["demonDifficulty"].asInt().unwrapOr();
        asSeed(stars);// = json["stars"].asInt().unwrapOr();
        asBool(autoLevel);// = json["autoLevel"].asBool().unwrapOr();
        asInt(coins);// = json["coins"].asInt().unwrapOr();
        asSeed(coinsVerified);// = json["coinsVerified"].asInt().unwrapOr();
        asSeed(password);// = json["password"].asInt().unwrapOr();
        asSeed(originalLevel);// = json["originalLevel"].asInt().unwrapOr();
        asBool(twoPlayerMode);// = json["twoPlayerMode"].asBool().unwrapOr();
        asInt(failedPasswordAttempts);// = json["failedPasswordAttempts"].asInt().unwrapOr();
        asSeed(firstCoinVerified);// = json["firstCoinVerified"].asInt().unwrapOr();
        asSeed(secondCoinVerified);// = json["secondCoinVerified"].asInt().unwrapOr();
        asSeed(thirdCoinVerified);// = json["thirdCoinVerified"].asInt().unwrapOr();
        asInt(starsRequested);// = json["starsRequested"].asInt().unwrapOr();
        asBool(showedSongWarning);// = json["showedSongWarning"].asBool().unwrapOr();
        asInt(starRatings);// = json["starRatings"].asInt().unwrapOr();
        asInt(starRatingsSum);// = json["starRatingsSum"].asInt().unwrapOr();
        asInt(maxStarRatings);// = json["maxStarRatings"].asInt().unwrapOr();
        asInt(minStarRatings);// = json["minStarRatings"].asInt().unwrapOr();
        asInt(demonVotes);// = json["demonVotes"].asInt().unwrapOr();
        asInt(rateStars);// = json["rateStars"].asInt().unwrapOr();
        asInt(rateFeature);// = json["rateFeature"].asInt().unwrapOr();
        asString(rateUser);// = json["rateUser"].asString().unwrapOr().c_str();
        asBool(dontSave);// = json["dontSave"].asBool().unwrapOr();
        asBool(levelNotDownloaded);// = json["levelNotDownloaded"].asBool().unwrapOr();
        asInt(requiredCoins);// = json["requiredCoins"].asInt().unwrapOr();
        asBool(isUnlocked);// = json["isUnlocked"].asBool().unwrapOr();
        { // cocos::CCPoint m_lastCameraPos
            json::Value pt = json["lastCameraPos"];
            asDouble(lastCameraPos.x);// = pt["x"].asDouble().unwrapOr();
            asDouble(lastCameraPos.y);// = pt["y"].asDouble().unwrapOr();
        }
        asDouble(lastEditorZoom);// = json["lastEditorZoom"].asDouble().unwrapOr();
        asInt(lastBuildTab);// = json["lastBuildTab"].asInt().unwrapOr();
        asInt(lastBuildPage);// = json["lastBuildPage"].asInt().unwrapOr();
        asInt(lastBuildGroupID);// = json["lastBuildGroupID"].asInt().unwrapOr();
        asInt(levelType, static_cast<GJLevelType>);// = (json["levelType"].asInt().unwrapOr());
        asInt(M_ID);// = json["M_ID"].asInt().unwrapOr();
        asString(tempName);// = json["tempName"].asString().unwrapOr().c_str();
        asString(capacityString);// = json["capacityString"].asString().unwrapOr().c_str();
        asBool(highObjectsEnabled);// = json["highObjectsEnabled"].asBool().unwrapOr();
        asBool(unlimitedObjectsEnabled);// = json["unlimitedObjectsEnabled"].asBool().unwrapOr();
        asString(personalBests);// = json["personalBests"].asString().unwrapOr().c_str();
        asInt(timestamp);// = json["timestamp"].asInt().unwrapOr();
        asInt(listPosition);// = json["listPosition"].asInt().unwrapOr();
        asString(songIDs);// = json["songIDs"].asString().unwrapOr().c_str();
        asString(sfxIDs);// = json["sfxIDs"].asString().unwrapOr().c_str();
        //asInt(54);// = json["field_54"].asInt().unwrapOr();
        asInt(bestTime);// = json["bestTime"].asInt().unwrapOr();
        asInt(bestPoints);// = json["bestPoints"].asInt().unwrapOr();
        asInt(platformerSeed);// = json["platformerSeed"].asInt().unwrapOr();
        asString(localBestTimes);// = json["localBestTimes"].asString().unwrapOr().c_str();
        asString(localBestPoints);// = json["localBestPoints"].asString().unwrapOr().c_str();

#undef asInt //(member, ...) level->m_##member = __VA_ARGS__(json[#member""].asInt().unwrapOr(static_cast<int>(level->m_##member)));
#undef asSeed //(member) level->m_##member = json[#member""].as<int>().unwrapOr(level->m_##member.value());
#undef asString //(member) level->m_##member = json[#member""].asString().unwrapOr(level->m_##member.c_str()).c_str();
#undef asDouble //(member) level->m_##member = json[#member""].asDouble().unwrapOr(level->m_##member);
#undef asBool //(member) level->m_##member = json[#member""].asBool().unwrapOr(level->m_##member);
    }

    inline sdk::Result<json::Value> exportLevelFile(
        sdk::Ref<GJGameLevel> level,
        fs::path to
    ) {
        try {
            if (!level) return Err(
                "The level ptr is null."
            );
            if (!sdk::typeinfo_cast<GJGameLevel*>(level.data())) return Err(
                "The level ptr is not GJGameLevel typed in RTTI."
            );

            to = sdk::CCFileUtils::get()->fullPathForFilename(ps(to).c_str(), 0).c_str();

            auto ignored_error = std::error_code();
            fs::create_directories(to.parent_path(), ignored_error);
            fs::remove(to, ignored_error);

            auto isJSON = str::endsWith(ps(to), ".json");

#define geode level // call Err from this namespace instead of geode
            GEODE_UNWRAP_INTO(
                auto archive, isJSON ? fs::Zip::create() : fs::Zip::create(to)
                .mapErr([](std::string err) { return fmt::format("Unable to create zip, {}", err); })
            );
#undef geode

            // makin data dump like that can save you from matjson errors related to random memory
            auto lvlJSON = jsonFromLevel(level);
            std::stringstream data;
            data << "{" << std::endl;
            for (auto& [k, v] : std::move(lvlJSON)) {
                data << "\t\"" << k << "\": " << json::parse(v.dump()).unwrapOr(
                    "invalid value (dump failed)"
                ).dump(0) << "," << std::endl;
            }
            data << "\t" R"("there is": "end!")" << std::endl;
            data << "}";

            if (isJSON) fs::writeStringSafe(to, data.str()).isOk();
            else archive.add("_data.json", data.str()).isOk();

            auto workingDir = to.parent_path();
            if (str::endsWith(ps(workingDir), "levels")) workingDir = workingDir.parent_path();
            if (isJSON) {
				fs::create_directories(workingDir / "songs", ignored_error);
                fs::create_directories(workingDir / "sfx", ignored_error);
            };

            //primary song id isnt 0
            if (level->m_songID) {
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSong(
                    level->m_songID
                ).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if exists
                if (cocos::fileExistsInSearchPaths(ps(path).c_str())) {
                    if (auto err = archive.add(fs::path(path).filename(), fs::readb(path)).err()) Err(
                        err.value_or("unknown error")
                    );
                }
                //copy song for json export
                if (isJSON) fs::copy_file(
                    path, workingDir / "songs" / ps(fs::path(path).filename())
                    , fs::copy_options::update_existing, fs::err
                );
            }

            //fe the ids from list
            for (auto id : str::split(level->m_songIDs, ",")) {
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSong(
                    utils::numFromString<int>(id).unwrapOrDefault()
                ).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if exists
                if (cocos::fileExistsInSearchPaths(ps(path).c_str())) {
                    if (auto err = archive.add(fs::path(path).filename(), fs::readb(path)).err()) Err(
                        err.value_or("unknown error")
                    );
                };
                //copy song for json export
                if (isJSON) fs::copy_file(
                    path, workingDir / "songs" / ps(fs::path(path).filename())
                    , fs::copy_options::update_existing, fs::err
                );
            }

            //fe the ids from list
            for (auto id : str::split(level->m_sfxIDs, ",")) {
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSFX(
                    utils::numFromString<int>(id).unwrapOrDefault()
                ).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if exists
                if (cocos::fileExistsInSearchPaths(ps(path).c_str())) {
                    if (auto err = archive.add(fs::path(path).filename(), fs::readb(path)).err()) Err(
                        err.value_or("unknown error")
                    );
                }
                //copy sfx for json export
                if (isJSON) fs::copy_file(
                    path, workingDir / "sfx" / ps(fs::path(path).filename())
                    , fs::copy_options::update_existing, fs::err
                );
            }

            return sdk::Ok(std::move(lvlJSON));
        }
        catch (std::exception& e) { // feels like nails plug in my fingers
            return Err("Exception reached, " + std::string(e.what()));
        }
    };

    inline sdk::Result<GJGameLevel*> importLevelFile(
        fs::path from,
        sdk::Ref<GJGameLevel> level = GJGameLevel::create()
    ) {
        try {
            if (!level) return Err(
                "Level ptr is null."
            );
            if (!sdk::typeinfo_cast<GJGameLevel*>(level.data())) return Err(
                "Level ptr is not GJGameLevel typed in RTTI."
            );

            from = sdk::CCFileUtils::get()->fullPathForFilename(ps(from).c_str(), 0).c_str();

            isImported(level, ps(from));

            auto importanterrc = std::error_code();
            auto sizech = fs::file_size(from, importanterrc);
            if (importanterrc or !sizech) return Err((std::stringstream() <<
                "Failed to check file size (" << sizech << "), " <<
                std::move(importanterrc).message()
                ).str());

            auto archiveData = fs::readb(from);
            if (archiveData.empty()) return Err(
                "Failed to read file (size is 0)"
            );


#define geode level // call Err from this namespace instead of geode
            GEODE_UNWRAP_INTO(
                auto archive, fs::Unzip::create(archiveData)
                .mapErr([](std::string err) { return fmt::format("Unable to read zip, {}", err); })
            );
#undef geode

            auto dump = archive.extract("_data.json").unwrapOrDefault();
            auto data = std::move(
                json::parse(std::string(dump.begin(), dump.end())).unwrapOrDefault()
            );

            updateLevelByJson(data, level);

            //primary song id isnt 0
            if (level->m_songID) {
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSong(level->m_songID).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if not exists
                if (!cocos::CCFileUtils::get()->isFileExist(ps(path).c_str())) {
                    auto atzip = ps(fs::path(path).filename());
                    fs::create_directories(path.parent_path(), fs::err);
                    if (auto err = fs::writeb(path, archive.extract(atzip).unwrapOrDefault()).err()) Err(
                        err.value_or("unknown error")
                    );
                };
            }

            for (auto id : str::split(level->m_songIDs, ",")) {
                //skip primary
                if (level->m_songID == utils::numFromString<int>(id).unwrapOr(0)) continue;
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSong(
                    utils::numFromString<int>(id).unwrapOrDefault()
                ).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if not exists
                if (!cocos::CCFileUtils::get()->isFileExist(ps(path).c_str())) {
                    auto atzip = ps(fs::path(path).filename());
                    fs::create_directories(path.parent_path(), fs::err);
                    if (auto err = fs::writeb(path, archive.extract(atzip).unwrapOrDefault()).err()) Err(
                        err.value_or("unknown error")
                    );
                }
            }

            for (auto id : str::split(level->m_sfxIDs, ",")) {
                //path
                fs::path path = MusicDownloadManager::sharedState()->pathForSFX(
                    utils::numFromString<int>(id).unwrapOrDefault()
                ).c_str();
                path = cocos::CCFileUtils::get()->fullPathForFilename(ps(path).c_str(), 0).c_str();
                //add if not exists
                if (!cocos::CCFileUtils::get()->isFileExist(ps(path).c_str())) {
                    auto atzip = ps(fs::path(path).filename());
                    fs::create_directories(path.parent_path(), fs::err);
                    if (auto err = fs::writeb(path, archive.extract(atzip).unwrapOrDefault()).err()) Err(
                        err.value_or("unknown error")
                    );
                }
            }

            return sdk::Ok(level.data());
        }
        catch (std::exception& e) { // FEELS LIKE NAILS PLUG IN MY FINGERS
            return Err("Exception reached, " + std::string(e.what()));
        }
    };
}