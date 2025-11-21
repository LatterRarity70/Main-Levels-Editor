#pragma once
#include <Geode/modify/LocalLevelManager.hpp>

class MLE_LevelsInJSON : public cocos2d::CCObject, public std::unordered_map<int, matjson::Value> {
    virtual ~MLE_LevelsInJSON() = default;
    static auto create() {
        auto ret = new MLE_LevelsInJSON();
        ret->autorelease();
        return ret;
    }
public:
    static auto get() {
        auto asd = geode::cast::typeinfo_cast<MLE_LevelsInJSON*>(LocalLevelManager::get()->getUserObject("MLE_LevelsInJSON"));
        if (!asd) {
			asd = create();
			LocalLevelManager::get()->setUserObject("MLE_LevelsInJSON", asd);
        }
		return asd;
    }
};