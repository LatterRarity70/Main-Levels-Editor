#include <_Main.hpp>

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
                for (auto id : MLE::getListingIDs()) unk3->addObject(
                    GameLevelManager::get()->getMainLevel(id, 0)
                );
                if (!unk3->count()) {
					unk3->addObject(GameLevelManager::get()->getMainLevel(-1, 0));
                    if (pages) for (auto page : CCArrayExt<CCNode>(pages)) page->setScale(0.f);
                }
            };
        }
        return BoomScrollLayer::create(pages, unk1, unk2, unk3, delegate);
    }
};

//__int64 __fastcall CustomListView::create(int a1, int a2, __int64 a3, float a4)
#include <Geode/modify/BoomListView.hpp>
class $modify(MLE_BoomListViewExt, BoomListView) {
    void trySetupSongs(CCArray* entries) {
        if (!entries) return; // log::error("entries is null");
        if (!entries->count()) return; //log::error("entries count is 0");

        auto test = typeinfo_cast<SongObject*>(entries->objectAtIndex(0));
        if (!test) return; // log::error("entry1 is not SongObject");

        entries->removeAllObjects();

        for (auto id : MLE::getAudioIDs()) {
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
