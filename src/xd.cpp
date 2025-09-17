

// the custom shared level format ".level" like ".gmd2", saves audio and almost ALL level data.
// created by because of the limitations of ".gmd" format, made same way as that one
#include <level.hpp>

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#define REMOVE_UI getMod()->getSettingValue<bool>("REMOVE_UI")
#define VERIFY_LEVEL_INTEGRITY getMod()->getSettingValue<bool>("VERIFY_LEVEL_INTEGRITY")
#define REPLACE_DIFFICULTY_SPRITE getMod()->getSettingValue<bool>("REPLACE_DIFFICULTY_SPRITE")
#define TYPE_AND_ID_HACKS_FOR_SECRET_COINS getMod()->getSettingValue<bool>("TYPE_AND_ID_HACKS_FOR_SECRET_COINS")

// Some helper functions
namespace mle {

    GJGameLevel* tryLoadFromFiles(GJGameLevel* level, int customLvlID = 0, bool appendSubDir = false) {
        auto levelID = customLvlID ? customLvlID : level->m_levelID;
        auto subdir = appendSubDir ? "levels/" : "";

        auto levelFileName = fmt::format("{}{}.level", subdir, levelID);
        if (fileExistsInSearchPaths(levelFileName.c_str())) {
            auto path = CCFileUtils::get()->fullPathForFilename(levelFileName.c_str(), 0);
            level = level::importLevelFile(path.c_str()).unwrapOr(level);
        }
        else log::debug("don't exists in search paths: {}", levelFileName.c_str());

        if (!appendSubDir) return tryLoadFromFiles(level, customLvlID, true);
        return level;
    };

    GJGameLevel* tryLoadFromFiles(int customLvlID) {
        return tryLoadFromFiles(GJGameLevel::create(), customLvlID);
    }

    std::vector<int> getListingIDs() {
        auto rtn = std::vector<int>();

        for (auto entry : string::split(getMod()->getSettingValue<std::string>("LEVELS_LISTING"), ",")) {
            //sequence
            if (string::contains(entry, ":")) {
                auto seq = string::split(entry, ":");
                auto start = utils::numFromString<int>(seq[0]).unwrapOr(0);
                auto end = utils::numFromString<int>(seq[1]).unwrapOr(0);
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
            [editor, related_File](auto) {
                editor->getLevelString();
                auto pause = EditorPauseLayer::create(editor);
                pause->saveLevel();
                auto result = level::exportLevelFile(editor->m_level, related_File);
                if (result.isErr()) Notification::create(
                    "failed to export level: " + result.err().value_or("unknown error"),
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
class MainLevelsEditorMenu : public geode::Popup<> {
protected:
    using FileEvent = Task<Result<std::filesystem::path>>;
    EventListener<FileEvent> m_listener;
    bool setup() override {
        this->setTitle("");

        auto menu = CCMenu::create();
        menu->setContentWidth(260.000f);
        menu->setContentHeight(262.000f);
        this->m_mainLayer->addChildAtPosition(menu, Anchor::Center, { 0.f, 0.f });

        auto lopts = AxisLayoutOptions::create()
            ->setScaleLimits(0.1f, 1.5f)
            ->setAutoScale(true);

        auto titlespr = [lopts](const char* a) {
            auto aw = ButtonSprite::create(a, "bigFont.fnt", "GJ_button_03.png");
			aw->setLayoutOptions(lopts);
            return aw;
            };

        auto btnspr = [lopts](const char* a) {
            auto aw = ButtonSprite::create(a, "bigFont.fnt", "GJ_button_05.png");
            aw->setLayoutOptions(lopts);
            return aw;
            };

        menu->addChild(titlespr("Main Levels Editor"));

        CCMenuItemSpriteExtra* settings = CCMenuItemExt::createSpriteExtra(
            btnspr("Open Settings"),
            [__this = Ref(this)](auto) {
                openSettingsPopup(getMod(), 1);
            }
        );
        settings->setLayoutOptions(lopts);
        settings->setID("settings"_spr);
        menu->addChild(settings);

        CCMenuItemSpriteExtra* reload_levels_cache = CCMenuItemExt::createSpriteExtra(
            btnspr("Reload levels cache!"),
            [__this = Ref(this)](auto) {
                LocalLevelManager::get()->init();
                if (auto s = CCScene::get()) if (auto l = s->getChildByType<LevelSelectLayer>(
                    0 // LevelSelectLayer
                )) l->keyBackClicked();
                Notification::create("local level manager was reinitialized", NotificationIcon::Info)->show();
            }
        );
		reload_levels_cache->setLayoutOptions(lopts);
        reload_levels_cache->setID("reload_levels_cache"_spr);
        menu->addChild(reload_levels_cache);

        static Ref shReload = reload_levels_cache;
        shReload = reload_levels_cache;

        menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6));

        menu->addChild(titlespr("Shared Level Files"));

        CCMenuItemSpriteExtra* load_level = CCMenuItemExt::createSpriteExtra(
            btnspr("Open .level file"),
            [__this = Ref(this)](auto) {
                auto IMPORT_PICK_OPTIONS = file::FilePickOptions{
                    std::nullopt, {{ "Extended Shared Level File", { "*.level" } }}
                };
                __this->m_listener.bind([](FileEvent::Event* e) {
                    if (e->getValue()) {
                        if (e->getValue()->isOk()) {
                            auto path = e->getValue()->unwrap();
                            auto exist = CCFileUtils::get()->isFileExist(string::pathToString(path).c_str());
                            if (!string::endsWith(string::pathToString(path), ".level") and !exist) {
                                path = std::filesystem::path(string::pathToString(path) + ".level");
                            }
                            auto level_import = level::importLevelFile(path);
                            if (level_import.isOk()) {
                                auto level = level_import.unwrapOrDefault();
                                auto pages = CCArray::create();
                                pages->addObject(LevelInfoLayer::create(level, 0));
                                pages->addObject(EditLevelLayer::create(level));
                                pages->addObject([level] {
                                    auto a = LevelPage::create(level);
                                    a->updateDynamicPage(level);
                                    return a;
                                    }());
                                auto layer = BoomScrollLayer::create(pages, 0, 0);
                                layer->m_extendedLayer->runAction(CCSequence::create(
                                    CCEaseBackOut::create(CCMoveBy::create(1.0f, { -42.f, 0.f })),
                                    CCEaseExponentialIn::create(CCMoveBy::create(0.5f, { 42.f, 0.f })),
                                    CallFuncExt::create([layer] { layer->moveToPage(layer->m_page); }),
                                    nullptr
                                ));
                                layer->addChild(createLayerBG(), -36);
                                layer->setPagesIndicatorPosition({ 74.f, layer->getContentHeight() - (320.000f - 312.000f) });
                                {
                                    auto dotsBG = CCScale9Sprite::create("square02_small.png");
                                    dotsBG->setPosition(layer->m_dotPosition);
                                    dotsBG->setAnchorPoint(CCPointMake(0.5f, 0.1f));
                                    dotsBG->setContentSize(CCSizeMake(52.f, 77.f));
                                    dotsBG->setOpacity(122);
                                    layer->addChild(dotsBG, 0);
                                }
                                switchToScene(layer);
                            }
                            else {
                                MDPopup::create("Failed to load level!", level_import.err().value_or("UNK ERROR"), "OK")->show();
                            }
                        }
                        else log::error("Something went wrong when picking files: {}", e->getValue()->err());
                    }
                    else log::error("Something went wrong when picking files: Value is empty.");
                    });
                __this->m_listener.setFilter(file::pick(file::PickMode::OpenFile, IMPORT_PICK_OPTIONS));
            }
        );
		load_level->setLayoutOptions(lopts);
        load_level->setID("load_level"_spr);
        menu->addChild(load_level);

        CCMenuItemSpriteExtra* edit_level = CCMenuItemExt::createSpriteExtra(
            btnspr("Edit .level file"),
            [__this = Ref(this)](auto) {
                auto IMPORT_PICK_OPTIONS = file::FilePickOptions{
                    std::nullopt, {{ "Extended Shared Level File", { "*.level" } }}
                };
                __this->m_listener.bind([](FileEvent::Event* e) {
                    if (e->getValue()) {
                        if (e->getValue()->isOk()) {
                            auto path = e->getValue()->unwrap();
                            auto exist = CCFileUtils::get()->isFileExist(string::pathToString(path).c_str());
                            if (!string::endsWith(string::pathToString(path), ".level") and !exist) {
                                path = std::filesystem::path(string::pathToString(path) + ".level");
                            }
                            auto level_import = level::importLevelFile(path);
                            if (level_import.isOk()) {
                                auto level = level_import.unwrapOrDefault();
                                auto layer = LevelEditorLayer::create(level, 0);
                                switchToScene(layer);
                            }
                            else {
                                MDPopup::create("Failed to load level!", level_import.err().value_or("UNK ERROR"), "OK")->show();
                            }
                        }
                        else log::error("Something went wrong when picking files: {}", e->getValue()->err());
                    }
                    else log::error("Something went wrong when picking files: Value is empty.");
                    });
                __this->m_listener.setFilter(file::pick(file::PickMode::OpenFile, IMPORT_PICK_OPTIONS));
            }
        );
		edit_level->setLayoutOptions(lopts);
        edit_level->setID("edit_level"_spr);
        menu->addChild(edit_level);

        menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6));

        menu->addChild(titlespr("Act. With Current Level"));

        menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 12));

        auto id_input = TextInput::create(350.000f,
            "        Level ID to Insert At:\nLeave blank to add as new level\nor -1 to add as new in front of list\n "
        );
        id_input->setLayoutOptions(lopts);
        id_input->setFilter("0123456789-");
        id_input->setID("level_id_input"_spr);
        menu->addChild(id_input);

        CCMenuItemSpriteExtra* insert_to_level_list = CCMenuItemExt::createSpriteExtra(
            btnspr("Insert to Level List"),
            [__this = Ref(this), id_input](CCMenuItem*) {
                if (!GameManager::get()->getGameLayer()) {
                    return Notification::create("You are not in a level", NotificationIcon::Error)->show();
                }

                auto level = GameManager::get()->getGameLayer()->m_level;

                // parse input
                std::string input = id_input->getString();
                bool add_new = input.empty();
                bool add_front = input == "-1";
                auto target_id = utils::numFromString<int>(
                    add_front ? "" : input
                ).unwrapOr(
                    level->m_levelID.value()
                );

                // export
                auto export_result = level::exportLevelFile(
                    level, getMod()->getConfigDir() / fmt::format("{}.level", target_id)
                );
                if (!export_result) return Notification::create(
                    "Failed to export level\n" + export_result.err().value_or("UNK ERROR")
                    , NotificationIcon::Error)->show();

                auto list = mle::getListingIDs();

                if (add_new) {
                    auto endp = std::find(list.begin(), list.end(), -1);
                    if (endp == list.end()) endp = std::find(list.begin(), list.end(), -2);

                    list.insert(endp == list.end() ? endp : endp - 1, target_id);
                }
                else if (add_front) {
                    list.insert(list.begin(), target_id);
                }
                else {
                    auto tarp = std::find(list.begin(), list.end(), target_id);
					list.erase(tarp);
					list.insert(tarp, target_id);
                }

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

                Mod::get()->setSettingValue<std::string>("LEVELS_LISTING", new_listing);
                Mod::get()->saveData().isOk();

                Notification::create("Level inserted to list!", NotificationIcon::Success)->show(); 
                
                shReload->activate();
            }
        );
		insert_to_level_list->setLayoutOptions(lopts);
        insert_to_level_list->setID("insert_to_level_list"_spr);
        menu->addChild(insert_to_level_list);

        CCMenuItemSpriteExtra* export_level = CCMenuItemExt::createSpriteExtra(
            btnspr("Export into .level file"),
            [__this = Ref(this)](CCMenuItem*) {
                if (!GameManager::get()->getGameLayer()) {
                    Notification::create("You are not in a level", NotificationIcon::Error)->show();
                    return;
                }
                Ref level = GameManager::get()->getGameLayer()->m_level;
                auto IMPORT_PICK_OPTIONS = file::FilePickOptions{
                    getMod()->getConfigDir() / fmt::format("{}.level", level->m_levelID.value()),
                    {{ "Extended Shared Level File", { "*.level" } }}
                };
                __this->m_listener.bind([level](FileEvent::Event* e) {
                    if (e->getValue()) {
                        if (e->getValue()->isOk()) {
                            //path
                            auto path = e->getValue()->unwrap(); 
                            path = string::endsWith(string::pathToString(path), ".level"
                            ) ? string::pathToString(path) : (string::pathToString(path) + ".level");
                            //dir
                            auto dir = path.parent_path();
                            //exporting.
                            auto level_export = level::exportLevelFile(level, path);
                            if (level_export.isOk()) {
                                auto dbg_json = level_export.unwrapOrDefault();
                                dbg_json["levelString"] = dbg_json["levelString"].asString().unwrapOrDefault().erase(36, 9999999) + "...";

                                auto body = std::stringstream();

                                body << """" "`  File:` [" + path.string() + "](file:///" + string::replace(path.string(), " ", "%20") + ")";
                                body << "\n";
                                body << "\n" "`   Dir:` [" + dir.string() + "](file:///" + string::replace(dir.string(), " ", "%20") + ")";
                                body << "\n";
                                body << "\n" "```";
                                body << "\n" "zip tree of \"" << std::filesystem::path(path).filename().string() << "\": ";
                                auto unzip = file::Unzip::create(string::pathToString(path));
                                if (unzip.err()) body
                                    << "\n" "FAILED TO OPEN CREATED ZIP!"
                                    << "\n" << unzip.err().value_or("unk err");
                                else for (auto entry : unzip.unwrap().getEntries()) body
                                    << "\n- " << string::pathToString(entry);
                                body << "\n" "```";
                                body << "\n";
                                body << "\n" "```";
                                body << "\n" "data \"" << std::filesystem::path(path).filename().string() << "\": ";
                                for (auto entry : dbg_json) body
                                    << "\n- " << entry.getKey().value_or("unk") + ": " << entry.dump();
                                body << "\n" "```";

                                MDPopup::create(
                                    "Level Exported!",
                                    body.str(),
                                    "Ok"
                                )->show();

                                shReload->activate();
                            }
                            else {
                                //aaaa msg
                                Notification::create("failed to save level!", NotificationIcon::Warning)->show();
                                //and err
                                if (level_export.err()) Notification::create(
                                    level_export.err().value_or("UNK ERROR")
                                    , NotificationIcon::Error
                                )->show();
                                log::error("{}", level_export.err());
                            }
                        }
                        else {
                            log::error("Something went wrong when picking files: {}", e->getValue()->err());
                        }
                    }
                    else {
                        log::error("Something went wrong when picking files: Value is empty.");
                    }
                    });
                __this->m_listener.setFilter(file::pick(file::PickMode::SaveFile, IMPORT_PICK_OPTIONS));
            }
        );
        export_level->setLayoutOptions(lopts);
        export_level->setID("export_level"_spr);
        menu->addChild(export_level);

        menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6));

        menu->addChild(titlespr("Create Shared Levels Pack..."));
        
        CCMenuItemSpriteExtra* tp_create = CCMenuItemExt::createSpriteExtra(
            btnspr("In resource pack (TP)"),
            [__this = Ref(this)](CCMenuItem*) -> void {
                auto err = std::error_code();
                namespace fs = std::filesystem;
                auto packs = getMod()->getConfigDir().parent_path() / "geode.texture-loader" / "packs";
                auto pack = packs / fmt::format(
                    "level pack by {} #{}", GameManager::get()->m_playerName.c_str(),
                    std::chrono::system_clock::now().time_since_epoch().count()
                );
                fs::create_directories(pack, err);
                //copy levels
                for (auto id : mle::getListingIDs()) {
                    if (auto impinfo = mle::tryLoadFromFiles(id)->getChildByTag("is-imported-from-file"_h)) {
                        fs::copy_file(impinfo->getID(), pack / fmt::format("{}.level", id), err);
                    }
                };
                //create settings
                fs::create_directories(pack / getMod()->getID(), err);
                if (auto err = file::writeToJson(
                    pack / getMod()->getID() / "settings.json",
                    getMod()->getSavedSettingsData()
                ).err()) Notification::create("Create settings err:\n" + err.value_or("UNK ERROR"), NotificationIcon::Error)->show();
                //create pack json
                auto json = matjson::Value();
                json.set("textureldr", "1.8.1");
                json.set("id", fmt::format("mle.level-pack{}", std::chrono::system_clock::now().time_since_epoch().count()));
                json.set("name", "Level Pack");
                json.set("version", getMod()->getVersion().toVString());
                json.set("author", GameManager::get()->m_playerName.c_str());
                if (auto err = file::writeToJson(pack / "pack.json", json).err()) Notification::create(
                    "Create settings err:\n" + err.value_or("UNK ERROR"), NotificationIcon::Error
                )->show();
                //copy icon
                if (auto err = file::writeBinary(
                    pack / "pack.png", file::readBinary(CCFileUtils::get()->fullPathForFilename(
                        "user95401.main-levels-editor/../../../logo.png", 0
                    ).c_str()).unwrapOrDefault()
                ).err()) Notification::create("Logo copy err:\n" + err.value_or("UNK ERROR"), NotificationIcon::Error)->show();
                //create zip
                auto __zip = file::Zip::create(packs / (string::pathToString(pack.filename()) + ".zip"));
                if (__zip.isErr()) return;
                auto zip = std::move(__zip).unwrap();
                zip.addAllFrom(pack).isOk();
                //remove pack dir
                fs::remove_all(pack, err);
                //show packs
                Ref options = OptionsLayer::create();
                auto item = typeinfo_cast<CCMenuItem*>(options->querySelector(
                    "geode.texture-loader/texture-loader-button"
                ));
                if (item) return item->activate();
#ifdef GEODE_IS_DESKTOP //ios scared me
                options->onVideo(options);
                __this->runAction(CCSequence::create(
                    CallFuncExt::create(
                        [] {
                            auto item = typeinfo_cast<CCMenuItem*>(CCScene::get()->querySelector(
                                "geode.texture-loader/texture-loader-button"
                            ));
                            if (item) item->activate();
                            else log::error("geode.tex.../texture-loader-button = {}", item);
                        }
                    ),
                    CallFuncExt::create(
                        [] {
                            auto item = typeinfo_cast<CCMenuItem*>(CCScene::get()->querySelector(
                                "PackSelectPopup reload-button"
                            ));
                            if (item) item->activate();
                            else log::error("reload-button = {}", item);

                            if (Ref a = CCScene::get()->getChildByType<VideoOptionsLayer>(0)) a->keyBackClicked();
                        }
                    ),
                    nullptr
                ));
#endif
            }
        );
        tp_create->setLayoutOptions(lopts);
		tp_create->setID("tp_create"_spr);
		menu->addChild(tp_create);

        CCMenuItemSpriteExtra* mod_create = CCMenuItemExt::createSpriteExtra(
            btnspr("In modified .geode package"),
            [__this = Ref(this)](CCMenuItem*) {
                auto err = std::error_code();
				namespace fs = std::filesystem;
                auto modid = fmt::format(
                    "mle.custom-package{}", std::chrono::system_clock::now().time_since_epoch().count()
                );
                auto package = getMod()->getPackagePath().parent_path() / fmt::format("{}.geode", modid);
                fs::copy_file(getMod()->getPackagePath(), package, err);
                //unzip
                auto workdir = package.parent_path() / ("_unzip-" + modid);
                auto __unzip = file::Unzip::create(package);
                if (__unzip.isErr()) return;
                auto unzip = std::move(__unzip).unwrap();
                unzip.extractAllTo(workdir).isOk();
                //copy levels
                for (auto id : mle::getListingIDs()) {
                    if (auto impinfo = mle::tryLoadFromFiles(id)->getChildByTag("is-imported-from-file"_h)) {
                        fs::copy_file(impinfo->getID(), workdir / "resources" / fmt::format("{}.level", id), err);
                    }
                };
                //create settings
                fs::create_directories(workdir / "resources" / getMod()->getID(), err);
                if (auto err = file::writeToJson(
                    workdir / "resources" / getMod()->getID() / "settings.json",
                    getMod()->getSavedSettingsData()
                ).err()) Notification::create("Create settings err:\n" + err.value_or("UNK ERROR"), NotificationIcon::Error)->show();
                //rename id in files
                for (auto& path : file::readDirectory(workdir, false).unwrapOrDefault()) {
                    auto str = string::pathToString(path);
                    //just rename bin names
                    fs::rename(str, string::replace(
                        str, getMod()->getID(), modid), err
                    );
                    //rewrite file content
                    if (string::endsWith(str, "mod.json")) {
                        auto read = file::readJson(path).unwrapOrDefault();
                        read.set("id", modid);
                        read.set("name", "\tMLE Custom Package\n");
                        read.set("developer", GameManager::get()->m_playerName.c_str());
                        read.set("tags", matjson::parse(R"([ "offline", "content", "enhancement" ])").unwrapOrDefault());
                        read.set("incompatibilities", matjson::parse(
                            R"([{"id": ")" + getMod()->getID() + R"(", "version": "*", "importance": "conflicting"}])"
                        ).unwrapOrDefault());
                        getMod()->disable().err();
                        for (auto& setting : read["settings"]) {
                            //default
                            setting.set("default", getMod()->getSavedSettingsData().get<matjson::Value>(
                                setting.getKey().value_or("")
                            ).unwrapOrDefault());
							//enable-if
							setting.set("enable-if", modid);
							setting.set("enable-if-description", "You can't change settings in custom package.");
                        }
                        file::writeToJson(path, read).isOk();
                    }
                }
                //create zip
                auto __zip = file::Zip::create(package);
                if (__zip.isErr()) return;
                auto zip = std::move(__zip).unwrap();
                for (auto& path : file::readDirectory(workdir, true).unwrapOrDefault()) {
                    fs::path atzip = string::replace(string::pathToString(path), string::pathToString(workdir / ""), "");
                    zip.add(atzip, file::readBinary(path).unwrapOrDefault()).isOk();
                }
                //remove pack dir
                fs::remove_all(workdir, err);
                //notify
				Notification::create(
					fmt::format(" Created custom package in mods folder:\n {}", string::pathToString(package.filename())),
					NotificationIcon::Success, 5.f
				)->show();
            }
        );
		mod_create->setLayoutOptions(lopts);
        mod_create->setID("mod_create"_spr);
		menu->addChild(mod_create);

        menu->setLayout(ColumnLayout::create()
            ->setAutoScale(true)
            ->setAxisReverse(true)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false)
        );
        limitNodeWidth(menu, this->m_mainLayer->getContentWidth() - 16.f, 1.f, 0.1f);

        return true;
    }
public:
    static MainLevelsEditorMenu* create() {
        auto ret = new MainLevelsEditorMenu();
        if (ret->initAnchored(258.000f, 284.000f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    static auto createButtonForMe() {
        auto item = CCMenuItemExt::createSpriteExtra(createModLogo(getMod()),
            [](CCObject* sender) { create()->show(); }
        );
		item->setID("menu-button"_spr);
		return item;
    }
};


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
#include <cache.hpp>
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
                MLE_LevelsInJSON::get()->insert_or_assign(id, level::jsonFromLevel(level));
                log::debug("Level {} json dump size is {}", id, MLE_LevelsInJSON::get()->at(id).dump().size());
            }
            else log::debug("The .level file for {} was not founded", id);
        }

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
        level->m_levelType = GJLevelType::Local;
        level->m_levelString = dontGetLevelString ? "" : level->m_levelString.c_str();

        return level;
    };

};



/*

LEVEL INTEGRITY VERIFY BYPASS
(and same but not necessary level getting update hook)

*/

#include <Geode/modify/LevelTools.hpp>
class $modify(MLE_LevelTools, LevelTools) {

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
        level->m_levelType = GJLevelType::Local;
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
            auto frameName = fmt::format("diffIcon_{:02d}_btn_001.png", diffID);
            if (CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(frameName.c_str())) {
                auto frame = CCSpriteFrameCache::get()->spriteFrameByName(frameName.c_str());
                if (frame) difficultySprite->setDisplayFrame(frame);
            }
            else {
                auto image = CCSprite::create(frameName.c_str());
                if (image) difficultySprite->setDisplayFrame(image->displayFrame());
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

#include <Geode/modify/EditorUI.hpp>
class $modify(MLE_EditorUI, EditorUI) {
    void showInfoPopup(float) {
        MDPopup::create(
            "Welcome to .level Editor",
            R"(Open default <cg>Level Settings</c> to open tools that help you edit the .level file:

- Meta data editor
- Difficulty sprite selector
- Coins replace tool

<co>`Current level can be saved to .level file only in that tools!`</c>

<cr>`And save buttons in editor pause menu doesn't work!`</c>)", 
"OK"
)->show();
    }
    $override bool init(LevelEditorLayer * editorLayer) {
		if (!EditorUI::init(editorLayer)) return false;
        if (auto impinfo = editorLayer->m_level->getChildByTag("is-imported-from-file"_h)) {
            this->scheduleOnce(schedule_selector(MLE_EditorUI::showInfoPopup), 1.f);
        }
		return true;
    }
    $override void onSettings(cocos2d::CCObject * sender) {
        EditorUI::onSettings(sender);

        //impinfo in level object was created at .level import function
        if (auto impinfo = m_editorLayer->m_level->getChildByTag("is-imported-from-file"_h)) {
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
                    lvl->m_levelType = GJLevelType::Local;
                    play->scheduleOnce(schedule_selector(MLE_GameObjectExt::PlayLayerCustomSetup), 0.f);
                }
            }
        }
        return rtn;
    }
};