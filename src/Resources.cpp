#include <_Main.hpp>

/*

Just additional search paths

*/

void ModLoaded() {
    auto ok = CCTexturePack();
    ok.m_paths.push_back(string::pathToString(getMod()->getSaveDir()).c_str());
    ok.m_paths.push_back(string::pathToString(getMod()->getConfigDir()).c_str());
    ok.m_paths.push_back(string::pathToString(getMod()->getResourcesDir()).c_str());
    ok.m_paths.push_back(string::pathToString(getMod()->getPersistentDir()).c_str());
    CCFileUtils::get()->addTexturePack(ok);
}
$on_mod(Loaded) { ModLoaded(); }

/*
LocalLevelManager::init called when game starts
there i load all {listingIDs:id}.level files in search paths
loaded json meta data from .level files
is stored in funny `MLE_LevelsInJSON` class from include/cache.hpp
*/

#include <Geode/modify/LocalLevelManager.hpp>
class $modify(MLE_LocalLevelManager, LocalLevelManager) {
    $override bool init() {
        m_mainLevels.clear();
        MLE_LevelsInJSON::get()->clear();

        if (!LocalLevelManager::init()) return false;

        //dont load levels if Loading Layer was not created
        //log::debug("CCFileUtils::get()->m_fullPathCache: {}", CCFileUtils::get()->m_fullPathCache);
        if (not CCFileUtils::get()->m_fullPathCache.contains("goldFont.fnt")) return true;

        auto backupPath = getMod()->getSaveDir() / "settings_backup.json";
        auto currentSettings = getMod()->getSavedSettingsData();

        auto fucku = std::error_code();
        if (not std::filesystem::exists(backupPath, fucku)) {
            if (auto err = file::writeToJson(backupPath, currentSettings).err()) log::warn(
                "Failed to create settings backup: {}", err
            );
            else log::info("Settings backup created at '{}'", backupPath);
        }

        //shared listing setup for example
        log::info("Searching for custom settings enforcement file '{}'...", "settings.json"_spr);
        if (fileExistsInSearchPaths("settings.json"_spr)) {
            //path
            auto path = std::filesystem::path(CCFileUtils::get()->fullPathForFilename(
                "settings.json"_spr, 0
            ).c_str());
            log::info("Found custom settings enforcement file at '{}'!", path);

            //read and overwrite values
            auto dataResult = file::readJson(path);
            if (dataResult.isOk()) {
                auto data = dataResult.unwrap();
                log::info("{}", data.dump());
                for (auto& [key, value] : data) getMod()->getSavedSettingsData().set(key, value);

                //save
                auto saveResult = file::writeToJson(
                    getMod()->getSaveDir() / "settings.json", getMod()->getSavedSettingsData()
                );
                if (saveResult.isErr()) log::error("Failed to save settings: {}", saveResult.unwrapErr());
                else {
                    //reload
                    auto loadResult = getMod()->loadData();
                    if (loadResult.isErr()) log::error("Failed to reload mod data: {}", loadResult.unwrapErr());
                    else log::info("Custom settings enforcement file loaded!");
                }
            }
            else log::error("Failed to read settings file: {}", dataResult.err());
        }
        else {
            log::info("Custom settings file not found, checking for backup...");
            if (std::filesystem::exists(backupPath, fucku)) {
                //replace
                std::filesystem::remove(getMod()->getSaveDir() / "settings.json", fucku);
                std::filesystem::rename(backupPath, getMod()->getSaveDir() / "settings.json", fucku);
                //reload
                auto loadResult = getMod()->loadData();
                if (loadResult.isErr()) log::error("Failed to reload mod data: {}", loadResult.unwrapErr());
                else log::info("Settings successfully restored from backup!");
            }
            else log::info("No backup file found, using current settings");
        }

        log::debug("Loading .level files for list: {}", MLE::getListingIDs());
        for (auto id : MLE::getListingIDs()) {

            log::debug("Loading level {}", id);

            auto level = GJGameLevel::create();
            level->m_levelName = "___level_was_not_loaded";
            level = MLE::tryLoadFromFiles(level, id);

            log::debug("{}", level->m_levelName.c_str());

            if (std::string(level->m_levelName.c_str()) != "___level_was_not_loaded") { // level name was changed if it was loaded
                log::info("Loaded level {}", id);

                if (std::string(level->m_levelString.c_str()).empty()) void();
                else m_mainLevels[id] = level->m_levelString;

                log::debug("Level {} string size is {}", id, std::string(level->m_levelString.c_str()).size());

                auto val = level::jsonFromLevel(level);
                if (auto importinf = level::isImported(level)) val["file"] = importinf->getID();
                else log::error("Level is not imported?.. {}", importinf);
                MLE_LevelsInJSON::get()->insert_or_assign(id, val);

                log::debug("Level {} json dump size is {}", id, MLE_LevelsInJSON::get()->at(id).dump().size());
                log::debug("Level file: {}", id, MLE_LevelsInJSON::get()->at(id)["file"].dump());
            }
            else log::debug("The .level file for {} was not founded", id);
        }

        for (auto shit : CCFileUtils::get()->getSearchPaths()) {
            auto path = std::string(shit.c_str());
            if (auto a = "audio.json"_spr; existsInPaths((path + a).c_str())) {
                MLE_LevelsInJSON::get()->insert_or_assign(
                    "audio"_h, 
                    file::readJson((path + a).c_str()).unwrapOr("")
                );
                MLE_LevelsInJSON::get()->at("audio"_h)["file"] = path + a;
            }
            if (auto a = "artists.json"_spr; existsInPaths((path + a).c_str())) {
                MLE_LevelsInJSON::get()->insert_or_assign(
                    "artists"_h, 
                    file::readJson((path + a).c_str()).unwrapOr("")
                );
                MLE_LevelsInJSON::get()->at("artists"_h)["file"] = path + a;
            }
        };

        if (!MLE_LevelsInJSON::get()->contains("audio"_h)) {
            MLE_LevelsInJSON::get()->insert_or_assign("audio"_h, matjson::Value());
            MLE_LevelsInJSON::get()->at(
                "audio"_h
            )["file"] = string::pathToString(getMod()->getConfigDir() / "audio.json"_spr);
        };
        if (!MLE_LevelsInJSON::get()->contains("artists"_h)) {
            MLE_LevelsInJSON::get()->insert_or_assign("artists"_h, matjson::Value());
            MLE_LevelsInJSON::get()->at(
                "artists"_h
            )["file"] = string::pathToString(getMod()->getConfigDir() / "artists.json"_spr);
        };

        return true;
    }
};

#include <Geode/modify/LoadingLayer.hpp>
class $modify(MLE_LoadingLayer, LoadingLayer) {
    static inline bool AFTER_LOADING = true;
    void reloadc(float) { LocalLevelManager::get()->init(); };
    $override bool init(bool a) {
        if (!LoadingLayer::init(a)) return false;
        this->scheduleOnce(schedule_selector(MLE_LoadingLayer::reloadc), 0.25f);
        AFTER_LOADING = true;
        return true;
    }
};

#include <Geode/modify/MenuLayer.hpp>
class $modify(MLE_MenuLayer, MenuLayer) {
    $override bool init() {
        if (!MenuLayer::init()) return false;
        //add frames form known mods
        if (MLE_LoadingLayer::AFTER_LOADING) {
            MLE_LoadingLayer::AFTER_LOADING = false;
            auto freeDiffID = 10;
            auto diffFrameExists = true;
            auto diffSpriteExists = true;
            do {
                auto name = fmt::format("diffIcon_{:02d}_btn_001.png", freeDiffID++);
                diffFrameExists = CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(name.c_str());
                diffSpriteExists = existsInPaths(name.c_str());
            } while (diffFrameExists or diffSpriteExists);
            freeDiffID--;
            log::debug("Free diff ID: {}", freeDiffID);
            auto list = std::vector<std::string>{
                "uproxide.more_difficulties/MD_Difficulty04Small.png",
                "uproxide.more_difficulties/MD_Difficulty04Small_Legacy.png",
                "uproxide.more_difficulties/MD_Difficulty07Small.png",
                "uproxide.more_difficulties/MD_Difficulty07Small_Legacy.png",
                "uproxide.more_difficulties/MD_Difficulty09Small.png",
                "uproxide.more_difficulties/MD_Difficulty09Small_Legacy.png",
            };
            //"hiimjustin000.demons_in_between/DIB_{01-20}_001.png",
            for (auto i = 1; i <= 20; i++) list.push_back(fmt::format(
                "hiimjustin000.demons_in_between/DIB_{:02d}_001.png", i
            ));
            for (auto targetFrameName : list) {
                auto name = fmt::format("diffIcon_{:02d}_btn_001.png", freeDiffID++);
                auto cache = CCSpriteFrameCache::get();
                if (cache->m_pSpriteFrames->objectForKey(targetFrameName.c_str())) {
                    auto frame = cache->spriteFrameByName(targetFrameName.c_str());
                    if (frame) cache->addSpriteFrame(frame, name.c_str());
                    log::debug("adding {} from {} ({})", name, targetFrameName, frame);
                }
            };
        };
        return true;
    }
};
