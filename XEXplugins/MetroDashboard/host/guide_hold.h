#pragma once
#include <cstdint>
// One event per uninterrupted hold by one controller. Release/disconnect or
// leaving the application cancels it; dismissing the menu doesn't re-arm it.
struct MetroGuideHold {
    int slot=-1;
    std::int64_t since=0;
    bool fired=false;
    bool update(int downSlot, bool ownsFocus, std::int64_t now) {
        if(!ownsFocus || downSlot<0) { slot=-1; fired=false; return false; }
        if(slot!=downSlot) { slot=downSlot; since=now; fired=false; return false; }
        if(!fired && now-since>=2500) { fired=true; return true; }
        return false;
    }
};
