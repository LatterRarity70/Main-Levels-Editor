// the custom shared level format ".level" like ".gmd2", saves audio and almost ALL level data.
// created by because of the limitations of ".gmd" format, made same way as that one
#include <level.hpp>

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#include <cache.hpp>

#define REMOVE_UI getMod()->getSettingValue<bool>("REMOVE_UI")
#define VERIFY_LEVEL_INTEGRITY getMod()->getSettingValue<bool>("VERIFY_LEVEL_INTEGRITY")
#define REPLACE_DIFFICULTY_SPRITE getMod()->getSettingValue<bool>("REPLACE_DIFFICULTY_SPRITE")
#define TYPE_AND_ID_HACKS_FOR_SECRET_COINS getMod()->getSettingValue<bool>("TYPE_AND_ID_HACKS_FOR_SECRET_COINS")

#define existsInPaths fileExistsInSearchPaths

// Some helper functions
namespace mle {

    GJGameLevel* tryLoadFromFiles(GJGameLevel* level, int customLvlID = 0) {
        auto levelID = customLvlID ? customLvlID : level->m_levelID;
        auto subdir = "levels/";

    asd:

        auto levelFileName = fmt::format("{}{}.level", subdir, levelID);
        if (fileExistsInSearchPaths(levelFileName.c_str())) {
            auto path = CCFileUtils::get()->fullPathForFilename(levelFileName.c_str(), 0);
            level = level::importLevelFile(path.c_str()).unwrapOr(level);
        }
        else log::debug("don't exists in search paths: {}", levelFileName.c_str());

        auto jsonFileName = fmt::format("{}{}.json", subdir, levelID);
        if (fileExistsInSearchPaths(jsonFileName.c_str())) {
            auto path = CCFileUtils::get()->fullPathForFilename(jsonFileName.c_str(), 0);
            level::updateLevelByJson(file::readJson(path.c_str()).unwrapOr(""), level);
            level::isImported(level, path.c_str());
        }
        else log::debug("don't exists in search paths: {}", jsonFileName.c_str());

        if (subdir != std::string("")) {
            subdir = "";
			goto asd;
        }

        return level;
    };

    GJGameLevel* tryLoadFromFiles(int customLvlID) {
        return tryLoadFromFiles(GJGameLevel::create(), customLvlID);
    }

    std::vector<int> getListingIDs(std::string val = "LEVELS_LISTING") {
        auto rtn = std::vector<int>();

        for (auto entry : string::split(getMod()->getSettingValue<std::string>(val.c_str()), ",")) {
            //sequence
            if (string::contains(entry.c_str(), ":")) {
                auto seq = string::split(entry.c_str(), ":");
                auto start = utils::numFromString<int>(seq[0].c_str()).unwrapOr(0);
                auto end = utils::numFromString<int>(seq[1].c_str()).unwrapOr(0);
                bool ew = start > end;//1:-22
                for (int q = start; ew ? q != (end - 1) : q != (end + 1); ew ? --q : ++q) {
                    auto id = q;
                    //log::debug("{} (\"{}\")", id, entry);
                    rtn.push_back(id);
                }
            }
            //single id
            else {
                auto id = utils::numFromString<int>(entry).unwrapOr(0);
                //log::debug("{} (\"{}\")", id, entry);
                rtn.push_back(id);
            }
        }

        return rtn;
    }
    auto getLevelIDs() { return getListingIDs("LEVELS_LISTING"); }
    auto getAudioIDs() { return getListingIDs("AUDIO_LISTING"); }

}

// genius name
class ConfigureLevelFileDataPopup : public geode::Popup<LevelEditorLayer*, std::filesystem::path> {
protected:
    bool setup(LevelEditorLayer* editor, std::filesystem::path related_File) override {

        auto scroll = ScrollLayer::create({
            this->m_buttonMenu->getContentSize().width * 0.86f,
            this->m_buttonMenu->getContentSize().height - 10.5f,
            });
        scroll->ignoreAnchorPointForPosition(0);
        this->m_buttonMenu->addChildAtPosition(scroll, Anchor::Center, { 0.f, 0.0f });

        auto json = std::shared_ptr<matjson::Value>(
            new matjson::Value(level::jsonFromLevel(editor->m_level))
        );
        for (auto asd : *json.get()) {
            auto key = asd.getKey().value_or("unnamed obj");
            if (string::containsAny(key, { "levelString" })) continue;

            auto layer = CCLayerColor::create({ 0,0,0,42 });
            layer->setContentWidth(scroll->getContentWidth());
            layer->setContentHeight(34.000f);

            if (string::containsAny(key, { "difficulty","stars","requiredCoins","Name","song","sfx","Track" })) layer->setOpacity(90);

            auto keyLabel = SimpleTextArea::create(key);
            keyLabel->setAnchorPoint({ 0.f, 0.5f });
            layer->addChildAtPosition(keyLabel, Anchor::Left, { 12, 0 });

            auto keyInputErr = SimpleTextArea::create("")->getLines()[0];
            keyInputErr->setColor(ccRED);
            keyInputErr->setZOrder(2);
            keyInputErr->setScale(0.6f);
            layer->addChildAtPosition(keyInputErr, Anchor::BottomLeft, { 6.f, 12.f });

            auto keyValInput = TextInput::create(132.f, key, keyLabel->getFont());

            keyValInput->setString(asd.dump());
            keyValInput->setCallback([=](auto str) mutable { //INPUT CALLBACK

                keyInputErr->setString("");

                auto parse = matjson::parse(str);
                if (parse.isOk()) {
                    (*json.get())[key] = parse.unwrapOrDefault();
                    level::updateLevelByJson(json, editor->m_level);
                }
                else {
                    if (parse.err()) keyInputErr->setString(
                        ("parse err: " + parse.err().value().message).c_str()
                    );
                };

                }); //INPUT CALLBACK

            auto bgSize = keyValInput->getBGSprite()->getContentSize();
            keyValInput->getBGSprite()->setSpriteFrame(CCSprite::create("groundSquare_18_001.png")->displayFrame());
            keyValInput->getBGSprite()->setContentSize(bgSize);

            layer->addChildAtPosition(keyValInput, Anchor::Right, { -72.f, 0 });

            layer->setZOrder(scroll->m_contentLayer->getChildrenCount());
            layer->setTag(scroll->m_contentLayer->getChildrenCount());
            scroll->m_contentLayer->addChild(layer);
        }
        scroll->m_contentLayer->setLayout(RowLayout::create()
            ->setGrowCrossAxis(1)
            ->setAxisReverse(1)
        );
        scroll->scrollToTop();

        auto bottomMenuPaddingX = 6.f;
        auto bottomMenuY = -16.f;

        CCMenuItemSpriteExtra* save_level = CCMenuItemExt::createSpriteExtra(
            SimpleTextArea::create("SAVE LEVEL")->getLines()[0],
            [editor, related_File](CCNode* item) {
                auto pause = EditorPauseLayer::create(editor);
                if (!item->getTag()) pause->saveLevel();
                if (auto err = level::exportLevelFile(editor->m_level, related_File).err())
                    Notification::create(
                        "failed to export level: " + err.value_or("unknown error"),
                        NotificationIcon::Error
                    )->show();
                else Notification::create("level saved to file!", NotificationIcon::Info)->show();
                LocalLevelManager::get()->init();
                Notification::create("local level manager was reinitialized", NotificationIcon::Info)->show();
            }
        );
        save_level->m_scaleMultiplier = 0.95;
        save_level->setID("save_level"_spr);
        save_level->setAnchorPoint({ 1.f, 0.5f });
        this->m_buttonMenu->addChildAtPosition(save_level, Anchor::BottomRight, { -bottomMenuPaddingX, bottomMenuY });

        CCMenuItemSpriteExtra* sort = CCMenuItemExt::createSpriteExtra(
            SimpleTextArea::create("[Move to top Main Levels related]")->getLines()[0],
            [scroll](auto) {
                findFirstChildRecursive<CCLayerColor>(scroll->m_contentLayer, [](auto me) {
                    if (me->getOpacity() == 90) me->setZOrder(me->getZOrder() == me->getTag() ? -1 : me->getTag());
                    return false;
                    });
                scroll->m_contentLayer->updateLayout();
                scroll->scrollToTop();
            }
        );
        sort->m_scaleMultiplier = 0.97;
        sort->setID("sort"_spr);
        sort->setAnchorPoint({ 0.f, 0.5f });
        this->m_buttonMenu->addChildAtPosition(sort, Anchor::BottomLeft, { bottomMenuPaddingX, bottomMenuY });

        auto bottomMenuBG = CCScale9Sprite::create("groundSquare_01_001.png");
        bottomMenuBG->setColor(ccBLACK);
        bottomMenuBG->setOpacity(190);
        bottomMenuBG->setID("bottomMenuBG"_spr);
        bottomMenuBG->setContentSize({
                this->m_buttonMenu->getContentSize().width,
                22.000f
            });
        bottomMenuBG->setZOrder(-1);
        this->m_buttonMenu->addChildAtPosition(bottomMenuBG, Anchor::Bottom, { 0.f, bottomMenuY });

        this->m_mainLayer->setPositionY(this->m_mainLayer->getPositionY() + fabs(bottomMenuY / 2));

        return true;
    }

public:
    static ConfigureLevelFileDataPopup* create(LevelEditorLayer* editor, std::filesystem::path related_File) {
        auto ret = new ConfigureLevelFileDataPopup();
        if (ret->initAnchored(410.000f, 262.000f, editor, related_File)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// Setup Menu
#include <MainLevelsEditorMenu.hpp>

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

        log::debug("Loading .level files for list: {}", mle::getListingIDs());
        for (auto id : mle::getListingIDs()) {

            log::debug("Loading level {}", id);

            auto level = GJGameLevel::create();
            level->m_levelName = "___level_was_not_loaded";
            level = mle::tryLoadFromFiles(level, id);

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
    void reloadc(float) { LocalLevelManager::get()->init(); };
    $override bool init(bool a) {
        if (!LoadingLayer::init(a)) return false;
        this->scheduleOnce(schedule_selector(MLE_LoadingLayer::reloadc), 0.25f);
        return true;
    }
};

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
		if (auto as = fmt::format("sfx/{}", id); existsInPaths(as.c_str())) {
			return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0).c_str();
		}
		if (auto as = fmt::format("sfx.{}", id); existsInPaths(as.c_str())) {
			return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0).c_str();
		}
		return MusicDownloadManager::pathForSFX(id).c_str();
    };
    gd::string pathForSong(int id) {
        if (auto as = fmt::format("songs/{}", id); existsInPaths(as.c_str())) {
            return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0).c_str();
        }
		if (auto as = fmt::format("song.{}", id); existsInPaths(as.c_str())) {
			return CCFileUtils::get()->fullPathForFilename(as.c_str(), 0).c_str();
		}
		return MusicDownloadManager::pathForSong(id).c_str();
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

        if (auto as = fmt::format("audio.{}", 0); existsInPaths(as.c_str())) return as.c_str();
        if (auto as = fmt::format("audio/{}", 0); existsInPaths(as.c_str())) return as.c_str();

        if (auto a = audio(p0); a.isOk()) 
            if (a.unwrap().contains("file")) 
                return a.unwrapOr("").get("file").unwrap().asString().unwrapOr("").c_str();

        return LevelTools::getAudioFileName(p0).c_str();
    };
    $override static gd::string getAudioTitle(int p0) {
        if (auto a = audio(p0); a.isOk()) 
            if (a.unwrap().contains("title")) 
                return a.unwrapOr("").get("title").unwrap().asString().unwrapOr("").c_str();
		return LevelTools::getAudioTitle(p0).c_str();
    };
    $override static gd::string urlForAudio(int p0) {
        if (auto a = audio(p0); a.isOk()) 
            if (a.unwrap().contains("url")) 
                return a.unwrapOr("").get("url").unwrap().asString().unwrapOr("").c_str();
        return LevelTools::urlForAudio(p0);
    };

    $override static int artistForAudio(int p0) {
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

/*

The custom level list support!

touching already created and setup LevelSelectLayer's BoomScrollLayer is scary
so i just change what LevelSelectLayer::init gives to BoomScrollLayer::create call

*/

#include <Geode/modify/BoomScrollLayer.hpp>
class $modify(BoomScrollLayerLevelSelectExt, BoomScrollLayer) {
    $override static BoomScrollLayer* create(cocos2d::CCArray * pages, int unk1, bool unk2, cocos2d::CCArray * unk3, DynamicScrollDelegate * delegate) {
        if (delegate and unk3) {
            if (exact_cast<LevelSelectLayer*>(delegate)) { //is created for LevelSelectLayer
                unk3->removeAllObjects();
                for (auto id : mle::getListingIDs()) unk3->addObject(
                    GameLevelManager::get()->getMainLevel(id, 0)
                );
            };
        }
        return BoomScrollLayer::create(pages, unk1, unk2, unk3, delegate);
    }
};

//__int64 __fastcall CustomListView::create(int a1, int a2, __int64 a3, float a4)
#include <Geode/modify/BoomListView.hpp>
class $modify(MLE_BoomListViewExt, BoomListView) {
    void trySetupSongs(CCArray * entries) {
        if (!entries) return; // log::error("entries is null");
        if (!entries->count()) return; //log::error("entries count is 0");

        auto test = typeinfo_cast<SongObject*>(entries->objectAtIndex(0));
        if (!test) return; // log::error("entry1 is not SongObject");

        entries->removeAllObjects();

        for (auto id : mle::getAudioIDs()) {
            auto obj = SongObject::create(id);
            if (obj) entries->addObject(obj);
        }
    }
    $override bool init(
        cocos2d::CCArray * entries, TableViewCellDelegate * delegate,
        float height, float width, int page, BoomListType type, float y
    ) {
        trySetupSongs(entries);
        return BoomListView::init(entries, delegate, height, width, page, type, y);
    }
};

/*

Target page fix and Setup Mode temp UI

*/

#include <Geode/modify/LevelSelectLayer.hpp>
class $modify(MLE_LevelSelectExt, LevelSelectLayer) {

    // shared static vars for all LevelSelectLayer objects (same as in namespace declaration).
    inline static int LastPlayedPage, LastPlayedPageLevelID, ForceNextTo;
    $override void keyDown(cocos2d::enumKeyCodes p0) {
        LevelSelectLayer::keyDown(p0);
        if (auto scroll = typeinfo_cast<BoomScrollLayer*>(this->m_scrollLayer)) {
            MLE_LevelSelectExt::ForceNextTo = scroll->m_page;
        }
    }

    $override bool init(int instpage) {
        /*
        log::debug("page={}", aw);
        log::debug("BoomScrollLayerExt::LastPlayedPage={}", BoomScrollLayerExt::LastPlayedPage);
        log::debug("BoomScrollL::LastPlayedPageLevelID={}", BoomScrollLayerExt::LastPlayedPageLevelID);
        :                             page=332
        BoomScrollL::LastPlayedPageLevelID=333
        BoomScrollLayerExt::LastPlayedPage=0
        */
        if (instpage + 1 == MLE_LevelSelectExt::LastPlayedPageLevelID) {
            instpage = MLE_LevelSelectExt::LastPlayedPage;
        };

        if (ForceNextTo) {
            instpage = ForceNextTo;
            ForceNextTo = 0;
        }

        if (!LevelSelectLayer::init(instpage)) return false;

        if (!REMOVE_UI) {
            auto menu = CCMenu::create();
            menu->setID("menu"_spr);
            menu->setScale(0.75f);
            menu->setAnchorPoint(CCPointZero);
            menu->addChild(MainLevelsEditorMenu::createButtonForMe());
            addChildAtPosition(menu, Anchor::BottomRight, { -25.f, 25.f }, false);
            menu->setZOrder(228);

#if 0 // that shit is too scary and strange to touch
            auto cp = CCControlColourPicker::colourPicker();
            cp->setID("cp"_spr);
            addChildAtPosition(cp, Anchor::BottomLeft, { 95.f, 95.f }, false);
            cp->setScale(0.700f);
            cp->setPositionX(52.000f);
            cp->setPositionY(48.000f);
            cp->runAction(CCRepeatForever::create(CCSequence::create(
                CallFuncExt::create(
                    [cp = Ref(cp), __this = Ref(this)]() {
                        if (!__this or !cp) return;
                        if (!__this->m_scrollLayer) return;
                        int page = ; //pizdec
                        //update color if page changed
						if (cp->getTag() != page) cp->setColorValue(__this->colorForPage(page));
                        //save color changes for page
                        else if (cp->getColorValue() != __this->colorForPage(page)) {
                            auto filename = "level-select-page-colors.json";
                            if (!fileExistsInSearchPaths(filename)) file::writeToJson<matjson::Value>(
                                getMod()->getConfigDir() / filename, {}
                            ).isOk();
                            auto path = CCFileUtils::get()->fullPathForFilename(filename, 0);
                            auto colors = file::readJson(path).unwrapOrDefault();
                            auto c = cp->getColorValue();
                            colors.set(
                                utils::numToString(page),
                                matjson::parse(fmt::format("[{}, {}, {}]", c.r, c.g, c.b)).unwrapOr("err")
                            );
							file::writeToJson(path, colors).isOk();
                            __this->scrollLayerMoved(__this->m_scrollLayer->m_position);
                        }
                        cp->setTag(page);
                    }
                ), CCDelayTime::create(0.1f), nullptr
            )));
#endif
		}

        return true;
    }

#if 0 // THAT SHIT IS TOO SCARY AND STRANGE TO TOUCH
    $override cocos2d::ccColor3B colorForPage(int page) {
        page = std::max(0, page); //-1 is crashing.. looks like stack overflow thing idk
        auto color = LevelSelectLayer::colorForPage(page);

        //super silent mode
		auto colors = file::readJson(CCFileUtils::get()->fullPathForFilename(
            "level-select-page-colors.json", 0
        )).unwrapOrDefault();
		auto c = colors.get<matjson::Value>(utils::numToString(page)).unwrapOr("");
        color.r = c.get<int>(0).unwrapOr(color.r); 
		color.g = c.get<int>(1).unwrapOr(color.g);
		color.b = c.get<int>(2).unwrapOr(color.b);

		return color;
    }
#endif

};

/*

LevelPage Mods:

- replace difficulty sprite impl
- temp id debug for setup mode

Also sets MLE_LevelSelectExt::LastPlayedPage/LastPlayedPageLevelID/ForceNextTo
That will be used in next LevelSelectLayer objects to fix page select stuff

*/

#include <Geode/modify/LevelPage.hpp>
class $modify(MLE_LevelPageExt, LevelPage) {

    void customSetup(float) { //dynamic page update function is hell
        Ref level = this->m_level;
        if (!level) return;
        if (!this->getChildByType<GJGameLevel>(0)) this->addChild(level);
        else {
            if (level == this->getChildByType<GJGameLevel>(0)) return;
            else {
                this->getChildByType<GJGameLevel>(0)->removeFromParent();
                this->addChild(level);
            }
        }
        //difficultySprite
        if (REPLACE_DIFFICULTY_SPRITE) if (auto difficultySprite = typeinfo_cast<CCSprite*>(this->getChildByIDRecursive("difficulty-sprite"))) {
            auto diffID = static_cast<int>(level->m_difficulty);
            if (difficultySprite->getTag() != diffID) {
                difficultySprite->setTag(diffID);
                auto sz = difficultySprite->getContentSize();
                auto frameName = fmt::format("diffIcon_{:02d}_btn_001.png", diffID);
                if (CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(frameName.c_str())) {
                    auto frame = CCSpriteFrameCache::get()->spriteFrameByName(frameName.c_str());
                    if (frame) difficultySprite->setDisplayFrame(frame);
                }
                else {
                    auto image = CCSprite::create(frameName.c_str());
                    if (image) difficultySprite->setDisplayFrame(image->displayFrame());
                }
                limitNodeSize(difficultySprite, sz, 999.f, 0.1f);
            }
        }
        //debg
        if (!REMOVE_UI) {
            while (auto a = this->getChildByTag("mle-id-debug"_h)) a->removeFromParent();
            this->addChild(SimpleTextArea::create(fmt::format(
                "                                                                       "
                "id: {}\n \n \n \n \n \n \n "
                , level->m_levelID.value()
            )), 1, "mle-id-debug"_h);
        }
    }

    $override bool init(GJGameLevel* level)  {
		if (!LevelPage::init(level)) return false;
		this->schedule(schedule_selector(MLE_LevelPageExt::customSetup));
		return true;
	}

    $override void onPlay(cocos2d::CCObject * sender) {
        if (auto a = getParent()) if (auto scroll = typeinfo_cast<BoomScrollLayer*>(a->getParent())) {
            MLE_LevelSelectExt::LastPlayedPage = scroll->pageNumberForPosition(this->getPosition());
            MLE_LevelSelectExt::LastPlayedPageLevelID = this->m_level->m_levelID.value();
        }
        LevelPage::onPlay(sender);
    }

    void saveCurrentPageForForceNextTo() {
        if (auto a = getParent()) if (auto scroll = typeinfo_cast<BoomScrollLayer*>(a->getParent())) {
            MLE_LevelSelectExt::ForceNextTo = scroll->pageNumberForPosition(this->getPosition());
        }
    }

    $override void onSecretDoor(cocos2d::CCObject * sender) {
        saveCurrentPageForForceNextTo();
        LevelPage::onSecretDoor(sender);
    }

    $override void onTheTower(cocos2d::CCObject * sender) {
        saveCurrentPageForForceNextTo();
        LevelPage::onTheTower(sender);
    }

};

/*

If editor was created for edit .level feature 
this allows user to edit meta data of level 
by showing ConfigureLevelFileDataPopup over default settings

ConfigureLevelFileDataPopup can be closed to get access to default level settings menu

*/

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(MLE_EditorPauseLayer, EditorPauseLayer) {
    $override void saveLevel() {
        EditorPauseLayer::saveLevel();

        //impinfo in level object was created at .level import function
        if (auto impinfo = level::isImported(m_editorLayer->m_level)) {
            auto k = ConfigureLevelFileDataPopup::create(this->m_editorLayer, impinfo->getID());
            if (k) if (auto a = typeinfo_cast<CCMenuItem*>(k->querySelector("save_level"_spr))) {
                a->setTag(true); //disable saveLevel call..
                a->activate();
            }
        }
    }
};

#include <Geode/modify/EditorUI.hpp>
class $modify(MLE_EditorUI, EditorUI) {
    inline static bool isJSON = false;
    void showInfoPopup(float) {
        MDPopup::create(
            isJSON ? 
            "Welcome to level editor for .json file" :
            "Welcome to level editor for .level file",
            R"(Open default <cg>Level Settings</c> to open tools that help you edit level file:
- Meta data editor
- Difficulty sprite selector
- Coins replace tool)", "OK")->show();
    }
    $override bool init(LevelEditorLayer * editorLayer) {
		if (!EditorUI::init(editorLayer)) return false;
        if (auto impinfo = level::isImported(editorLayer->m_level)) {
            isJSON = string::endsWith(impinfo->getID(), ".json");
            this->scheduleOnce(schedule_selector(MLE_EditorUI::showInfoPopup), 1.f);
        }
		return true;
    }
    $override void onSettings(cocos2d::CCObject * sender) {
        EditorUI::onSettings(sender);

        //impinfo in level object was created at .level import function
        if (auto impinfo = level::isImported(m_editorLayer->m_level)) {
            //coins replacer
            if (TYPE_AND_ID_HACKS_FOR_SECRET_COINS) createQuickPopup(
                "Replace coins?",
                "Here you can change all\n<co>User Coins</c>\nin level to\n<cy>Secret Coins</c>",
                "Nah", "Replace", [editor = Ref(m_editorLayer), impinfo = Ref(impinfo)](void*, bool replace) {
                    if (!replace) return;
                    auto replaced = 0;
                    for (auto obj : CCArrayExt<GameObject*>(editor->m_objects)) if (obj) {
                        if (obj->m_objectID == 1329) {
                            obj->m_savedObjectType = GameObjectType::SecretCoin;
                            obj->m_objectType = GameObjectType::SecretCoin;
                            obj->m_objectID = 142;
                            obj->customSetup();
                            if (auto e = typeinfo_cast<EffectGameObject*>(obj)) e->m_secretCoinID = ++replaced;
                        }
                    };
                    Notification::create(
                        fmt::format("Changed {} coins", replaced),
                        CCSprite::createWithSpriteFrameName("GJ_coinsIcon_001.png")
                    )->show();
                    editor->m_level->m_coins = replaced;
                    EditorPauseLayer::create(editor)->saveLevel();
                    level::exportLevelFile(Ref(editor->m_level), impinfo->getID()).isOk();
                }
            );
            //difficulty sprite selector
            class DiffcltySelector : public Popup<GJGameLevel*, std::filesystem::path> {
                void scrollWheel(float x, float y) override {
                    if (std::fabs(x) > 5.f) if (auto a = m_buttonMenu) if (auto item = a->getChildByType<CCMenuItem>(
                        1 + (x < 0.f)
                    )) item->activate();
                }
                bool setup(GJGameLevel* level, std::filesystem::path related_File) override {
                    this->setTitle("Select Difficulty Sprite");
                    this->setMouseEnabled(true);

                    Ref preview = CCSprite::create("diffIcon_01_btn_001.png");
                    this->m_buttonMenu->addChildAtPosition(preview, Anchor::Center, { 0.f, 20.000f });
                    preview->setScale(1.350f);

                    Ref name = CCLabelBMFont::create("diffIcon_01_btn_001.png", "chatFont.fnt");
                    this->m_buttonMenu->addChildAtPosition(name, Anchor::Bottom, { 0.f, 69.000f });

                    auto updatePreview = [=](bool zxc = false) { start:
						auto frameName = fmt::format("diffIcon_{:02d}_btn_001.png", (int)level->m_difficulty);
                        if (preview) {
                            if (CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(frameName.c_str())) {
                                auto frame = CCSpriteFrameCache::get()->spriteFrameByName(frameName.c_str());
                                if (frame) preview->setDisplayFrame(frame);
                            }
                            else if(fileExistsInSearchPaths(frameName.c_str())) {
                                auto image = CCSprite::create(frameName.c_str());
                                if (image) preview->setDisplayFrame(image->displayFrame());
                            }
                            else if (level->m_difficulty != GJDifficulty::Auto) {
                                level->m_difficulty = GJDifficulty::Auto;
                                goto start;
                            }
                        };
                        if (name) name->setString(frameName.c_str());
					};
                    updatePreview();

                    this->m_buttonMenu->addChildAtPosition(CCMenuItemExt::createSpriteExtra(
                        CCLabelBMFont::create("<", "bigFont.fnt"), [=, level = Ref(level)](void*) { 
                            level->m_difficulty = (GJDifficulty)((int)level->m_difficulty - 1); updatePreview(); 
                        }
                    ), Anchor::Left, { 32.f, 0.f });

                    this->m_buttonMenu->addChildAtPosition(CCMenuItemExt::createSpriteExtra(
                        CCLabelBMFont::create(">", "bigFont.fnt"), [=, level = Ref(level)](void*) {
                            level->m_difficulty = (GJDifficulty)((int)level->m_difficulty + 1); updatePreview();
                        }
                    ), Anchor::Right, { -32.f, 0.f });

                    this->m_buttonMenu->addChildAtPosition(CCMenuItemExt::createSpriteExtra(
                        ButtonSprite::create("   Save   ", "bigFont.fnt", "GJ_button_05.png", 0.6f), 
                        [this, level, related_File](void*) {
                            Ref(this)->keyBackClicked();
                            level::exportLevelFile(Ref(level), related_File).isOk();
                        }
                    ), Anchor::Bottom, { 0.f, 32.f });

					return true;
                }
            public:
                static DiffcltySelector* create(GJGameLevel* level, std::filesystem::path related_File) {
                    auto ret = new DiffcltySelector();
                    if (ret->initAnchored(266.6f, 169.000f, level, related_File)) {
                        ret->autorelease();
                        return ret;
                    }
                    delete ret;
                    return nullptr;
                }
            };
            if (REPLACE_DIFFICULTY_SPRITE) DiffcltySelector::create(Ref(m_editorLayer->m_level), impinfo->getID())->show();
            // full meta data editor
            ConfigureLevelFileDataPopup::create(
                this->m_editorLayer, impinfo->getID()
            )->show();
        }
    };
};

/*

Temp Setup UI for users

*/

#include <Geode/modify/PauseLayer.hpp>
class $modify(MLE_PauseExt, PauseLayer) {
    $override void customSetup() {
        PauseLayer::customSetup();

        if (!REMOVE_UI) {
            auto menu = CCMenu::create();
            menu->setID("menu"_spr);
            menu->setScale(0.75f);
            menu->setAnchorPoint(CCPointZero);
            menu->addChild(MainLevelsEditorMenu::createButtonForMe());
            addChildAtPosition(menu, Anchor::BottomRight, { -25.f, 25.f }, false);
            menu->setZOrder(228);
        }
    };
};


/*

Type and ID hacks for Secret Coins

Hack around type and id of Secret Coin objects to bypass some game anti-cheat logic. 
Basically aims to force game add them to level as User Coins.

*/

#include <Geode/modify/GameObject.hpp>
class $modify(MLE_GameObjectExt, GameObject) {

    void customSetup(float) { customSetup(); };
    $override void customSetup() {
        if (auto v = this->getUserObject("org-"_spr + std::string("m_objectID"))) this->m_objectID = v->getTag();
        if (auto v = this->getUserObject("org-"_spr + std::string("m_objectType"))) this->m_objectType = (GameObjectType)v->getTag();
        if (auto v = this->getUserObject("org-"_spr + std::string("m_savedObjectType"))) this->m_savedObjectType = (GameObjectType)v->getTag();
        GameObject::customSetup();
    };

    static auto valTagContainerObj(int val) { auto a = new CCObject(); a->autorelease(); a->setTag(val); return a; };

    void PlayLayerCustomSetup(float) {
        if (auto play = typeinfo_cast<PlayLayer*>(this)) {
            auto lvl = play->m_level;
            if (auto v = lvl->getUserObject("org-"_spr + std::string("m_localOrSaved"))) lvl->m_localOrSaved = v->getTag();
            if (auto v = lvl->getUserObject("org-"_spr + std::string("m_levelType"))) lvl->m_levelType = (GJLevelType)v->getTag();
        }
    };

    $override static GameObject* objectFromVector(
        gd::vector<gd::string>&p0, gd::vector<void*>&p1, GJBaseGameLayer * p2, bool p3
    ) {
        auto rtn = GameObject::objectFromVector(p0, p1, p2, p3);
        if (!rtn) return rtn;
        if (TYPE_AND_ID_HACKS_FOR_SECRET_COINS) {
            if (auto editor = typeinfo_cast<LevelEditorLayer*>(p2)) {
                if (rtn) if (rtn->m_objectID == 142) {
                    rtn->setUserObject("org-"_spr + std::string("m_objectID"), valTagContainerObj(rtn->m_objectID));
                    rtn->m_objectID = 1329; //user coin object id
                    rtn->setUserObject("org-"_spr + std::string("m_objectType"), valTagContainerObj((int)rtn->m_objectType));
                    rtn->m_objectType = GameObjectType::UserCoin; //user coin object
                    rtn->setUserObject("org-"_spr + std::string("m_savedObjectType"), valTagContainerObj((int)rtn->m_savedObjectType));
                    rtn->m_savedObjectType = GameObjectType::UserCoin; //what
                    rtn->scheduleOnce(schedule_selector(MLE_GameObjectExt::customSetup), 0.f);
                }
            };
            if (auto play = typeinfo_cast<PlayLayer*>(p2)) {
                if (rtn) if (rtn->m_objectID == 142) {
                    auto lvl = play->m_level;
                    lvl->setUserObject("org-"_spr + std::string("m_localOrSaved"), valTagContainerObj(lvl->m_localOrSaved));
                    lvl->m_localOrSaved = true;
                    lvl->setUserObject("org-"_spr + std::string("m_levelType"), valTagContainerObj((int)lvl->m_levelType));
                    lvl->m_levelType = GJLevelType::Main;
                    play->scheduleOnce(schedule_selector(MLE_GameObjectExt::PlayLayerCustomSetup), 0.f);
                }
            }
        }
        return rtn;
    }
};