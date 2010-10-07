/************************************************************************
 *                                                                      *
 *  FreeSynd - a remake of the classic Bullfrog game "Syndicate".       *
 *                                                                      *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>              *
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>   *
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>              *
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>    *
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net> *
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "screen.h"
#include "app.h"

Mission::Mission(): mtsurfaces_(NULL), mdpoints_(NULL), mdpoints_cp_(NULL),
map_(0), min_x_(0), min_y_(0), max_x_(0), max_y_(0), objective_(0),
objective_ped_(-1), objective_vehicle_(-1)
{
    memset(&level_data_, 0, sizeof(level_data_));
}

Mission::~Mission()
{
    for (unsigned int i = 0; i < vehicles_.size(); i++)
        delete vehicles_[i];
    for (unsigned int i = 0; i < peds_.size(); i++)
        delete peds_[i];
    for (unsigned int i = 0; i < weapons_.size(); i++)
        delete weapons_[i];
    clrSurfaces();
}

#define copydata(x, y) memcpy(&level_data_.x, levelData + y, sizeof(level_data_.x))

bool Mission::loadLevel(uint8 * levelData)
{
    copydata(u01, 0);
    copydata(map, 6);
    copydata(offset_ref, 32774);
    copydata(people, 32776);
    copydata(cars, 56328);
    copydata(statics, 59016);
    copydata(weapons, 71016);
    copydata(sfx, 89448);
    copydata(scenarios, 97128);
    copydata(u09, 113512);
    copydata(mapinfos, 113960);
    copydata(objectives, 113974);
    copydata(u11, 114114);

    map_ = READ_LE_UINT16(level_data_.mapinfos.map);
    printf("map to load %X\n", map_);
    min_x_ = READ_LE_UINT16(level_data_.mapinfos.min_x) / 2;
    min_y_ = READ_LE_UINT16(level_data_.mapinfos.min_y) / 2;
    max_x_ = READ_LE_UINT16(level_data_.mapinfos.max_x) / 2;
    max_y_ = READ_LE_UINT16(level_data_.mapinfos.max_y) / 2;
    objective_ = 2;
/* objective data not 10, 7 or less
    objective_ =
        level_data_.u10.objective[0] | (level_data_.u10.objective[1] << 8);
    int objective_data =
        level_data_.u10.objective_data[0] | (level_data_.u10.
                                             objective_data[1] << 8);
                                             */

    // 1  = persuade
    // 2  = assassinate
    // 3  = protect
    // 5  = equipment aquisition
    // 10 = combat sweep (police)
    // 11 = combat sweep
    // 14 = raid and rescue
    // 15 = use vehicle
    //printf("%i %i %x\n", objective_, objective_data, objective_data);

    //if (objective_ == 1 || objective_ == 2 || objective_ == 3)
        //objective_ped_ = (objective_data - 2) / 0x5c;

    //if (objective_ == 14 || objective_ == 15)
    //    objective_vehicle_ = ((objective_data & 0xff) - 2) / 0x2a;

    vehicles_.clear();

    uint16 vindx[64];
    uint16 pindx[256];
    uint16 driverindx[256];
    memset(vindx, 0xFF, 2*64);
    memset(pindx, 0xFF, 2*256);
    memset(driverindx, 0xFF, 2*256);

#if 0
    // for hacking vehicles data
    char nameSv[256];
    sprintf(nameSv, "vehicles%02X.hex", map_);
    FILE *staticsFv = fopen(nameSv,"wb");
    if (staticsFv) {
        fwrite(level_data_.cars, 1, 42*64, staticsFv);
        fclose(staticsFv);
    }

#endif
    uint16 mindx = 0;
    for (int i = 0; i < 64; i++) {
        LEVELDATA_CARS & car = level_data_.cars[i];
        // car.sub_type 0x09 - train
        if (car.type == 0x0)
            continue;
        VehicleInstance *v =
            g_App.vehicles().loadInstance((uint8 *) & car, map_);
        if (v) {
            vehicles_.push_back(v);
            vindx[i] = mindx;
            if (car.offset_of_driver != 0) {
                driverindx[(car.offset_of_driver - 2) / 92] = mindx;
            }
            mindx++;
        }
    }

    peds_.clear();

#if 0
    // for hacking peds data
    char nameSp[256];
    sprintf(nameSp, "peds%02X.hex", map_);
    FILE *staticsFp = fopen(nameSp,"wb");
    if (staticsFp) {
        fwrite(level_data_.people, 1, 256*92, staticsFp);
        fclose(staticsFp);
    }
#endif

    mindx = 0;
    for (int i = 0; i < 256; i++) {
        LEVELDATA_PEOPLE & pedref = level_data_.people[i];
        if(pedref.type == 0x0 || pedref.desc == 0x0D || pedref.desc == 0x0C)
            continue;
        if(pedref.desc != 0x04 && pedref.desc != 0x05) {
            if(pedref.type !=0) //TODO: pedref.desc == 0 exist?
                printf("unknown property %i\n", pedref.desc);
        }
        PedInstance *p =
            g_App.peds().loadInstance((uint8 *) & pedref, map_);
        if (p) {
            if (pedref.desc == 0x05) {
                if (driverindx[i] != 0xFFFF) {
                    p->putInVehicle(vehicles_[driverindx[i]]);
                    p->setMap(-1);
                    vehicles_[driverindx[i]]->forceSetDriver(p);
                } else {
                    uint16 vin = READ_LE_UINT16(pedref.offset_of_vehicle);
                    if (vin != 0) {
                        vin = (vin - 0x5C02) / 42; // 42 vehicle data size
                        vin = vindx[vin];
                        if (vin != 0xFFFF) {
                            p->putInVehicle(vehicles_[vin]);
                            p->setMap(-1);
                            vehicles_[vin]->setDriver(p);
                        }
                    }
                }
            }
            peds_.push_back(p);
            if (i > 7) {
                //p->setHostile(true);
                p->setSightRange(7);
            }
            pindx[i] = mindx;
            mindx++;
        }
    }
#if 0
    // for hacking statics data
    char nameSs[256];
    sprintf(nameSs, "statics%02X.hex", map_);
    FILE *staticsFs = fopen(nameSs,"wb");
    if (staticsFs) {
        fwrite(level_data_.statics, 1, 12000, staticsFs);
        fclose(staticsFs);
    }
#endif

    statics_.clear();
    for (unsigned int i = 0; i < 400; i++) {
        LEVELDATA_STATICS & sref = level_data_.statics[i];
        if(sref.desc == 0)
            continue;
        Static *s = Static::loadInstance((uint8 *) & sref, map_);
        if (s)
            statics_.push_back(s);
    }

#if 0
    // for hacking weapons data
    char nameSw[256];
    sprintf(nameSw, "weapons%02X.hex", map_);
    FILE *staticsFw = fopen(nameSw,"wb");
    if (staticsFw) {
        fwrite(level_data_.weapons, 1, 36*512, staticsFw);
        fclose(staticsFw);
    }
#endif
    weapons_.clear();
    for (unsigned int i = 0; i < 512; i++) {
        LEVELDATA_WEAPONS & wref = level_data_.weapons[i];
        if(wref.desc == 0)
            continue;
        WeaponInstance *w = g_App.weapons().loadInstance((uint8 *) & wref, map_);
        if (w) {
            if (wref.desc == 0x05) {
                uint16 offset_owner = READ_LE_UINT16(wref.offset_owner);
                if (offset_owner != 0) {
                    offset_owner = (offset_owner - 2) / 92; // 92 = ped data size
                    if (offset_owner > 7 && pindx[offset_owner] != 0xFFFF) {
                        // TODO: correct weapons for enemy agents
                        peds_[pindx[offset_owner]]->addWeapon(w);
                        weapons_.push_back(w);
                    } else {
                        delete w;
                    }
                } else {
                    delete w;
                }
            } else {
                w->setMap(map_);
                weapons_.push_back(w);
            }
        }
    }
    return true;
}

bool Mission::loadMission(uint8 * missData, int size)
{
    char *miss = (char *) missData;
    miss[size - 1] = 0;

    info_costs_[0] = info_costs_[1] = info_costs_[2] = 0;
    enhance_costs_[0] = enhance_costs_[1] = enhance_costs_[2] = 0;

    int i = 0;

    while (*miss != '|') {
        info_costs_[i++] = atoi(miss);
        miss = strchr(miss, '\n') + 1;
    }

    miss += 2;
    i = 0;

    while (*miss != '|') {
        enhance_costs_[i++] = atoi(miss);
        miss = strchr(miss, '\n') + 1;
    }

    miss += 2;

    briefing_ = "";
    if (miss) {
        briefing_ = miss;
    }
    return true;
}

bool Mission::loadMap()
{
    return g_App.maps().loadMap(map_);
}

int Mission::mapWidth()
{
    return g_App.maps().mapWidth(map_);
}

int Mission::mapHeight()
{
    return g_App.maps().mapHeight(map_);
}

int Mission::startX()
{
    int x =
        g_App.maps().tileToScreenX(map_, peds_[0]->tileX(),
        peds_[0]->tileY(), mmax_z_ - 1, 0,
                                   0);
    x -= (GAME_SCREEN_WIDTH - 129) / 2;
    if (x < 0)
        x = 0;
    return x;
}

int Mission::startY()
{
    int y =
        g_App.maps().tileToScreenY(map_, peds_[0]->tileX(),
                                   peds_[0]->tileY(), mmax_z_ - 1, 0,
                                   0);
    y -= GAME_SCREEN_HEIGHT / 2;
    if (y < 0)
        y = 0;
    return y;
}

int Mission::minScreenX()
{
    return g_App.maps().tileToScreenX(map_, min_x_, min_y_, 0, 0, 0);
}

int Mission::minScreenY()
{
    return g_App.maps().tileToScreenY(map_, min_x_, min_y_, 0, 0, 0);
}

int Mission::maxScreenX()
{
    return g_App.maps().tileToScreenX(map_, max_x_, max_y_, 0, 0, 0);
}

int Mission::maxScreenY()
{
    return g_App.maps().tileToScreenY(map_, max_x_, max_y_, 0, 0, 0);
}

int fastKey(int tx, int ty, int tz)
{
    return tx | (ty << 8) | (tz << 16);
}

int fastKey(MapObject * m)
{
    return fastKey(m->tileX(), m->tileY(), m->tileZ());
}

void Mission::drawMap(int scrollx, int scrolly)
{
    fast_vehicle_cache_.clear();
    for (unsigned int i = 0; i < vehicles_.size(); i++)
        fast_vehicle_cache_.insert(fastKey(vehicles_[i]));

    fast_ped_cache_.clear();
    for (unsigned int i = 0; i < 4; i++)
        if (g_App.teamMember(i)->isActive())
            fast_ped_cache_.insert(fastKey(peds_[i]));
    for (unsigned int i = 8; i < peds_.size(); i++)
        fast_ped_cache_.insert(fastKey(peds_[i]));

    fast_weapon_cache_.clear();
    for (unsigned int i = 0; i < weapons_.size(); i++)
        if (weapons_[i]->map() != -1)
            fast_weapon_cache_.insert(fastKey(weapons_[i]));

    fast_statics_cache_.clear();
    // TODO: static objects don't move :))
    for (unsigned int i = 0; i < statics_.size(); i++)
        fast_statics_cache_.insert(fastKey(statics_[i]));

    g_App.maps().drawMap(map_, scrollx, scrolly, this);
}

void Mission::drawAt(int tilex, int tiley, int tilez, int x, int y,
                     int scrollX, int scrollY)
{
    int key = fastKey(tilex, tiley, tilez);

    if (fast_vehicle_cache_.find(key) != fast_vehicle_cache_.end()) {
        // draw vehicles
        for (unsigned int i = 0; i < vehicles_.size(); i++)
            if (vehicles_[i]->tileX() == tilex
                && vehicles_[i]->tileY() == tiley
                && vehicles_[i]->tileZ() == tilez)
                vehicles_[i]->draw(x, y);
    }

    if (fast_ped_cache_.find(key) != fast_ped_cache_.end()) {
        // draw agents
        for (unsigned int i = 0; i < 4; i++)
            if (g_App.teamMember(i)->isActive())
                if (peds_[i]->tileX() == tilex
                    && peds_[i]->tileY() == tiley
                    && peds_[i]->tileZ() == tilez) {
                    peds_[i]->draw(x, y, scrollX, scrollY);
#if 0
                    g_Screen.drawLine(x - TILE_WIDTH / 2, y,
                                      x + TILE_WIDTH / 2, y, 11);
                    g_Screen.drawLine(x + TILE_WIDTH / 2, y,
                                      x + TILE_WIDTH / 2, y + TILE_HEIGHT,
                                      11);
                    g_Screen.drawLine(x + TILE_WIDTH / 2, y + TILE_HEIGHT,
                                      x - TILE_WIDTH / 2, y + TILE_HEIGHT,
                                      11);
                    g_Screen.drawLine(x - TILE_WIDTH / 2, y + TILE_HEIGHT,
                                      x - TILE_WIDTH / 2, y, 11);
#endif
                }
        // draw peds
        for (unsigned int i = 8; i < peds_.size(); i++)
            if (peds_[i]->tileX() == tilex && peds_[i]->tileY() == tiley
                && peds_[i]->tileZ() == tilez)
                peds_[i]->draw(x, y, scrollX, scrollY);
    }

    if (fast_weapon_cache_.find(key) != fast_weapon_cache_.end()) {
        // draw weapons
        for (unsigned int i = 0; i < weapons_.size(); i++)
            if (weapons_[i]->map() != -1 && weapons_[i]->tileX() == tilex
                && weapons_[i]->tileY() == tiley
                && weapons_[i]->tileZ() == tilez) {
                weapons_[i]->draw(x, y);
            }
    }

    if (fast_statics_cache_.find(key) != fast_statics_cache_.end()) {
        // draw statics
        for (unsigned int i = 0; i < statics_.size(); i++)
            if (statics_[i]->tileX() == tilex
                && statics_[i]->tileY() == tiley
                && statics_[i]->tileZ() == tilez) {
                statics_[i]->draw(x, y);
            }
    }
}

void Mission::start()
{
    for (int i = 0; i < 4; i++)
        if (g_App.teamMember(i)) {
            if(g_App.teamMember(i)->isActive()){
                peds_[i]->setHealth(g_App.teamMember(i)->health() *
                                peds_[i]->health() / 255);
                while (g_App.teamMember(i)->numWeapons()) {
                    WeaponInstance *wi = g_App.teamMember(i)->removeWeapon(0);
                    weapons_.push_back(wi);
                    peds_[i]->addWeapon(wi);
                }
                peds_[i]->setAsAgent(PedInstance::Agent_Active);
            }else{
                peds_[i]->setHealth(-1);
                peds_[i]->setAsAgent(PedInstance::Agent_Non_Active);
            }
        } else {
            peds_[i]->setHealth(-1);
            peds_[i]->setAsAgent(PedInstance::Agent_Non_Active);
        }
}

bool Mission::failed()
{
    // TODO: other ways to fail
    // INFO: peds are loaded in wrong way objective number
    // can be non-present
    //if (objective_ == 1 && peds_[objective_ped_]->health() <= 0)
        //return true;
    return false;
}

bool Mission::completed()
{
    // TODO: other ways to complete
    // INFO: peds are loaded in wrong way objective number
    // can be non-present
    //if (objective_ == 2 && peds_[objective_ped_]->health() <= 0)
        //return true;
    return false;
}

void Mission::end()
{
    //TODO: requires attention
    for (int i = 0; i < 4; i++)
        if (g_App.teamMember(i)) {
            // TODO: kill agents if associated ped is dead
            while (peds_[i]->numWeapons()) {
                WeaponInstance *wi = peds_[i]->removeWeapon(0);
                std::vector < WeaponInstance * >::iterator it =
                    weapons_.begin();
                while (it != weapons_.end() && *it != wi)
                    it++;
                assert(it != weapons_.end());
                weapons_.erase(it);
                g_App.teamMember(i)->addWeapon(wi);
            }
        }
}

void Mission::addWeapon(WeaponInstance * w)
{
    w->setMap(map_);
    for (unsigned int i = 0; i < weapons_.size(); i++)
        if (weapons_[i] == w)
            return;
    weapons_.push_back(w);
}

MapObject * Mission::findAt(int tilex, int tiley, int tilez,
                            int *majorType, int *searchIndex, bool only)
{
    switch(*majorType) {
        case 0:
            for (unsigned int i = *searchIndex; i < vehicles_.size(); i++)
                if (vehicles_[i]->tileX() == tilex
                    && vehicles_[i]->tileY() == tiley
                    && vehicles_[i]->tileZ() == tilez) {
                    *searchIndex = i + 1;
                    *majorType = 0;
                    return vehicles_[i];
                }
            if(only)
                return NULL;
            *searchIndex = 0;
        case 1:
            for (unsigned int i = *searchIndex; i < peds_.size(); i++)
                if (peds_[i]->tileX() == tilex && peds_[i]->tileY() == tiley
                    && peds_[i]->tileZ() == tilez) {
                    *searchIndex = i + 1;
                    *majorType = 1;
                    return peds_[i];
                }
            if(only)
                return NULL;
            *searchIndex = 0;
        case 2:
            for (unsigned int i = *searchIndex; i < weapons_.size(); i++)
                if (weapons_[i]->map() != -1 && weapons_[i]->tileX() == tilex
                    && weapons_[i]->tileY() == tiley
                    && weapons_[i]->tileZ() == tilez) {
                    *searchIndex = i + 1;
                    *majorType = 2;
                    return weapons_[i];
                }
            if(only)
                return NULL;
            *searchIndex = 0;
        case 3:
            for (unsigned int i = *searchIndex; i < statics_.size(); i++)
                if (statics_[i]->tileX() == tilex
                    && statics_[i]->tileY() == tiley
                    && statics_[i]->tileZ() == tilez) {
                    *searchIndex = i + 1;
                    *majorType = 3;
                    return statics_[i];
                }
    }
    return NULL;
}

bool Mission::sWalkable(char thisTile, char upperTile) {

    return (thisTile != upperTile
        && (((thisTile >= 0x05 && thisTile <= 0x09) ||
        thisTile == 0x0B || (thisTile >= 0x0D && thisTile <= 0x0F)
        || (thisTile == 0x11 || thisTile == 0x12))
            ? (upperTile == 0x0 || upperTile == 0x10) : true))
        || (thisTile > 0x00 && thisTile < 0x05
        && (upperTile == 0x0 || upperTile == 0x10));
}

bool Mission::isSurface(char thisTile) {
    return (thisTile >= 0x05 && thisTile <= 0x09) ||
        thisTile == 0x0B || (thisTile >= 0x0D && thisTile <= 0x0F)
        || (thisTile == 0x11 || thisTile == 0x12);
}

bool Mission::isStairs(char thisTile) {
    return thisTile >= 0x01 && thisTile <= 0x04;
}

bool Mission::setSurfaces() {

    // Description: creates map of walkable surfaces, also
    // defines directions where movement is possible

    // NOTE: tiles walkdata type 0x0D are quiet special, and they
    // are not handled correctly, these correction and andjustings
    // can create additional speed drain, as such I didn't
    // implemented them as needed. To make it possible a patch
    // required to walkdata and a lot of changes which I don't
    // want to do.
    // 0x10 appear above walking tile where train stops
    clrSurfaces();
    if (!(g_App.maps().mapDimensions(map_,
        &mmax_x_, &mmax_y_, &mmax_z_)))
        return false;
    mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    mtsurfaces_ = (surfaceDesc *)malloc(mmax_m_all * sizeof(surfaceDesc));
    mdpoints_ = (floodPointDesc *)malloc(mmax_m_all * sizeof(floodPointDesc));
    mdpoints_cp_ = (floodPointDesc *)malloc(mmax_m_all * sizeof(floodPointDesc));
    if(mtsurfaces_ == NULL || mdpoints_ == NULL || mdpoints_cp_ == NULL) {
        clrSurfaces();
        return false;
    }
    mmax_m_xy = mmax_x_ * mmax_y_;
    memset((void *)mtsurfaces_, 0, mmax_m_all * sizeof(surfaceDesc));
    memset((void *)mdpoints_, 0, mmax_m_all * sizeof(floodPointDesc));
    Map *m = g_App.maps().map(map_);
    for (int ix = 0; ix < mmax_x_; ix++) {
        for (int iy = 0; iy < mmax_y_; iy++) {
            for (int iz = 0; iz < mmax_z_; iz++) {
                mtsurfaces_[ix + iy * mmax_x_ + iz * mmax_m_xy].twd =
                    g_App.walkdata_p_[m->tileAt(ix, iy, iz)];
            }
        }
    }

    //printf("surface data size %i\n", sizeof(surfaceDesc) * mmax_m_all);
    //printf("flood data size %i\n", sizeof(floodPointDesc) * mmax_m_all);

    for (unsigned int i = 0; i < peds_.size(); i++) {
        PedInstance *p = peds_[i];
        int x = p->tileX();
        int y = p->tileY();
        int z = p->tileZ();
        if (z >= mmax_z_)
            continue;
        if (mdpoints_[x + y * mmax_x_ + z * mmax_m_xy].t == m_fdNotDefined) {
            toDefineXYZ stodef;
            std::vector<toDefineXYZ> vtodefine;
            mdpoints_[x + y * mmax_x_ + z * mmax_m_xy].t = m_fdDefReq;
            stodef.x = x;
            stodef.y = y * mmax_x_;
            stodef.z = z * mmax_m_xy;
            vtodefine.push_back(stodef);
            do {
                stodef = vtodefine.back();
                vtodefine.pop_back();
                x = stodef.x;
                y = stodef.y;
                z = stodef.z;
                int xm = x - 1;
                int ym = y - mmax_x_;
                int zm = z - mmax_m_xy;
                int xp = x + 1;
                int yp = y + mmax_x_;
                int zp = z + mmax_m_xy;
                surfaceDesc *ms = &(mtsurfaces_[x + y + z]);
                surfaceDesc *nxts;
                uint8 this_s = ms->twd;
                uint8 upper_s = 0;
                floodPointDesc *cfp = &(mdpoints_[x + y + z]);
                floodPointDesc *nxtfp;
                if (zp < (mmax_m_xy * (mmax_z_ - 2))) {
                    upper_s = mtsurfaces_[x + y + zp].twd;
                    if(!sWalkable(this_s, upper_s)) {
                        cfp->t = m_fdNonWalkable;
                        continue;
                    }
                } else {
                    cfp->t = m_fdNonWalkable;
                    continue;
                }
                unsigned char sdirm = 0x0;
                unsigned char sdirh = 0x0;
                unsigned char sdirl = 0x0;
                unsigned char sdirmr = 0;

                switch (this_s) {
                    case 0x00:
                        cfp->t = m_fdNonWalkable;
                        break;
                    case 0x01:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (yp < mmax_m_xy) {
                                nxts = &(mtsurfaces_[x + yp + zm]);
                                nxtfp = &(mdpoints_[x + yp + zm]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + yp + z].twd;
                                if (isSurface(this_s) || this_s == 0x01) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x01;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = yp;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xm >= 0) {
                                nxts = &(mtsurfaces_[xm + y + zm]);
                                nxtfp = &(mdpoints_[xm + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x40;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xp < mmax_x_) {
                                nxts = &(mtsurfaces_[xp + y + zm]);
                                nxtfp = &(mdpoints_[xp + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x04;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (ym >= 0) {
                            nxts = &(mtsurfaces_[x + ym + z]);
                            nxtfp = &(mdpoints_[x + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + ym + zp].twd;
                            if(isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= 0x10;
                            } else {
                                nxtfp->t = m_fdNonWalkable;
                                if(upper_s == 0x01 && (zp + mmax_m_xy) < mmax_m_all) {
                                    if(sWalkable(upper_s, mtsurfaces_[x + ym + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x10;
                                        if(mdpoints_[x + ym + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + ym + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = ym;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = ym;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (xm >= 0) {
                            nxts = &(mtsurfaces_[xm + y + z]);
                            nxtfp = &(mdpoints_[xm + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xm + y + zp].twd;
                            if (isSurface(this_s) || this_s == 0x01) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x40;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xm;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if (isSurface(this_s) || this_s == 0x01) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |=0x04;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x02:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (ym >= 0) {
                                nxts = &(mtsurfaces_[x + ym + zm]);
                                nxtfp = &(mdpoints_[x + ym + zm]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + ym + z].twd;
                                if (isSurface(this_s) || this_s == 0x02) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x10;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = ym;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xm >= 0) {
                                nxts = &(mtsurfaces_[xm + y + zm]);
                                nxtfp = &(mdpoints_[xm + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x40;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xp < mmax_x_) {
                                nxts = &(mtsurfaces_[xp + y + zm]);
                                nxtfp = &(mdpoints_[xp + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x04;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if(isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= 0x01;
                            } else {
                                nxtfp->t = m_fdNonWalkable;
                                if(upper_s == 0x02 && (zp + mmax_m_xy) < mmax_m_all) {
                                    if(sWalkable(upper_s, mtsurfaces_[x + yp + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x01;
                                        if(mdpoints_[x + yp + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + yp + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = yp;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (xm >= 0) {
                            nxts = &(mtsurfaces_[xm + y + z]);
                            nxtfp = &(mdpoints_[xm + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xm + y + zp].twd;
                            if (isSurface(this_s) || this_s == 0x02) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x40;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xm;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if (isSurface(this_s) || this_s == 0x02) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x04;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x03:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (xm >= 0) {
                                nxts = &(mtsurfaces_[xm + y + zm]);
                                nxtfp = &(mdpoints_[xm + y + zm]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + y + z].twd;
                                if (isSurface(this_s) || this_s == 0x03) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x40;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (ym >= 0) {
                                nxts = &(mtsurfaces_[x + ym + zm]);
                                nxtfp = &(mdpoints_[x + ym + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + ym + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x10;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = ym;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (yp < mmax_m_xy) {
                                nxts = &(mtsurfaces_[x + yp + zm]);
                                nxtfp = &(mdpoints_[x + yp + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + yp + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x01;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = yp;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if(isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= 0x04;
                            } else {
                                nxtfp->t = m_fdNonWalkable;
                                if(upper_s == 0x03 && (zp + mmax_m_xy) < mmax_m_all) {
                                    if(sWalkable(upper_s, mtsurfaces_[xp + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x04;
                                        if(mdpoints_[xp + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xp + y + zp].t = m_fdDefReq;
                                            stodef.x = xp;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (ym >= 0) {
                            nxts = &(mtsurfaces_[x + ym + z]);
                            nxtfp = &(mdpoints_[x + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + ym + zp].twd;
                            if (isSurface(this_s) || this_s == 0x03) {
                                if (sWalkable(this_s, upper_s)) { 
                                    sdirm |= 0x10;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = ym;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if (isSurface(this_s) || this_s == 0x03) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x01;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x04:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (xp < mmax_x_) {
                                nxts = &(mtsurfaces_[xp + y + zm]);
                                nxtfp = &(mdpoints_[xp + y + z]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + y + z].twd;
                                if (isSurface(this_s) || this_s == 0x04) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x04;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (ym >= 0) {
                                nxts = &(mtsurfaces_[x + ym + zm]);
                                nxtfp = &(mdpoints_[x + ym + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + ym + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x10;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = ym;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (yp < mmax_m_xy) {
                                nxts = &(mtsurfaces_[x + yp + zm]);
                                nxtfp = &(mdpoints_[x + yp + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + yp + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x01;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = yp;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (xm >= 0) {
                            nxts = &(mtsurfaces_[xm + y + z]);
                            nxtfp = &(mdpoints_[xm + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xm + y + zp].twd;
                            if(isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= 0x40;
                            } else {
                                nxtfp->t = m_fdNonWalkable;
                                if(upper_s == 0x04 && (zp + mmax_m_xy) < mmax_m_all) {
                                    if(sWalkable(upper_s, mtsurfaces_[xm + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x40;
                                        if(mdpoints_[xm + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xm + y + zp].t = m_fdDefReq;
                                            stodef.x = xm;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xm;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (ym >= 0) {
                            nxts = &(mtsurfaces_[x + ym + z]);
                            nxtfp = &(mdpoints_[x + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + ym + zp].twd;
                            if (isSurface(this_s) || this_s == 0x04) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x10;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = ym;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if (isSurface(this_s) || this_s == 0x04) {
                                if (sWalkable(this_s, upper_s)) {
                                    sdirm |= 0x01;
                                } else {
                                    nxtfp->t = m_fdNonWalkable;
                                }
                            }
                            if(nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x05:
                    case 0x06:
                    case 0x07:
                    case 0x08:
                    case 0x09:
                    case 0x0B:
                    case 0x0D:
                    case 0x0E:
                    case 0x0F:
                        cfp->t = m_fdWalkable;
                        if (xm >=0) {
                            nxts = &(mtsurfaces_[xm + y + z]);
                            nxtfp = &(mdpoints_[xm + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xm + y + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x20 | 0x40 | 0x80);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x20 | 0x80);
                                if (this_s == 0x01 || this_s == 0x02 || this_s == 0x03){
                                    sdirm |= 0x40;
                                }
                            } else {
                                sdirmr |= (0x20 | 0x80);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x02 || upper_s == 0x04
                                    || upper_s == 0x12)) {
                                    if (sWalkable(upper_s, mtsurfaces_[xm + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x40;
                                        if (mdpoints_[xm + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xm + y + zp].t = m_fdDefReq;
                                            stodef.x = xm;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xm;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x20 | 0x80);

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x02 | 0x04 | 0x08);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x02 | 0x08);
                                if (this_s == 0x01 || this_s == 0x02 || this_s == 0x04){
                                    sdirm |= 0x04;
                                }
                            } else {
                                sdirmr |= (0x02 | 0x08);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x02 || upper_s == 0x03
                                    || upper_s == 0x11)) {
                                    if (sWalkable(upper_s, mtsurfaces_[xp + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x04;
                                        if (mdpoints_[xp + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xp + y + zp].t = m_fdDefReq;
                                            stodef.x = xp;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x02 | 0x08);

                        if(ym >= 0) {
                            nxts = &(mtsurfaces_[x + ym + z]);
                            nxtfp = &(mdpoints_[x + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + ym + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x08 | 0x10 | 0x20);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x08 | 0x20);
                                if (this_s == 0x02 || this_s == 0x03 || this_s == 0x04){
                                    sdirm |= 0x10;
                                }
                            } else {
                                sdirmr |= (0x08 | 0x20);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x03 || upper_s == 0x04
                                    || upper_s == 0x11)) {
                                    if (sWalkable(upper_s, mtsurfaces_[x + ym + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x10;
                                        if (mdpoints_[x + ym + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + ym + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = ym;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = ym;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x08 | 0x20);

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x80 | 0x01 | 0x02);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x80 | 0x02);
                                if (this_s == 0x01 || this_s == 0x03 || this_s == 0x04) {
                                    sdirm |= 0x01;
                                }
                            } else {
                                sdirmr |= (0x80 | 0x02);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x02 || upper_s == 0x03 || upper_s == 0x04
                                    || upper_s == 0x12)) {
                                    if (sWalkable(upper_s, mtsurfaces_[x + yp + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x01;
                                        if (mdpoints_[x + yp + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + yp + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = yp;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x80 | 0x02);
                        sdirm &= (0xFF ^ sdirmr);

                        // edges

                        if (xm >= 0) {
                            if ( ym >= 0 && (sdirm & 0x20) != 0) {
                                nxts = &(mtsurfaces_[xm + ym + z]);
                                nxtfp = &(mdpoints_[xm + ym + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + ym + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x20);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = ym;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }

                            if ( yp < mmax_m_xy && (sdirm & 0x80) != 0) {
                                nxts = &(mtsurfaces_[xm + yp + z]);
                                nxtfp = &(mdpoints_[xm + yp + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + yp + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x80);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = yp;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (xp < mmax_x_) {
                            if ( ym >= 0 && (sdirm & 0x08) != 0) {
                                nxts = &(mtsurfaces_[xp + ym + z]);
                                nxtfp = &(mdpoints_[xp + ym + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + ym + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x08);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = ym;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }

                            if ( yp < mmax_m_xy && (sdirm & 0x02) != 0) {
                                nxts = &(mtsurfaces_[xp + yp + z]);
                                nxtfp = &(mdpoints_[xp + yp + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + yp + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x02);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = yp;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x0A:
                    case 0x0C:
                    case 0x10:
                        cfp->t = m_fdNonWalkable;
                        break;
                    case 0x11:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (xm >= 0) {
                                nxts = &(mtsurfaces_[xm + y + zm]);
                                nxtfp = &(mdpoints_[xm + y + zm]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x40;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (ym >= 0) {
                                nxts = &(mtsurfaces_[x + ym + zm]);
                                nxtfp = &(mdpoints_[x + ym + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + ym + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x10;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = ym;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (yp < mmax_m_xy) {
                                nxts = &(mtsurfaces_[x + yp + zm]);
                                nxtfp = &(mdpoints_[x + yp + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + yp + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x01;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = yp;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x02 | 0x04 | 0x08);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x02 | 0x08);
                                if (this_s == 0x01 || this_s == 0x02 || this_s == 0x04){
                                    sdirm |= 0x04;
                                }
                            } else {
                                sdirmr |= (0x02 | 0x08);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x02 || upper_s == 0x03)) {
                                    if (sWalkable(upper_s, mtsurfaces_[xp + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x04;
                                        if (mdpoints_[xp + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xp + y + zp].t = m_fdDefReq;
                                            stodef.x = xp;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x02 | 0x08);

                        if(ym >= 0) {
                            nxts = &(mtsurfaces_[x + ym + z]);
                            nxtfp = &(mdpoints_[x + ym + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + ym + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x08 | 0x10);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x08 | 0x20);
                                if (this_s == 0x02 || this_s == 0x03 || this_s == 0x04){
                                    sdirm |= 0x10;
                                }
                            } else {
                                sdirmr |= (0x08 | 0x20);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x03 || upper_s == 0x04)) {
                                    if (sWalkable(upper_s, mtsurfaces_[x + ym + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x10;
                                        if (mdpoints_[x + ym + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + ym + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = ym;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = ym;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x08);

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x01 | 0x02);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x80 | 0x02);
                                if (this_s == 0x01 || this_s == 0x03 || this_s == 0x04) {
                                    sdirm |= 0x01;
                                }
                            } else {
                                sdirmr |= (0x80 | 0x02);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x02 || upper_s == 0x03 || upper_s == 0x04)) {
                                    if (sWalkable(upper_s, mtsurfaces_[x + yp + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x01;
                                        if (mdpoints_[x + yp + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + yp + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = yp;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x80 | 0x02);
                        sdirm &= (0xFF ^ sdirmr);

                        // edges
                        if (xp < mmax_x_) {
                            if ( ym >= 0 && (sdirm & 0x08) != 0) {
                                nxts = &(mtsurfaces_[xp + ym + z]);
                                nxtfp = &(mdpoints_[xp + ym + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + ym + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x08);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = ym;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }

                            if ( yp < mmax_m_xy && (sdirm & 0x02) != 0) {
                                nxts = &(mtsurfaces_[xp + yp + z]);
                                nxtfp = &(mdpoints_[xp + yp + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + yp + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x02);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = yp;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }
                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                    case 0x12:
                        cfp->t = m_fdWalkable;
                        if (zm >= 0) {
                            if (ym >= 0) {
                                nxts = &(mtsurfaces_[x + ym + zm]);
                                nxtfp = &(mdpoints_[x + ym + zm]);
                                mdpoints_[x + y + zm].t = m_fdNonWalkable;
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[x + ym + z].twd;
                                if (isSurface(this_s) || this_s == 0x02) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x10;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = x;
                                    stodef.y = ym;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xm >= 0) {
                                nxts = &(mtsurfaces_[xm + y + zm]);
                                nxtfp = &(mdpoints_[xm + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x40;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xp < mmax_x_) {
                                nxts = &(mtsurfaces_[xp + y + zm]);
                                nxtfp = &(mdpoints_[xp + y + zm]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + y + z].twd;
                                if (isSurface(this_s)) {
                                    if(sWalkable(this_s, upper_s)) {
                                        sdirl |= 0x04;
                                    } else {
                                        nxtfp->t = m_fdNonWalkable;
                                    }
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = y;
                                    stodef.z = zm;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        if (xm >=0) {
                            nxts = &(mtsurfaces_[xm + y + z]);
                            nxtfp = &(mdpoints_[xm + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xm + y + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x40 | 0x80);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x20 | 0x80);
                                if (this_s == 0x01 || this_s == 0x02 || this_s == 0x03){
                                    sdirm |= 0x40;
                                }
                            } else {
                                sdirmr |= (0x20 | 0x80);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x02 || upper_s == 0x04)) {
                                    if (sWalkable(upper_s, mtsurfaces_[xm + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x40;
                                        if (mdpoints_[xm + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xm + y + zp].t = m_fdDefReq;
                                            stodef.x = xm;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xm;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x20 | 0x80);

                        if (xp < mmax_x_) {
                            nxts = &(mtsurfaces_[xp + y + z]);
                            nxtfp = &(mdpoints_[xp + y + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[xp + y + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x02 | 0x04);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x02 | 0x08);
                                if (this_s == 0x01 || this_s == 0x02 || this_s == 0x04){
                                    sdirm |= 0x04;
                                }
                            } else {
                                sdirmr |= (0x02 | 0x08);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x01 || upper_s == 0x02 || upper_s == 0x03)) {
                                    if (sWalkable(upper_s, mtsurfaces_[xp + y + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x04;
                                        if (mdpoints_[xp + y + zp].t == m_fdNotDefined) {
                                            mdpoints_[xp + y + zp].t = m_fdDefReq;
                                            stodef.x = xp;
                                            stodef.y = y;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = xp;
                                stodef.y = y;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x02 | 0x08);

                        if (yp < mmax_m_xy) {
                            nxts = &(mtsurfaces_[x + yp + z]);
                            nxtfp = &(mdpoints_[x + yp + z]);
                            this_s = nxts->twd;
                            upper_s = mtsurfaces_[x + yp + zp].twd;
                            if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                sdirm |= (0x80 | 0x01 | 0x02);
                            } else if (isStairs(this_s) && sWalkable(this_s, upper_s)) {
                                sdirmr |= (0x80 | 0x02);
                                if (this_s == 0x01 || this_s == 0x03 || this_s == 0x04) {
                                    sdirm |= 0x01;
                                }
                            } else {
                                sdirmr |= (0x80 | 0x02);
                                nxtfp->t = m_fdNonWalkable;
                                if ((zp + mmax_m_xy) < mmax_m_all
                                    && (upper_s == 0x02 || upper_s == 0x03 || upper_s == 0x04)) {
                                    if (sWalkable(upper_s, mtsurfaces_[x + yp + (zp + mmax_m_xy)].twd)) {
                                        sdirh |= 0x01;
                                        if (mdpoints_[x + yp + zp].t == m_fdNotDefined) {
                                            mdpoints_[x + yp + zp].t = m_fdDefReq;
                                            stodef.x = x;
                                            stodef.y = yp;
                                            stodef.z = zp;
                                            vtodefine.push_back(stodef);
                                        }
                                    }
                                }
                            }
                            if (nxtfp->t == m_fdNotDefined) {
                                nxtfp->t = m_fdDefReq;
                                stodef.x = x;
                                stodef.y = yp;
                                stodef.z = z;
                                vtodefine.push_back(stodef);
                            }
                        } else
                            sdirmr |= (0x80 | 0x02);
                        sdirm &= (0xFF ^ sdirmr);

                        // edges
                        if (yp < mmax_m_xy) {
                            if ( xm >= 0 && (sdirm & 0x80) != 0) {
                                nxts = &(mtsurfaces_[xm + yp + z]);
                                nxtfp = &(mdpoints_[xm + yp + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xm + yp + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x80);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xm;
                                    stodef.y = yp;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }
                            if (xp < mmax_x_ && (sdirm & 0x02) != 0) {
                                nxts = &(mtsurfaces_[xp + yp + z]);
                                nxtfp = &(mdpoints_[xp + yp + z]);
                                this_s = nxts->twd;
                                upper_s = mtsurfaces_[xp + yp + zp].twd;
                                if (isSurface(this_s) && sWalkable(this_s, upper_s)) {
                                    ;
                                } else {
                                    if (isSurface(this_s))
                                        nxtfp->t = m_fdNonWalkable;
                                    sdirm &= (0xFF ^ 0x02);
                                }
                                if(nxtfp->t == m_fdNotDefined) {
                                    nxtfp->t = m_fdDefReq;
                                    stodef.x = xp;
                                    stodef.y = yp;
                                    stodef.z = z;
                                    vtodefine.push_back(stodef);
                                }
                            }
                        }

                        cfp->dirm = sdirm;
                        cfp->dirh = sdirh;
                        cfp->dirl = sdirl;

                        break;
                }
            } while (vtodefine.size());
        }
    }
#if 0
    unsigned int cw = 0;
    for (int iz = 0; iz < mmax_z_; iz++) {
        for (int iy = 0; iy < mmax_y_; iy++) {
            for (int ix = 0; ix < mmax_x_; ix++) {
                floodPointDesc *cfpp = &(mdpoints_[ix + iy * mmax_x_ + iz * mmax_m_xy]);

                if ((cfpp->t & m_fdWalkable) == m_fdWalkable)
                    cw++;
            }
        }
    }

    printf("flood walkables %i\n", cw);
#endif
    return true;
}

void Mission::clrSurfaces() {

    if(mtsurfaces_ != NULL) {
        free(mtsurfaces_);
        mtsurfaces_ = NULL;
    }
    if(mdpoints_ != NULL) {
        free(mdpoints_);
        mdpoints_ = NULL;
    }
    if(mdpoints_cp_ != NULL) {
        free(mdpoints_cp_);
        mdpoints_cp_ = NULL;
    }
}

bool Mission::getWalkable(int &x, int &y, int &z, int &ox, int &oy) {
    bool gotit = false;
    int bx, by, box, boy;
    int bz = mmax_z_;
    unsigned int cindx;
    unsigned char twd;
    do{
        bz--;
        bx = x * 256 + ox + 128 * bz;
        box = bx % 256;
        bx = bx / 256;
        by = y * 256 + oy + 128 * bz;
        boy = by % 256;
        by = by / 256;
        if (bz >= mmax_z_ || bx >= mmax_x_ || by >= mmax_y_)
            continue;
        cindx = bx + by * mmax_x_ + bz * mmax_m_xy;
        if (mdpoints_[cindx].t == m_fdWalkable) {
            twd = mtsurfaces_[cindx].twd;
            int dx = 0;
            int dy = 0;
            switch (twd) {
                case 0x01:
                    dy = (boy * 2) / 3;
                    dx = box - dy / 2;
                    if (dx >= 0) {
                        gotit = true;
                        box = dx;
                        boy = dy;
                    } else {
                        if ((bx - 1) >= 0) {
                            cindx = (bx - 1) + by * mmax_x_ + bz * mmax_m_xy;
                            if (mdpoints_[cindx].t == m_fdWalkable) {
                                if (mtsurfaces_[cindx].twd == 0x01) {
                                    gotit = true;
                                    bx--;
                                    box = dx + 256;
                                    boy = dy;
                                }
                            }
                        }
                    }
                    break;
                case 0x02:
                    dy = (boy - 128) * 2;
                    dx = (box + dy / 2) - 128;
                    if (dy >= 0) {
                        if (dx >= 0) {
                            if (dx < 256) {
                                gotit = true;
                                box = dx;
                                boy = dy;
                            } else {
                                if ((bx + 1) < mmax_x_) {
                                    cindx = (bx + 1) + by * mmax_x_ + bz * mmax_m_xy;
                                    if (mdpoints_[cindx].t == m_fdWalkable) {
                                        if (mtsurfaces_[cindx].twd == 0x02) {
                                            gotit = true;
                                            bx++;
                                            box = dx - 256;
                                            boy = dy;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ((bx - 1) >= 0) {
                                cindx = (bx - 1) + by * mmax_x_ + bz * mmax_m_xy;
                                if (mdpoints_[cindx].t == m_fdWalkable) {
                                    if (mtsurfaces_[cindx].twd == 0x02) {
                                        gotit = true;
                                        bx--;
                                        box = dx + 256;
                                        boy = dy;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case 0x03:
                    dx = (box - 128) * 2;
                    dy = (boy + dx / 2) - 128;
                    if (dx >= 0) {
                        if (dy >= 0) {
                            if (dy < 256) {
                                gotit = true;
                                box = dx;
                                boy = dy;
                            } else {
                                if ((by + 1) < mmax_y_) {
                                    cindx = bx + (by + 1) * mmax_x_ + bz * mmax_m_xy;
                                    if (mdpoints_[cindx].t == m_fdWalkable) {
                                        if (mtsurfaces_[cindx].twd == 0x03) {
                                            gotit = true;
                                            by++;
                                            box = dx;
                                            boy = dy - 256;
                                        }
                                    }
                                }
                            }
                        } else {
                            if ((by - 1) >= 0) {
                                cindx = bx + (by - 1) * mmax_x_ + bz * mmax_m_xy;
                                if (mdpoints_[cindx].t == m_fdWalkable) {
                                    if (mtsurfaces_[cindx].twd == 0x03) {
                                        gotit = true;
                                        by--;
                                        box = dx;
                                        boy = dy + 256;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case 0x04:
                    dx = (box * 2) / 3;
                    dy = boy - dx / 2;
                    if (dy >= 0) {
                        gotit = true;
                        box = dx;
                        boy = dy;
                    } else {
                        if ((by - 1) >= 0) {
                            cindx = bx + (by - 1) * mmax_x_ + bz * mmax_m_xy;
                            if (mdpoints_[cindx].t == m_fdWalkable) {
                                if (mtsurfaces_[cindx].twd == 0x04) {
                                    gotit = true;
                                    by--;
                                    box = dx;
                                    boy = dy + 256;
                                }
                            }
                        }
                    }
                    break;
                default:
                    gotit = true;
                break;
            }
        } else {
            if (box < 128) {
                if ((bx - 1) >= 0) {
                    cindx = (bx - 1) + by * mmax_x_ + bz * mmax_m_xy;
                    if (mdpoints_[cindx].t == m_fdWalkable) {
                        int dx = 0;
                        int dy = 0;
                        twd = mtsurfaces_[cindx].twd;
                        if (twd == 0x01) {
                            dy = (boy * 2) / 3;
                            dx = (box + 256) - dy / 2;
                            if (dx < 256) {
                                bx--;
                                gotit = true;
                                box = dx;
                                boy = dy;
                            }
                        } else if (twd == 0x04) {
                            dx = ((box + 256) * 2) / 3;
                            dy = boy - dx / 2;
                            if (dy >= 0) {
                                bx--;
                                gotit = true;
                                box = dx;
                                boy = dy;
                            }
                        }
                    }
                }
            }
            if (!gotit && boy < 128) {
                if ((by - 1) >= 0) {
                    cindx = bx + (by - 1) * mmax_x_ + bz * mmax_m_xy;
                    if (mdpoints_[cindx].t == m_fdWalkable) {
                        int dx = 0;
                        int dy = 0;
                        twd = mtsurfaces_[cindx].twd;
                        if (twd == 0x01) {
                            dy = ((boy + 256) * 2) / 3;
                            dx = box - dy / 2;
                            if (dx >= 0) {
                                by--;
                                gotit = true;
                                box = dx;
                                boy = dy;
                            }
                        } else if (twd == 0x04) {
                            dx = (box * 2) / 3;
                            dy = (boy + 256) - dx / 2;
                            if (dy < 256) {
                                by--;
                                gotit = true;
                                box = dx;
                                boy = dy;
                            }
                        }
                    }
                    if(!gotit && box < 128 && (bx - 1) >= 0) {
                        cindx--;
                        if (mdpoints_[cindx].t == m_fdWalkable) {
                            int dx = 0;
                            int dy = 0;
                            twd = mtsurfaces_[cindx].twd;
                            if (twd == 0x01) {
                                dy = ((boy + 256) * 2) / 3;
                                dx = (box + 256) - dy / 2;
                                if (dx < 256 && dy < 256) {
                                    bx--;
                                    by--;
                                    gotit = true;
                                    box = dx;
                                    boy = dy;
                                }
                            } else if (twd == 0x04) {
                                dx = ((box + 256) * 2) / 3;
                                dy = (boy + 256) - dx / 2;
                                if (dx < 256 && dy < 256) {
                                    bx--;
                                    by--;
                                    gotit = true;
                                    box = dx;
                                    boy = dy;
                                }
                            }
                        }
                    }
                }
            }
        }
    }while (bz != 0 && !gotit);
    if (gotit) {
        x = bx;
        y = by;
        z = bz;
        ox = box;
        oy = boy;
    }
    return gotit;
}

void Mission::adjXYZ(int &x, int &y, int &z) {
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (z < 0 || z >= mmax_z_)
        z = 0;
    if (x >= mmax_x_)
        x = mmax_x_ - 1;
    if (y >= mmax_y_)
        y = mmax_y_ - 1;
}
