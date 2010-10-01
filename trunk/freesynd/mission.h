/************************************************************************
 *                                                                      *
 *  FreeSynd - a remake of the classic Bullfrog game "Syndicate".       *
 *                                                                      *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>              *
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>   *
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>              *
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
 *  **																	*
 *																		*
 *  Thanks to Tomasz Lis for some preliminary work on the level files.	*
 *  Some of his code is included below (without permission).			*
 *																		*
 ************************************************************************/

#ifndef MISSION_H
#define MISSION_H

#include "common.h"
#include "mapobject.h"
#include "pathsurfaces.h"
#include <string>
#include <vector>
#include <set>

class VehicleInstance;
class PedInstance;
class WeaponInstance;
class Static;
class MapHelper;

/*!
 * Contains information read from original mission data file.
 */
class Mission : public MapHelper {
public:
    Mission();
    virtual ~Mission();
    bool loadMission(uint8 *missData, int size);
    bool loadLevel(uint8 *levelData);
    bool loadMap();

    int map() { return map_; }

    int mapWidth();
    int mapHeight();
    int startX();
    int startY();

    int minX() { return min_x_; }
    int minY() { return min_y_; }
    int maxX() { return max_x_; }
    int maxY() { return max_y_; }

    int minScreenX();
    int minScreenY();
    int maxScreenX();
    int maxScreenY();
    void drawMap(int scrollx, int scrolly);
    virtual void drawAt(int tilex, int tiley, int tilez, int x, int y,
            int scrollX, int scrollY);
    const char *briefing() { return briefing_.c_str(); }

    int infoCost(int lvl) {
        assert(lvl < 3);
        return info_costs_[lvl];
    }

    int enhanceCost(int lvl) {
        assert(lvl < 3);
        return enhance_costs_[lvl];
    }

    int numPeds() { return (int) peds_.size(); }
    PedInstance *ped(int i) { return peds_[i]; }
    int numVehicles() { return (int) vehicles_.size(); }
    VehicleInstance *vehicle(int i) { return vehicles_[i]; }

    int numWeapons() { return (int) weapons_.size(); }
    WeaponInstance *weapon(int i) { return weapons_[i]; }
    int numStatics() { return (int) statics_.size(); }
    Static *statics(int i) { return statics_[i]; }
    int objective() { return objective_; }
    int objectivePed() { return objective_ped_; }
    int objectiveVehicle() { return objective_vehicle_; }

    void start();
    bool failed();
    bool completed();
    void end();

    void addWeapon(WeaponInstance *w);
    virtual MapObject * findAt(int tilex, int tiley, int tilez,
            int *majorType, int *searchIndex, bool only);
    bool setSurfaces();
    void clrSurfaces();
    bool getWalkable(int &x, int &y, int &z, int &ox, int &oy);
    void adjXYZ(int &x, int &y, int &z);

    typedef struct {
        uint8 unkn10[8];
    } LEVELDATA_UNKN01;         // total: 8 bytes

    typedef struct {
        uint8 objs[128][128][2];
    } LEVELDATA_MAP;            // total: 32768 bytes

    // This structure contains all people in the game, including agents
    // This struct size is 92.
    // most 2 byte variable have little-endianess byte order(LE)
    typedef struct {
        // (LE data)'offset + 32774' gives the offset in this
        // file of the next object
        uint8 offset_next[2];
        // (LE data)'offset + 32774' gives the offset in this
        // file of the previous object (sometimes weapon, or
        // the next target for example ???)
        uint8 offset_prev[2];
        // [0] - offset, [1] - tile
        uint8 mapposx[2];
        uint8 mapposy[2];
        // to convert to our system use as uint16(LE)
        // tile = (uint16)/128, offz =(uint16)%128 or offz = mapposz[0] & 0x1F
        uint8 mapposz[2];
        //0x04 - ped is walking
        //0x05 - ped is driving
        //0x0D - they are not visible/present on original map(on water located), purpose?
        //0x0C - located level above possible walking surface, purpose?
        //0x0D and 0x0C are exluded from being loaded
        uint8 desc;
        uint8 unkn2;            // this bit is connected with control
        uint8 unkn3[2];         // nothing changes when this changes
        uint8 index_base_anim[2];  //index in (HSTA-0.ANI)
        uint8 index_current_frame[2];   //index in (HFRA-0.ANI)
        uint8 index_current_anim[2]; // index in (HSTA-0.ANI)
        uint8 health[2]; // something to do with amo / health
        uint8 offset_last_enemy[2];
        //0x01 ped; 0x02 vehicle; 0x04 weapon;
        //0x05 object; allow to display a target, a pickup, and for minimap
        uint8 type;
        uint8 status;
        //from 0xF0 to 0x10 : south = 0
        //from 0x10 to 0x30 : south-east = 1
        //from 0x30 to 0x50 : east = 2
        //from 0x50 to 0x70 : east-north = 3
        //from 0x70 to 0x90 : north = 4
        //from 0x90 to 0xB0 : north-west = 5
        //from 0xB0 to 0xD0 : west = 6
        //from 0xD0 to 0xF0 : west-south = 7
        // surface is mapped not to 360 degrees/surface, but 256 degrees/surface
        uint8 orientation;
        uint8 unkn4;
        uint8 type_ped; // when 01 pedestrian, 02 agent, 04 police, 08 guard : change IA and minimap
        uint8 unkn5[3];
        uint8 offset_of_persuader[2];
        uint8 unkn6[2];
        uint8 offset_of_vehicle[2];
        uint8 offset_scenario_1[2];
        uint8 offset_scenario_2[2];
        uint8 unkn7[2];
        uint8 offset_of_vehicle_2[2]; // ??
        uint8 goto_mapposx[2];
        uint8 goto_mapposy[2];
        uint8 goto_mapposz[2];
        uint8 unkn8[6];
        uint8 offset_equipment[2];
        //bitmask, 0b - gender, 1-2b - leg, 3-4b - arm, 5-6b - chest
        // 7-8b - heart, 9-10b - eye, 11-12b - brain, 13-15b - unknown
        uint8 mods_info[2];
        uint8 unkn9[6];
        uint8 offset_cur_weapon[2];
        uint8 unkn10;
        // IPA levels: white bar level,set level,exhaused level and forced level
        uint8 adrenalevels[4];
        uint8 intelilevels[4];
        uint8 perceplevels[4];
        uint8 unkn11;           // This value is different for most objects
        uint8 unkn12[8];       // ??
    } LEVELDATA_PEOPLE;         // total: 92 bytes

    // This structure contains all cars in the game, including destroyed
    // This struct size is 42.
    typedef struct {
        uint8 unkn2[4];
        uint8 mapposx[2];
        uint8 mapposy[2];
        uint8 mapposz[2];
        uint8 unkn3;            // if not set to 4, there is no car
        uint8 unkn4[5];         // somewhere here is damage made to car (filled with 122, one shot destroys car)
        uint8 currvistype;      // actual car picture; changed automatically when car is moving
        uint8 unkn6;            // car is not exist when not set to 6
        uint8 unkn7[3];         // ???
        // Half of structure - 21 bytes
        uint8 unkn8[3];         // usually zeroes
        uint8 status;           // Car status;2-can enter into car
        uint8 type;             // Car type;13-standard
        uint8 direction[4];     // direction the car is turned; setting all to 122 makes you die when exiting car (?)
        uint8 unkn11[2];        // ??
        uint8 unkn12[2];        // if both set to 122, there is no car
        uint8 unkn16[6];        // usually zeroes
        uint8 speed;            // changing this to 0 make car unable to move. The bigger, the faster car is.
        uint8 unkn20;           // ??
    } LEVELDATA_CARS;           // total: 42 bytes

    typedef struct {
        uint8 unkn1[4];
        uint8 mapposx[2];
        uint8 mapposy[2];
        uint8 mapposz[2];
        uint8 unkn3;            // seems to be set to 4 or 6
        uint8 zero[3];
        uint8 unknAnim[2];      // animation (LE data)
        uint8 subType;            // sub-type, for map editor maybe
        uint8 objType;            // looks like major type
        uint8 firstAnim[2];     // objects first animation (LE data)
        uint8 unkn10;           // 28 or 32
        uint8 zero1[3];
        uint8 unkn11;           // seems to be set to 5
        uint8 unkn12;           // 0C, 13, 16
        uint8 unkn13;           // 40, 80, 00
        uint8 zero2[3];

/*

statics data from first level:

00 00 00 00 D4 49 7C 34 00 01 04 00 00 00 35 04 6E 12 36 04 28 00 00 00 05 0C 40 00 00 00
00 00 00 00 64 3C EC 30 00 01 04 00 00 00 4D 04 8A 12 4D 04 28 00 00 00 05 0C 80 00 00 00
00 00 00 00 68 49 D8 21 00 01 04 00 00 00 35 04 63 12 35 04 32 00 00 00 05 0C 00 00 00 00
00 00 00 00 60 40 E0 20 00 01 04 00 00 00 3D 04 73 12 3D 04 28 00 00 00 05 0C 80 00 00 00
00 00 54 67 20 43 E0 20 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
00 00 72 67 20 43 E0 1F 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
00 00 90 67 20 43 E0 1E 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
FA 66 00 00 20 43 60 20 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
18 67 00 00 20 43 60 1F 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
36 67 00 00 20 43 60 1E 80 01 06 00 00 00 00 00 5F 07 27 02 32 00 00 00 05 13 40 00 00 00
00 00 00 00 C8 58 08 30 00 01 06 00 00 00 01 00 74 05 E3 00 32 00 00 00 05 16 40 00 00 00
00 00 00 00 A8 59 A8 2F 00 01 06 00 00 00 01 00 74 05 E3 00 32 00 00 00 05 16 40 00 00 00
00 00 00 00 60 59 00 30 00 01 06 00 00 00 02 00 79 05 E6 00 32 00 00 00 05 16 40 00 00 00
00 00 44 68 8C 59 34 2E 00 01 06 00 00 00 02 00 79 05 E6 00 32 00 00 00 05 16 40 00 00 00
00 00 00 00 04 58 0C 2E 00 01 06 00 00 00 03 00 81 05 E9 00 32 00 00 00 05 16 40 00 00 00
08 68 00 00 B8 59 68 2E 00 01 06 00 00 00 03 00 81 05 E9 00 32 00 00 00 05 16 40 00 00 00
00 00 00 00 50 5A 70 2F 00 01 06 00 00 00 03 00 81 05 E9 00 32 00 00 00 05 16 40 00 00 00
00 00 9E 68 58 5C 68 27 00 01 06 00 00 00 01 00 74 05 E3 00 32 00 00 00 05 16 40 00 00 00
80 68 00 00 D8 5C 58 27 00 01 06 00 00 00 02 00 79 05 E6 00 32 00 00 00 05 16 40 00 00 00
00 00 DA 68 0C 5B 04 27 00 01 06 00 00 00 02 00 79 05 E6 00 32 00 00 00 05 16 40 00 00 00
BC 68 00 00 F0 5B B0 27 00 01 06 00 00 00 03 00 81 05 E9 00 32 00 00 00 05 16 40 00 00 00
00 00 00 00 B8 3E 78 21 80 02 06 00 00 00 00 00 6F 0A 28 02 32 00 00 00 05 12 00 00 00 00
00 00 00 00 38 3F 78 21 80 02 06 00 00 00 00 00 5E 07 26 02 32 00 00 00 05 13 00 00 00 00

*/




    } LEVELDATA_STATICS;        // total: 30 bytes

    typedef struct {
        uint8 unkn10[36];
    } LEVELDATA_UNKN06;         // total: 36 bytes

    typedef struct {
        uint8 unkn10[12];
    } LEVELDATA_UNKN07;         // total: 12 bytes

    typedef struct {
        uint8 unkn10[8];
    } LEVELDATA_UNKN08;         // total: 8 bytes

    typedef struct {
        uint8 unkn10[437];
    } LEVELDATA_UNKN09;         // total: 437 bytes

    typedef struct {
        uint8 unkn10[11];
        uint8 map[2];
        uint8 minx[2];
        uint8 miny[2];
        uint8 maxx[2];
        uint8 maxy[2];
        uint8 zero[4];
        uint8 objective[2];
        uint8 objective_data[2];
        uint8 unkn11[14 * 9 - 11 - 18];
    } LEVELDATA_UNKN10;         // total: 126 bytes

    // Agents table?
    typedef struct {
        uint8 unkn10[15];
    } LEVELDATA_UNKN11;         // total: 15 bytes

    typedef struct {
        /*      0 */ LEVELDATA_UNKN01 u01;
        /*      8 */ LEVELDATA_MAP map;
        /*  32776 */ LEVELDATA_PEOPLE people[256];
        /*  56328 */ LEVELDATA_CARS cars[64];
        /*  59016 */ LEVELDATA_STATICS statics[400];
        /*  71016 */ LEVELDATA_UNKN06 u06[725];
        /*  97116 */ LEVELDATA_UNKN07 u07;
        /*  97128 */ LEVELDATA_UNKN08 u08[2048];
        /* 113512 */ LEVELDATA_UNKN09 u09;
        /* 113949 */ LEVELDATA_UNKN10 u10;
        /* 114075 */ LEVELDATA_UNKN11 u11[129];
        /* 116010 */
    } LEVELDATA;

    // map-tile surfaces
    surfaceDesc *mtsurfaces_;
    // map-directions points
    floodPointDesc *mdpoints_;
    // for copy in pathfinding
    floodPointDesc *mdpoints_cp_;
    int mmax_x_, mmax_y_, mmax_z_;
    int mmax_m_all, mmax_m_xy;

  protected:
    LEVELDATA level_data_;
    bool sWalkable(char thisTile, char upperTile);
    bool isSurface(char thisTile);
    bool isStairs(char thisTile);

    // eventually all this level data will be replaced by objects like this:
    std::vector<VehicleInstance *> vehicles_;
    std::vector<PedInstance *> peds_;
    std::vector<WeaponInstance *> weapons_;
    std::vector<Static *> statics_;

    int info_costs_[3];
    int enhance_costs_[3];
    std::string briefing_;
    int map_, min_x_, min_y_, max_x_, max_y_;
    int objective_;
    int objective_ped_;
    int objective_vehicle_;

    std::set<int> fast_vehicle_cache_, fast_ped_cache_, fast_weapon_cache_,
            fast_statics_cache_;
};

#endif
