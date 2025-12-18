#include <_Main.hpp>

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
                        Ref scroll = __this->m_scrollLayer;
                        //pizdec
                        auto page = scroll->m_page;
                        if (scroll->m_dynamicObjects) {
                            auto pages = scroll->m_dynamicObjects->count();
                            if (page < 0) page += pages;
                            else if (page >= pages) {
                                auto fk = page / pages;
								page -= fk * pages;
                            }
                        };
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

        //pizdec
        if (auto scroll = this->m_scrollLayer) {
            page = scroll->m_page;
            if (scroll->m_dynamicObjects) {
                auto pages = scroll->m_dynamicObjects->count();
                if (page < 0) page += pages;
                else if (page >= pages) {
                    auto fk = page / pages;
                    page -= fk * pages;
                }
            };
        };

        //still not callable even
        auto color = LevelSelectLayer::colorForPage(
            std::max(0, page)
        );

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

    void updateDynamicPage(GJGameLevel* pLevel) { //dynamic page update function is hell
        Ref level = pLevel;
        LevelPage::updateDynamicPage(level);

        //REPLACE_DIFFICULTY_SPRITE really
        if (REPLACE_DIFFICULTY_SPRITE) if (auto difficultySprite = m_difficultySprite) {
            auto diffID = static_cast<int>(level->m_difficulty);

            auto sz = difficultySprite->getContentSize(); //limitNodeSize

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

        //debg
        if (!REMOVE_UI) {
            while (auto a = this->getChildByTag("mle-id-debug"_h)) a->removeFromParent();
            auto text = CCLabelBMFont::create(fmt::format(
                "id: {}, {}"
                , level->m_levelID.value()
                , level::isImported(level)
                ? "was imported (editable)"
                : "not imported (read-only)"
            ).c_str(), "geode.loader/mdFontMono.fnt");
            text->setPosition(this->getContentSize() / 2);
            text->setPositionY(this->getContentHeight() - 46);
            text->setScale(0.6f);
            this->addChild(text, 1, "mle-id-debug"_h);
        }
    }

    $override bool init(GJGameLevel* level)  {
		if (!LevelPage::init(level)) return false;

        if (!REMOVE_UI) if (Ref levelMenu = m_levelMenu) {
            if (auto editLevel = CCMenuItemExt::createSpriteExtraWithFrameName(
                "GJ_editBtn_001.png", 0.35f, [_this = Ref(this)](void*) {
                    if (!_this) return;
                    Ref level = _this->m_level;
                    if (!level) return;
                    if (!level::isImported(level)) return (void)createQuickPopup(
                        "Not editable", 
                        """""Please export level as file first!" 
                        "\n""1. Play this level and pause it"
                        "\n""2. Export using Main Levels Editor Menu"
                        "\n""- - -"
                        "\n""This mod loads levels according to list of IDs, trying to find {id}.level or {id}.json files"
                        , "OK", nullptr, nullptr
                    );
                    switchToScene(LevelEditorLayer::create(level, false));
                }
            )) {
                editLevel->setID("editLevel"_spr);
                levelMenu->addChild(editLevel);

                auto view = CCDirector::get()->getVisibleSize();
                auto viewCenter = view / 2;
                auto worldPos = viewCenter + CCPointMake((view.width / 2) - 68, 90);
                editLevel->setPosition(levelMenu->convertToNodeSpace(worldPos));
            };
            if (auto deleteLevel = CCMenuItemExt::createSpriteExtraWithFrameName(
                "GJ_resetBtn_001.png", 0.9f, [_this = Ref(this)](void*) {
                    if (!_this) return;
                    Ref level = _this->m_level;
                    if (!level) return;
                    createQuickPopup(
                        "Remove this level?",
                        "This will change level listing config and file of level (if exists)", 
                        "No", "Yes", [_this, level](void*, bool Yes) {
							if (!Yes) return;
							if (!level) return;
                            auto page = 0;
                            auto gotPage = false;
                            std::vector<int> list;
                            for (auto p : MLE::getLevelIDs()) {
                                if (p == level->m_levelID) {
									gotPage = true;
                                    continue;
                                }
                                list.push_back(p);
                                if (!gotPage) page++;
							}
                            MLE::updateListingIDs(list);
                            if (auto import = level::isImported(level)) {
                                auto err = std::error_code();
                                std::filesystem::remove_all(import->getID(), err);
                            };
                            if (Ref a = CCScene::get()->getChildByType<LevelSelectLayer>(0)) {
                                a->removeFromParent();
                                MLE_LevelSelectExt::ForceNextTo = page;
                                CCScene::get()->addChild(LevelSelectLayer::create(page));
                            }
                        }
                    );
                }
            )) {
                deleteLevel->setID("deleteLevel"_spr);
                levelMenu->addChild(deleteLevel);
                auto view = CCDirector::get()->getVisibleSize();
                auto viewCenter = view / 2;
                auto worldPos = viewCenter + CCPointMake((view.width / 2) - 68, 60);
                deleteLevel->setPosition(levelMenu->convertToNodeSpace(worldPos));
            };
        }

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
    void showInfoPopup(float = 0.f) {
        auto pop = MDPopup::create(
            isJSON ?
            "Welcome to level editor for .json file" :
            "Welcome to level editor for .level file",
            R"(Open default <cg>Level Settings</c> to open tools that help you edit level file:
- Meta data editor
- Difficulty sprite selector
- Coins replace tool)", "OK");
        pop->m_scene = (this);
        pop->show();
    }
    $override bool init(LevelEditorLayer * editorLayer) {
		if (!EditorUI::init(editorLayer)) return false;
        if (auto impinfo = level::isImported(editorLayer->m_level)) {
            isJSON = string::endsWith(impinfo->getID(), ".json");
            queueInMainThread([a = Ref(this)] { if (a) a->showInfoPopup(); });
        }
		return true;
    }
    void openSetupWindows(float = 0.f) {
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

                    Ref preview = CCSprite::create();
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
    $override void onSettings(cocos2d::CCObject* sender) {
        EditorUI::onSettings(sender);
        scheduleOnce(schedule_selector(MLE_EditorUI::openSetupWindows), 0.f);
    }
};

/*

Temp Setup UI for users

*/

#include <Geode/modify/PauseLayer.hpp>
class $modify(MLE_PauseExt, PauseLayer) {
    $override void customSetup() {
        PauseLayer::customSetup();

        if (!REMOVE_UI) queueInMainThread(
            [aw = Ref(this)] {
                if (!aw) return;
                if (auto menu = aw->querySelector("right-button-menu")) {
                    if (auto editLevel = CCMenuItemExt::createSpriteExtraWithFrameName(
                        "GJ_editBtn_001.png", 0.35f, [aw](void*) {
                            Ref level = GJBaseGameLayer::get()->m_level;
                            if (!level) return;
                            if (!level::isImported(level)) return (void)createQuickPopup(
                                "Not editable",
                                """""Please export level as file first using Main Levels Editor Menu!"
                                "\n""This mod loads levels according to list of IDs, trying to find {id}.level or {id}.json files"
                                , "OK", nullptr, nullptr
                            ); 
                            aw->goEdit();
                        }
                    )) {
                        editLevel->setID("editLevel"_spr);
                        menu->addChild(editLevel);
                    };
                    if (auto image = geode::createModLogo(getMod())) {
                        CCNode* ref = menu->getChildByType<CCMenuItem>(0);
                        if (!ref) ref = menu;
                        limitNodeWidth(image, ref->getContentWidth(), 99.f, 0.1f);
                        auto item = CCMenuItemExt::createSpriteExtra(
                            image, [](void*) {
                                MainLevelsEditorMenu::create()->show();
                            }
                        );
                        item->setID("menu-button"_spr);
                        menu->addChild(item);
                    }
                    menu->updateLayout();
                };
            }
        );
    };
};