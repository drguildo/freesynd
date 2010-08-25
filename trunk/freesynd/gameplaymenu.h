/************************************************************************
 *                                                                      *
 *  FreeSynd - a remake of the classic Bullfrog game "Syndicate".       *
 *                                                                      *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>              *
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>   *
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>              *
 *                                                                      *
 *    This program is free software;  you can redistribute it and / or  *
 *  modify it  under the  terms of the  GNU General  Public License as  *
 *  published by the Free Software Foundation; either version 2 of the  *
 *  License, or (at your option) any later version.                     *
 *                                                                      *
 *    This program is  distributed in the hope that it will be useful,  *
 *  but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of  *
 *  MERCHANTABILITY  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 *  General Public License for more details.                            *
 *                                                                      *
 *    You can view the GNU  General Public License, online, at the GNU  *
 *  project's  web  site;  see <http://www.gnu.org/licenses/gpl.html>.  *
 *  The full text of the license is also included in the file COPYING.  *
 *                                                                      *
 ************************************************************************/

#ifndef GAMEPLAYMENU_H
#define GAMEPLAYMENU_H

class BriefMenu;
class Mission;

/*!
 * Gameplay Menu class.
 */
class GameplayMenu : public Menu {
public:
    GameplayMenu(MenuManager *m, LoadingMenu *loading, MapMenu *mapMenu);

    bool isSubMenu() { return false; }

    void handleTick(int elapsed);
    void handleShow();
    void handleLeave();
    void handleMouseMotion(int x, int y, int state);
    void handleMouseDown(int x, int y, int button);
    void handleMouseUp(int x, int y, int button);
    void handleUnknownKey(Key key, KeyMod mod, bool pressed);

protected:
    void drawAgentSelectors();
    void drawPerformanceMeters();
    void drawSelectAllButton();
    void drawMissionHint(int elapsed);
    void drawWeaponSelectors();
    void drawMiniMap();
    //! Scroll the map vertically or horizontally.
    bool scroll();
    bool isScrollLegal(int newScrollX, int newScrollY);
    void improveScroll(int &newScrollX, int &newScrollY);
    int selectedAgentsCount();
    void selectAgent(unsigned int agentNo);
    void selectAllAgents();

    bool isAgentSelected(unsigned int agentNo) {
        return (selected_agents_ & (1 << agentNo))!=0;
    }

protected:
    LoadingMenu *loading_;
    MapMenu *map_menu_;
    int tick_count_, last_animate_tick_;
    int last_motion_tick_, last_motion_x_, last_motion_y_;
    int mission_hint_ticks_,mission_hint_;
    Mission *mission_;
    /*! Holds the X coordinate of the screen origin (top left corner) in the world coordinate.*/
    int world_x_;
    /*! Holds the Y coordinate of the screen origin (top left corner) in the world coordinate.*/
    int world_y_;
    /*! Holds the amount of scroll on the X axis.*/
    int scroll_x_;
    /*! Holds the amount of scroll on the Y axis.*/
    int scroll_y_;
    unsigned int selected_agents_;
    unsigned int selectable_agents_;
    bool ctrl_, alt_;
    int pointing_at_ped_, pointing_at_vehicle_, pointing_at_weapon_;
    int mm_tx_, mm_ty_;
    bool completed_;
};

#endif
