#pragma once

// the custom shared level format ".level" like ".gmd2", saves audio and almost ALL level data.
// created by because of the limitations of ".gmd" format, made same way as that one
#include <level.hpp>
#include <cache.hpp>

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#define REMOVE_UI getMod()->getSettingValue<bool>("REMOVE_UI")
#define VERIFY_LEVEL_INTEGRITY getMod()->getSettingValue<bool>("VERIFY_LEVEL_INTEGRITY")
#define REPLACE_DIFFICULTY_SPRITE getMod()->getSettingValue<bool>("REPLACE_DIFFICULTY_SPRITE")
#define THE_DATA_DRIVEN_ACHIEVEMENTS getMod()->getSettingValue<bool>("THE_DATA_DRIVEN_ACHIEVEMENTS")
#define TYPE_AND_ID_HACKS_FOR_SECRET_COINS getMod()->getSettingValue<bool>("TYPE_AND_ID_HACKS_FOR_SECRET_COINS")

#define existsInPaths fileExistsInSearchPaths

// Some helper functions
namespace MLE {

    inline GJGameLevel* tryLoadFromFiles(GJGameLevel* level, int customLvlID = 0) {
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

    inline GJGameLevel* tryLoadFromFiles(int customLvlID) {
        return tryLoadFromFiles(GJGameLevel::create(), customLvlID);
    }

    inline std::string createListingIDs(const std::vector<int>& list) {
        std::string new_listing;
        for (auto it = list.begin(); it != list.end(); ) {
            int start = *it, end = start;
            while (++it != list.end() && *it == end + 1) end++;

            if (end - start < 2) { // 1-2 IDs
                new_listing += fmt::format("{}{}", start, end > start ? fmt::format(",{}", end) : "");
            }
            else { // 3+
                new_listing += fmt::format("{}:{}", start, end);
            }
            new_listing += ",";
        }
        if (!new_listing.empty()) new_listing.pop_back(); // remove last comma)
        return new_listing;
    }

    inline void updateListingIDs(const std::vector<int>& list, std::string val = "LEVELS_LISTING") {
        if (cocos::fileExistsInSearchPaths((val + ".txt").c_str())) file::writeStringSafe(
            CCFileUtils::get()->fullPathForFilename((val + ".txt").c_str(), 0).c_str(),
            createListingIDs(list)
        ).err();
        else {
            Mod::get()->setSettingValue<std::string>(val, createListingIDs(list));
            Mod::get()->saveData().isOk();
        }
    }

    inline std::vector<int> getListingIDs(std::string val = "LEVELS_LISTING") {
        auto rtn = std::vector<int>();

        auto list = file::readString(CCFileUtils::get()->fullPathForFilename(
            (val + ".txt").c_str(), 0
        ).c_str()).unwrapOr(
            getMod()->getSettingValue<std::string>(val.c_str())
        );

        for (auto entry : string::split(list, ",")) {
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
    inline auto getLevelIDs() { return getListingIDs("LEVELS_LISTING"); }
    inline auto getAudioIDs() { return getListingIDs("AUDIO_LISTING"); }

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

            keyValInput->setFilter(" !\"#$ % &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
            keyValInput->getInputNode()->m_allowedChars = " !\"#$ % &'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
            keyValInput->setString(asd.dump());
            keyValInput->setCallback(
                [=](auto str) mutable { //INPUT CALLBACK

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
                }
            ); //INPUT CALLBACK

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
                if (!editor) return;
                if (!editor->isRunning()) return;
                if (item->getTag() == "DontSaveLevel"_h) void();
				else EditorPauseLayer::create(editor)->saveLevel();
                if (auto err = level::exportLevelFile(editor->m_level, related_File).err())
                    Notification::create(
                        "  Failed to export level: \n  " + err.value_or("unknown error"),
                        NotificationIcon::Error
                    )->show();
                else
                    Notification::create(
                        "Level saved to file!",
                        NotificationIcon::Info
                    )->show();
                LocalLevelManager::get()->init();
                Notification::create("Local level manager was reinitialized", NotificationIcon::Info)->show();
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
        sort->activate();
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
#include <_MainLevelsEditorMenu.hpp>
