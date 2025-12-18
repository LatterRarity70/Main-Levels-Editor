#include <_Main.hpp>

/*

level getting update hook

*/

#include <Geode/modify/GameLevelManager.hpp>
class $modify(MLE_GameLevelManager, GameLevelManager) {

    $override GJGameLevel* getMainLevel(int levelID, bool dontGetLevelString) {
        Ref level = GameLevelManager::getMainLevel(levelID, dontGetLevelString);

        if (MLE_LevelsInJSON::get()->contains(levelID)) {
            /*log::debug(
                "MLE_LocalLevelManager::m_mainLevelsInJSON[{}]->{}",
                levelID, MLE_LocalLevelManager::m_mainLevelsInJSON[levelID].dump()
            );*/
            auto loadedLevel = GJGameLevel::create();
            level::updateLevelByJson(MLE_LevelsInJSON::get()->at(levelID), loadedLevel);
            if (auto aw = level::isImported(loadedLevel)) level::isImported(level, aw->getID());
            //xd
            level->m_levelString = loadedLevel->m_levelString.c_str();
            level->m_stars = (loadedLevel->m_stars.value());
            level->m_requiredCoins = loadedLevel->m_requiredCoins;
            level->m_levelName = loadedLevel->m_levelName;
            level->m_audioTrack = loadedLevel->m_audioTrack;
            level->m_songID = loadedLevel->m_songID;
            level->m_songIDs = loadedLevel->m_songIDs;
            level->m_sfxIDs = loadedLevel->m_sfxIDs;
            level->m_demon = (loadedLevel->m_demon.value());
            level->m_twoPlayerMode = loadedLevel->m_twoPlayerMode;
            level->m_difficulty = loadedLevel->m_difficulty;
            level->m_capacityString = loadedLevel->m_capacityString;
            level->m_levelID = (levelID);
            level->m_timestamp = loadedLevel->m_timestamp;
            level->m_levelLength = loadedLevel->m_levelLength;
        };

        level->m_levelID = levelID; // -1, -2 for listing exists. no default id pls
        level->m_songID = !level->m_audioTrack ? level->m_songID : 0; // what the fuck why its ever was saved
        level->m_levelType = GJLevelType::Main;
        level->m_levelString = dontGetLevelString ? "" : level->m_levelString.c_str();

        return level;
    };

};

/*

LEVEL INTEGRITY VERIFY BYPASS
(and same but not necessary level getting update hook)

*/

#include <Geode/modify/MusicDownloadManager.hpp>
class $modify(MLE_MusicDownloadManager, MusicDownloadManager) {
    gd::string pathForSFX(int id) {
        //log::debug("{}:{}({})", __func__, __LINE__, id);
        std::filesystem::path ref = MusicDownloadManager::pathForSFX(id);
        if (auto as = fmt::format("sfx/{}{}", id, ref.extension()); existsInPaths(as.c_str())) {
            return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0);
        }
        if (auto as = fmt::format("sfx.{}{}", id, ref.extension()); existsInPaths(as.c_str())) {
            return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0);
        }
        return MusicDownloadManager::pathForSFX(id);
    }; 
    
    gd::string pathForSong(int id) {
        //log::debug("{}:{}({})", __func__, __LINE__, id);
        if (auto sc = CCScene::get())
            if (sc->getChildByType<LoadingLayer>(0))
                return MusicDownloadManager::pathForSong(id);
        std::filesystem::path ref = MusicDownloadManager::pathForSong(id);
        if (auto as = fmt::format("songs/{}{}", id, ref.extension()); existsInPaths(as.c_str())) {
            return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0);
        }
        if (auto as = fmt::format("song.{}{}", id, ref.extension()); existsInPaths(as.c_str())) {
            return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0);
        }
        return MusicDownloadManager::pathForSong(id);
    }
};

#include <Geode/modify/LevelTools.hpp>
class $modify(MLE_LevelTools, LevelTools) {

    static Result<matjson::Value> audio(int p0) {
        if (MLE_LevelsInJSON::get()->contains("audio"_h)) {
            auto audio = MLE_LevelsInJSON::get()->at("audio"_h);
            auto id = fmt::format("{}", p0);
            if (audio.contains(id)) return Ok(audio[id]);
        };
        return Err("audio not found");
    };

    $override static gd::string getAudioFileName(int p0) {
        //log::debug("{}:{}({})", __func__, __LINE__, p0);

        if (auto as = fmt::format("audio.{}.mp3", p0); existsInPaths(as.c_str())) return as.c_str();
        if (auto as = fmt::format("audio/{}.mp3", p0); existsInPaths(as.c_str())) return as.c_str();

        if (auto a = audio(p0); a.isOk())
            if (a.unwrap().contains("file"))
                return a.unwrapOr("").get("file").unwrap().asString().unwrapOr("").c_str();

        return LevelTools::getAudioFileName(p0).c_str();
    };
    $override static gd::string getAudioTitle(int p0) {
        //log::debug("{}:{}({})", __func__, __LINE__, p0);
        if (auto a = audio(p0); a.isOk())
            if (a.unwrap().contains("title"))
                return a.unwrapOr("").get("title").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::getAudioTitle(p0).c_str();
    };
    $override static gd::string urlForAudio(int p0) {
        //log::debug("{}:{}({})", __func__, __LINE__, p0);
        if (auto a = audio(p0); a.isOk())
            if (a.unwrap().contains("url"))
                return a.unwrapOr("").get("url").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::urlForAudio(p0);
    };

    $override static int artistForAudio(int p0) {
        //log::debug("{}:{}({})", __func__, __LINE__, p0);
        if (auto a = audio(p0); a.isOk())
            if (a.unwrap().contains("artist"))
                return a.unwrap().get("artist").unwrap().asInt().unwrapOr(0);
        return LevelTools::artistForAudio(p0);
    };

    static Result<matjson::Value> artists(int p0) {
        if (MLE_LevelsInJSON::get()->contains("artists"_h)) {
            auto artists = MLE_LevelsInJSON::get()->at("artists"_h);
            auto id = fmt::format("{}", p0);
            if (artists.contains(id)) return Ok(artists[id]);
        };
        return Err("artists not found");
    };

    $override static gd::string ytURLForArtist(int p0) {
        if (auto a = artists(p0); a.isOk())
            if (a.unwrap().contains("yt"))
                return a.unwrap().get("yt").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::ytURLForArtist(p0).c_str();
    };
    $override static gd::string ngURLForArtist(int p0) {
        if (auto a = artists(p0); a.isOk())
            if (a.unwrap().contains("ng"))
                return a.unwrap().get("ng").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::ngURLForArtist(p0).c_str();
    };
    $override static gd::string fbURLForArtist(int p0) {
        if (auto a = artists(p0); a.isOk())
            if (a.unwrap().contains("fb"))
                return a.unwrap().get("fb").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::fbURLForArtist(p0).c_str();
    };
    $override static gd::string nameForArtist(int p0) {
        if (auto a = artists(p0); a.isOk())
            if (a.unwrap().contains("name"))
                return a.unwrap().get("name").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::nameForArtist(p0).c_str();
    };

    //paranoic hook
    $override static GJGameLevel* getLevel(int levelID, bool dontGetLevelString) {
        Ref level = LevelTools::getLevel(levelID, dontGetLevelString);

        if (MLE_LevelsInJSON::get()->contains(levelID)) {
            /*log::debug(
                "MLE_LocalLevelManager::m_mainLevelsInJSON[{}]->{}",
                levelID, MLE_LocalLevelManager::m_mainLevelsInJSON[levelID].dump()
            );*/
            auto loadedLevel = GJGameLevel::create();
            level::updateLevelByJson(MLE_LevelsInJSON::get()->at(levelID), loadedLevel);
            //xd
            level->m_levelString = loadedLevel->m_levelString.c_str();
            level->m_stars = (loadedLevel->m_stars.value());
            level->m_requiredCoins = loadedLevel->m_requiredCoins;
            level->m_levelName = loadedLevel->m_levelName;
            level->m_audioTrack = loadedLevel->m_audioTrack;
            level->m_songID = loadedLevel->m_songID;
            level->m_songIDs = loadedLevel->m_songIDs;
            level->m_sfxIDs = loadedLevel->m_sfxIDs;
            level->m_demon = (loadedLevel->m_demon.value());
            level->m_twoPlayerMode = loadedLevel->m_twoPlayerMode;
            level->m_difficulty = loadedLevel->m_difficulty;
            level->m_capacityString = loadedLevel->m_capacityString;
            level->m_levelID = (levelID);
            level->m_timestamp = loadedLevel->m_timestamp;
            level->m_levelLength = loadedLevel->m_levelLength;
        };

        level->m_levelID = levelID; // -1, -2 for listing exists. no default id pls
        level->m_songID = !level->m_audioTrack ? level->m_songID : 0; // what the fuck why its ever was saved
        level->m_levelType = GJLevelType::Main;
        level->m_levelString = dontGetLevelString ? "" : level->m_levelString.c_str();

        return level;
    };

    $override static bool verifyLevelIntegrity(gd::string p0, int p1) {
        return VERIFY_LEVEL_INTEGRITY ? LevelTools::verifyLevelIntegrity(p0, p1) : true;
    }

};