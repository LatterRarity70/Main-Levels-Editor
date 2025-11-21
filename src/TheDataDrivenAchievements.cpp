#include <_Main.hpp>

/*

    f u c k

*/

#include <Geode/modify/AchievementManager.hpp>
class $modify(MLE_AchievementManager, AchievementManager) {
    void resortAchievements() {
        //i can't regenerate changed sort at runtime cuz im stupid
        auto createdOrLoadedFileDeleted = false;
        createdOrLoadedFileDeleted = !existsInPaths("achievements-sort.txt") 
            and CCFileUtils::get()->m_fullPathCache.contains("achievements-sort.txt");
        if (createdOrLoadedFileDeleted) return game::restart(true);
        // create(and load) or load sort
        if (!existsInPaths("achievements-sort.txt")) {
            auto list = std::stringstream();
            for (auto dict : CCArrayExt<CCDictionary*>(m_allAchievementsSorted)) {
                /*
                log::debug("{}", dict);
                for (auto [key, value] : CCDictionaryExt<std::string, CCString*>(dict)) {
                    log::debug("{} -> {}", key, value->getCString());
                }
                CCMessageBox("dsa", "asd");
                */
                list << CCDictionaryExt<std::string, CCString*>(
                    dict
                )["identifier"]->getCString() << "\n";
            }
            file::writeStringSafe(
                getMod()->getConfigDir() / "achievements-sort.txt",
                list.str()
            ).isOk();
            resortAchievements();
        }
        else {
            auto list = file::readString(CCFileUtils::get()->fullPathForFilename(
                "achievements-sort.txt", 0
            ).c_str()).unwrapOr("");
            m_allAchievementsSorted->removeAllObjects();
            for (auto identifier : string::split(list, "\n")) {
                auto dict = getAchievementsWithID(identifier.c_str());
                if (dict) m_allAchievementsSorted->addObject(dict);
            }
        }
    }
    $override void addAchievement(
        gd::string identifier, gd::string title,
        gd::string achievedDescription, gd::string unachievedDescription,
        gd::string icon, int limits
    ) {
        if (!THE_DATA_DRIVEN_ACHIEVEMENTS) return AchievementManager::addAchievement(
            identifier, title, achievedDescription, unachievedDescription, icon, limits
        );
        if (this->getUserObject("is-data-file-generating"_spr)) {
            auto val = file::readJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str()).unwrapOr(matjson::Value());

            auto entry = matjson::Value();
            entry["title"] = title.c_str();
            entry["achievedDescription"] = achievedDescription.c_str();
            entry["unachievedDescription"] = unachievedDescription.c_str();
            entry["icon"] = icon.c_str();
            entry["limits"] = limits;

            val[identifier.c_str()] = entry;

            file::writeToJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str(), val);
        }
        else {
            log::debug("addAchievement: {}", identifier.c_str());
            AchievementManager::addAchievement(
                identifier, title,
                achievedDescription, unachievedDescription,
                icon, limits
            );
        };
    }
    $override void addManualAchievements() {
        if (!THE_DATA_DRIVEN_ACHIEVEMENTS) return AchievementManager::addManualAchievements();
        if (!existsInPaths("achievements.json")) { // generate default file
            file::writeStringSafe(getMod()->getConfigDir() / "achievements.json", "{}");
            setUserObject("is-data-file-generating"_spr, new CCObject());
            AchievementManager::addManualAchievements();
            setUserObject("is-data-file-generating"_spr, nullptr);
            addManualAchievements();
        }
        else {
            auto val = file::readJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str()).unwrapOr(matjson::Value());
			for (auto& [identifier, entry] : val) {
                AchievementManager::addAchievement(
                    identifier, entry["title"].asString().unwrapOr("err").c_str(),
                    entry["achievedDescription"].asString().unwrapOr("err").c_str(),
                    entry["unachievedDescription"].asString().unwrapOr("err").c_str(),
                    entry["icon"].asString().unwrapOr("err").c_str(),
                    entry["limits"].asInt().unwrapOr(0)
				);
			};
        }

        queueInMainThread(
            [aw = Ref(this)] {
                if (aw) aw->resortAchievements();
            }
        );

    }
    inline static auto s_Events = matjson::Value();
};

#include <Geode/modify/AchievementsLayer.hpp>
class $modify(AchievementsLayerExt, AchievementsLayer) {
    $override void customSetup() {
        AchievementsLayer::customSetup();
        if (!THE_DATA_DRIVEN_ACHIEVEMENTS) return;
        if (REMOVE_UI) return;
        if (auto menu = m_mainLayer->getChildByType<CCMenu>(0)) {
            auto reload = CCMenuItemExt::createSpriteExtraWithFrameName(
                "GJ_updateBtn_001.png", 1.f, [aw = Ref(this)](void*) {
                    auto manager = AchievementManager::sharedState();
                    if (auto a = (manager->m_allAchievements)) a->removeAllObjects();
                    manager->addManualAchievements();
                    ;;;;;;; aw->hideLayer(true);
                    queueInMainThread(
                        [aw] {
                            AchievementsLayer::create()->showLayer(true);
                        }
                    );
                }
            );
			reload->setID("reload"_spr);
            reload->setPosition(
                8.000f, -menu->getContentHeight() + (320.000 - 266.000)
            );
			menu->addChild(reload, 0, "reload"_h);
        }
    }
};
