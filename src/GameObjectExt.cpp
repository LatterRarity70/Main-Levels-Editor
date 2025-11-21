#include <_Main.hpp>

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