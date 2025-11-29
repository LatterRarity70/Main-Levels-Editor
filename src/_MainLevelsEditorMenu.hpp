#pragma once

class MainLevelsEditorMenu : public geode::Popup<> {
protected:
    using FileEvent = Task<Result<std::filesystem::path>>;
    EventListener<FileEvent> m_listener;
    static auto resolveListEntry(Ref<TextInput> textinput, Ref<GJGameLevel> level) {
        // parse input
        std::string input = textinput->getString();
        bool add_new = input.empty();
        bool add_front = input == "-1";
        auto target_id = utils::numFromString<int>(
            add_front ? "" : input
        ).unwrapOr(
            level->m_levelID.value()
        );

        auto list = MLE::getListingIDs();

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
            if (tarp != list.end()) {
                list.erase(tarp);
                list.insert(tarp, target_id);
            }
        }

        MLE::updateListingIDs(list);

        return target_id;
    }
    void addAudioSetupMenu(auto artists) {
        this->removeChildByID("audio-setup-menu"_spr);

        Ref btnsMenu = m_mainLayer->getChildByType<CCMenu>(0);
        if (!btnsMenu) return;
        btnsMenu->setVisible(false);

        Ref menu = CCMenu::create();
        if (menu) {
            menu->setTag(0);
            menu->setID("audio-setup-menu"_spr);
            menu->ignoreAnchorPointForPosition(0);
            menu->setContentSize(btnsMenu->getContentSize() * 0.82f);
            this->m_mainLayer->addChildAtPosition(menu, Anchor::Center, { 0.f, 0.f });
        }

        if (auto bg = CCScale9Sprite::create("GJ_square06.png")) {
            bg->setContentSize(menu->getContentSize());
            bg->setAnchorPoint({ 0.0f, 0.0f });
            menu->addChild(bg);
        };

        Ref idLabel = CCLabelBMFont::create("ID: 0", "bigFont.fnt");
        if (idLabel) {
            idLabel->setScale(0.45f);
            menu->addChildAtPosition(idLabel, Anchor::Bottom, { 0.f, 16.000f }, !"nopls");
        }

        if (auto close = CCLabelBMFont::create("X", "bigFont.fnt")) {
            auto item = CCMenuItemExt::createSpriteExtra(
                close, [menu, btnsMenu](CCObject* sender) {
                    menu->removeFromParent();
                    btnsMenu->setVisible(btnsMenu);
                }
            );
            menu->addChildAtPosition(item, Anchor::TopRight, { -4.f, -4.f }, !"nopls");
        }

        typedef LevelTools LT;
        auto id = [menu] { return menu->getTag(); };

        auto inputUpdateFuncs = std::vector<std::function<void()>>();

        auto addInput = [id, menu, &inputUpdateFuncs](
            const char* a, float pos, const char* valname, 
            std::function<matjson::Value()> def, int type = "audio"_h
            ) mutable {
                Ref label = CCLabelBMFont::create(a, "bigFont.fnt");
                Ref input = TextInput::create(
                    menu->getContentWidth() - 32.f, label->getString()
                );
                input->setCallback([id, type, valname](std::string str)
                    {
                        if (!MLE_LevelsInJSON::get()->contains(type)) {
                            MLE_LevelsInJSON::get()->insert_or_assign(type, matjson::Value());
                        }
                        auto& content = MLE_LevelsInJSON::get()->at(type);
                        auto parse = matjson::parse(str);
                        if (parse.isOk()) content[fmt::format("{}", id())].set(
                            valname, parse.unwrapOr("")
                        ); else content[fmt::format("{}", id())].erase(valname);
                        auto toWrite = matjson::Value(content);
                        toWrite.erase("file");
                        auto path = content["file"].asString().unwrapOr("");
                        file::createDirectoryAll(std::filesystem::path(path).parent_path());
                        file::writeString(path, toWrite.dump());
                    }
                );
                input->setScale(0.725f);
                input->addChild(label);
                label->setScale(0.65f);
                label->setPositionX(input->getContentWidth() * 0.5f);
                label->setPositionY(input->getContentHeight() + label->getContentHeight() * 0.35f);
                menu->addChildAtPosition(input, Anchor::Bottom, { 0.f, pos }, !"nopls");
                inputUpdateFuncs.push_back([input, def, valname, type]()
                    { input->setString(def().dump().c_str()); });
            };
        if (artists) {
            addInput("Name", 174.000f, "name", [id] -> matjson::Value {
                return LT::nameForArtist(id()).c_str(); }, "artists"_h
            );
            addInput("Newgrounds", 132.000f, "ng", [id] -> matjson::Value {
                return LT::ngURLForArtist(id()).c_str(); }, "artists"_h
            );
            addInput("YouTube", 90.000f, "yt", [id] -> matjson::Value {
                return LT::ytURLForArtist(id()).c_str(); }, "artists"_h
            );
            addInput("Facebook", 46.000f, "fb", [id] -> matjson::Value {
                return LT::fbURLForArtist(id()).c_str(); }, "artists"_h
            );
        }
        else {
            addInput("Track Filename", 174.000f, "file", [id] -> matjson::Value { 
                return LT::getAudioFileName(id()).c_str(); }
            );
            addInput("Track Title", 132.000f, "title", [id] -> matjson::Value { 
                return LT::getAudioTitle(id()).c_str(); }
            );
            addInput("Track URL", 90.000f, "url", [id] -> matjson::Value { 
                return LT::urlForAudio(id()).c_str(); }
            );
            addInput("Artist ID", 46.000f, "artist", [id] -> matjson::Value { 
                return LT::artistForAudio(id()); }
            );
        }
        for (auto f : inputUpdateFuncs) f();

        auto updID = [menu, idLabel, inputUpdateFuncs](CCObject* sender)
            {
                menu->setTag(menu->getTag() + sender->getTag());
                idLabel->setString(fmt::format("ID: {}", menu->getTag()).c_str());
                for (auto f : inputUpdateFuncs) f();
            };

        if (auto a = CCLabelBMFont::create(">", "bigFont.fnt")) {
            auto item = CCMenuItemExt::createSpriteExtra(a, updID);
            item->setTag(1);
            menu->addChildAtPosition(item, Anchor::Right, { -4.f, -0.f }, !"nopls");
        }
        if (auto a = CCLabelBMFont::create("<", "bigFont.fnt")) {
            auto item = CCMenuItemExt::createSpriteExtra(a, updID);
            item->setTag(-1);
            menu->addChildAtPosition(item, Anchor::Left, { 4.f, -0.f }, !"nopls");
        }
    }
    bool setup() override {
        this->setTitle("");

        //menu
        auto menu = CCMenu::create();
        menu->setContentWidth(260.000f);
        menu->setContentHeight(262.000f);
        this->m_mainLayer->addChildAtPosition(menu, Anchor::Center, { 0.f, 0.f });

        //layout options for buttons
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

        //title
        menu->addChild(titlespr("Main Levels Editor"), 0, "presist"_h);

        //settings
        CCMenuItemSpriteExtra* settings = CCMenuItemExt::createSpriteExtra(
            btnspr("Open Settings"), [__this = Ref(this)](auto) {
                openSettingsPopup(getMod(), 1);
            }
        );
        if (menu and settings) {
            settings->setLayoutOptions(lopts);
            settings->setID("settings"_spr);
            menu->addChild(settings, 0, "presist"_h);
        };

        //reload_levels_cache
        CCMenuItemSpriteExtra* reload_levels_cache = CCMenuItemExt::createSpriteExtra(
            btnspr("Reload levels cache!"), [__this = Ref(this)](auto) {
                LocalLevelManager::get()->init();
                if (auto s = CCScene::get()) if (auto l = s->getChildByType<LevelSelectLayer>(
                    0 // LevelSelectLayer
                )) l->keyBackClicked();
                Notification::create("local level manager was reinitialized", NotificationIcon::Info)->show();
            }
        );
        if (menu and reload_levels_cache) {
            reload_levels_cache->setLayoutOptions(lopts);
            reload_levels_cache->setID("reload_levels_cache"_spr);
            menu->addChild(reload_levels_cache, 0, "presist"_h);
        }
        static Ref shReload = reload_levels_cache;
        shReload = reload_levels_cache;

        //padding
        menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6), 0, "presist"_h);

        if ("page1") {
            //title
            menu->addChild(titlespr("Shared Level Files"));

            //load_level
            CCMenuItemSpriteExtra* load_level = CCMenuItemExt::createSpriteExtra(
                btnspr("Open .level file"), [__this = Ref(this)](auto) {
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
            if (menu and load_level) {
                load_level->setLayoutOptions(lopts);
                load_level->setID("load_level"_spr);
                menu->addChild(load_level);
            }

            //edit_level
            CCMenuItemSpriteExtra* edit_level = CCMenuItemExt::createSpriteExtra(
                btnspr("Edit .level file"), [__this = Ref(this)](auto) {
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
            if (menu and edit_level) {
                edit_level->setLayoutOptions(lopts);
                edit_level->setID("edit_level"_spr);
                menu->addChild(edit_level);
            }

            //padding
            menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6));

            //title
            menu->addChild(titlespr("Act. With Current Level"));

            //padding
            menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 12));

            //id_input
            auto id_input = TextInput::create(350.000f, {
                "        Level ID to Insert At:\n"
                "Leave blank to add as new level\n"
                "or -1 to add as new in front of list\n "
                });
            if (menu and id_input) {
                id_input->setLayoutOptions(lopts);
                id_input->setFilter("0123456789-");
                id_input->setID("level_id_input"_spr);
                menu->addChild(id_input);
            }

            //insert
            CCMenuItemSpriteExtra* insert_to_level_list = CCMenuItemExt::createSpriteExtra(
                btnspr("Insert to Level List"), [id_input](void*) {
                    if (!GameManager::get()->getGameLayer()) {
                        return Notification::create("You are not in a level", NotificationIcon::Error)->show();
                    }

                    auto level = GameManager::get()->getGameLayer()->m_level;

                    // parse input
                    auto target_id = resolveListEntry(id_input, level);

                    // export
                    auto export_result = level::exportLevelFile(
                        level, getMod()->getConfigDir() / fmt::format("{}.level", target_id)
                    );
                    if (!export_result) return Notification::create(
                        "Failed to export level\n" + export_result.err().value_or("UNK ERROR")
                        , NotificationIcon::Error)->show();

                    Notification::create("Level inserted to list!", NotificationIcon::Success)->show();

                    shReload->activate();
                }
            );
            if (menu and insert_to_level_list) {
                insert_to_level_list->setLayoutOptions(lopts);
                insert_to_level_list->setID("insert_to_level_list"_spr);
                menu->addChild(insert_to_level_list);
            }

            //export
            CCMenuItemSpriteExtra* export_level = CCMenuItemExt::createSpriteExtra(
                btnspr("Export into .level file"), [__this = Ref(this)](void*) {
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
            if (menu and export_level) {
                export_level->setLayoutOptions(lopts);
                export_level->setID("export_level"_spr);
                menu->addChild(export_level);
            }

            //pad
            menu->addChild(CCLayerColor::create({ 0,0,0,0 }, 12, 6));

            //tt
            menu->addChild(titlespr("Create Shared Levels Pack..."));

            //tp
            CCMenuItemSpriteExtra* tp_create = CCMenuItemExt::createSpriteExtra(
                btnspr("In resource pack (TP)"), [__this = Ref(this)](void*) -> void {
                    auto err = std::error_code();
                    namespace fs = std::filesystem;
                    auto packs = getMod()->getConfigDir().parent_path() / "geode.texture-loader" / "packs";
                    auto pack = packs / fmt::format(
                        "level pack by {} #{}", GameManager::get()->m_playerName.c_str(),
                        std::chrono::system_clock::now().time_since_epoch().count()
                    );
                    fs::create_directories(pack, err);
                    //copy levels
                    for (auto id : MLE::getListingIDs()) {
                        if (auto impinfo = level::isImported(MLE::tryLoadFromFiles(id))) {
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
            if (menu and tp_create) {
                tp_create->setLayoutOptions(lopts);
                tp_create->setID("tp_create"_spr);
                menu->addChild(tp_create);
            }

            CCMenuItemSpriteExtra* mod_create = CCMenuItemExt::createSpriteExtra(
                btnspr("In modified .geode package"), [__this = Ref(this)](void*) {
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
                    for (auto id : MLE::getListingIDs()) {
                        if (auto impinfo = level::isImported(MLE::tryLoadFromFiles(id))) {
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
                            read.set("developers", matjson::parse(fmt::format(
                                "[ \"{}\", \"{}\" ]", getMod()->getDevelopers()[0].c_str(),
                                GameManager::get()->m_playerName.c_str()
                            )).unwrapOrDefault());
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
            if (menu and mod_create) {
                mod_create->setLayoutOptions(lopts);
                mod_create->setID("mod_create"_spr);
                menu->addChild(mod_create);
            }
        };

        //page2toggle at m_buttonMenu
        auto page2toggle = CCMenuItemExt::createToggler(
            titlespr("More (feat. v7)"),
            btnspr("More (feat. v7)"),
            [menu = Ref(menu)](CCMenuItem*) {
                for (auto node : menu->getChildrenExt()) {
                    if (!node) continue;
                    if (node->getTag() == "presist"_h) continue;
                    node->setVisible(!node->isVisible());
                }
                menu->updateLayout();
            }
        );
        if (m_buttonMenu and page2toggle) {
            page2toggle->setScale(0.550f);
            page2toggle->setID("page-2"_spr);
            m_buttonMenu->addChildAtPosition(page2toggle, Anchor::Bottom, { 0.f, 0.f });
        }

        if ("page2") {
            //title
            if (Ref title = titlespr("Local tracks")) {
                title->setVisible(false);
                menu->addChild(title);
            };

            //edit
            CCMenuItemSpriteExtra* edit_audio = CCMenuItemExt::createSpriteExtra(
                btnspr("Edit tracks"), [__this = Ref(this)](auto) {
                    __this->addAudioSetupMenu(not "for artists");
                }
            );
            if (menu and edit_audio) {
                edit_audio->setLayoutOptions(lopts);
                edit_audio->setID("edit_audio"_spr);
                edit_audio->setVisible(false);
                menu->addChild(edit_audio);
            }

            //edit_artists
            CCMenuItemSpriteExtra* edit_artists = CCMenuItemExt::createSpriteExtra(
                btnspr("Edit artists"), [__this = Ref(this)](auto) {
                    __this->addAudioSetupMenu("for artists");
                }
            );
            if (menu and edit_artists) {
                edit_artists->setLayoutOptions(lopts);
                edit_artists->setID("edit_artists"_spr);
                edit_artists->setVisible(false);
                menu->addChild(edit_artists);
            }

            //open_songs
            CCMenuItemSpriteExtra* open_songs = CCMenuItemExt::createSpriteExtra(
                btnspr("View soundtracks"), [__this = Ref(this)](auto) {
                    Ref a = LevelSelectLayer::create(0);
                    __this->getParent()->addChild(a);
                    __this->setZOrder(11);
                    a->setVisible(false);
                    a->onDownload(a);
                    queueInMainThread([a] { a->removeFromParent(); });
                }
            );
            if (menu and open_songs) {
                open_songs->setLayoutOptions(lopts);
                open_songs->setID("open_songs"_spr);
                open_songs->setVisible(false);
                menu->addChild(open_songs);
            }

            //pad
            if (Ref pad = CCLayerColor::create({ 0,0,0,0 }, 12, 6)) {
                pad->setVisible(false);
                menu->addChild(pad);
            };

            //title
            if (Ref title = titlespr("Act. With Current Level")) {
                title->setVisible(false);
                menu->addChild(title);
            };

            //exportAs
            if ("exportAsBtn") {
                Ref textinput = TextInput::create(52.f, "ID");
                if (textinput) {
                    textinput->setID("input.exportAs"_spr);
                    textinput->setFilter("0123456789-");
                    textinput->setMaxCharCount(6);
                    /*if (auto game = GameManager::get()->m_gameLayer) {
                        if (auto level = game->m_level)
                            input->
                    }*/
                }
                CCMenuItemSpriteExtra* exportAs = CCMenuItemExt::createSpriteExtra(
                    btnspr("Export as .json"), [textinput](auto) {
                        if (!GameManager::get()->getGameLayer()) return Notification::create(
                            "You are not in a level", NotificationIcon::Error
                        )->show();

                        auto level = GameManager::get()->getGameLayer()->m_level;

                        auto target_id = resolveListEntry(textinput, level);

                        // makin data dump like that can save you from matjson errors related to random memory
                        auto lvlJSON = level::jsonFromLevel(level);
                        std::stringstream data;
                        data << "{" << std::endl;
                        for (auto& [k, v] : std::move(lvlJSON)) {
                            data << "\t\"" << k << "\": " << matjson::parse(v.dump()).unwrapOr(
                                "invalid value (dump failed)"
                            ).dump(0) << "," << std::endl;
                        }
                        data << "\t" R"("there is": "end!")" << std::endl;
                        data << "}";

                        auto expPath = getMod()->getConfigDir() / "levels";
                        expPath = expPath / fmt::format("{}.json", target_id);

                        auto errcd = std::error_code();
                        std::filesystem::create_directories(expPath.parent_path(), errcd);

                        // export
                        if (auto err = level::exportLevelFile(level, expPath).err())
                            return Notification::create(
                                "Failed to export level\n" + err.value_or("UNK ERROR")
                                , NotificationIcon::Error)->show();

                        Notification::create(
                            "Level inserted to list!", NotificationIcon::Success
                        )->show();

                        shReload->activate();
                    }
                );
                if (menu and exportAs) {
                    exportAs->setLayoutOptions(lopts);
                    exportAs->setID("exportAs"_spr);
                    exportAs->setVisible(false);
                    if (auto inp = textinput; auto btn = exportAs) {
                        btn->addChild(inp);
                        inp->setPosition(btn->getContentSize() / 2.f);
                        inp->setPositionX(
                            (btn->getContentWidth()) +
                            (3.f) + (inp->getContentWidth() / 2.f)
                        );
                        inp->getBGSprite()->setContentHeight(btn->getContentHeight() * 2);
                        inp->getInputNode()->m_cursor->setFntFile("geode.loader/mdFont.fnt");
                        inp->getInputNode()->m_cursor->setString(
                            "                                   "
                            "                                                     "
                            "blank for new at end of list\n"
                            "                                   "
                            "                                                     "
                            "       -1 for new at front of list\n"
                        );
                    }
                    menu->addChild(exportAs);
                }
            };

            //last pad
            if (Ref pad = CCLayerColor::create({ 0,0,0,0 }, 12, 12)) {
                pad->setVisible(false);
                menu->addChild(pad);
            };

            //v7featinf
            CCMenuItemSpriteExtra* v7featinf = CCMenuItemExt::createSpriteExtra(
                SimpleTextArea::create(fmt::format(
                    "* Theese tools will generate stuff at config dir..."
                    , getMod()->getID())),
                [__this = Ref(this)](auto) {
                    for (auto p : {
                        getMod()->getConfigDir() / getMod()->getID(),
                        getMod()->getConfigDir() / "levels",
                        }) {
                        auto errcd = std::error_code();
                        std::filesystem::create_directories(p, errcd);
                        file::openFolder(p);
                    }
                }
            );
            if (menu and v7featinf) {
                v7featinf->setLayoutOptions(lopts);
                v7featinf->setID("v7featinf"_spr);
                v7featinf->setVisible(false);
                menu->addChild(v7featinf);
            }
        }

        //layout
        menu->setLayout(ColumnLayout::create()
            ->setAutoScale(true)
            ->setAxisReverse(true)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false)
        );
        menu->getLayout()->ignoreInvisibleChildren(true); //lol
        menu->updateLayout(); //xd ^^^
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