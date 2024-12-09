/*
 * Copyright (c) 2015 Jon Escombe (jone@dresco.co.uk)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_RemoteControl_h
#define _GC_RemoteControl_h 1
#include "GoldenCheetah.h"
#include "Settings.h"
#include "ANT.h"

#include <QList>

// remote control command ids
#define GC_REMOTE_CMD_START     1
#define GC_REMOTE_CMD_STOP      2
#define GC_REMOTE_CMD_LAP       3
#define GC_REMOTE_CMD_HIGHER    4
#define GC_REMOTE_CMD_LOWER     5
#define GC_REMOTE_CMD_CALIBRATE 6

// config strings for ANT remote control commands
#define ANT_CONTROL_GENERIC_MENU_UP      "ant_generic_menu_up"
#define ANT_CONTROL_GENERIC_MENU_DOWN    "ant_generic_menu_down"
#define ANT_CONTROL_GENERIC_MENU_SELECT  "ant_generic_menu_select"
#define ANT_CONTROL_GENERIC_MENU_BACK    "ant_generic_menu_back"
#define ANT_CONTROL_GENERIC_HOME         "ant_generic_home"
#define ANT_CONTROL_GENERIC_START        "ant_generic_start"
#define ANT_CONTROL_GENERIC_STOP         "ant_generic_stop"
#define ANT_CONTROL_GENERIC_RESET        "ant_generic_reset"
#define ANT_CONTROL_GENERIC_LENGTH       "ant_generic_length"
#define ANT_CONTROL_GENERIC_LAP          "ant_generic_lap"
#define ANT_CONTROL_GENERIC_USER_1       "ant_generic_user_1"
#define ANT_CONTROL_GENERIC_USER_2       "ant_generic_user_2"
#define ANT_CONTROL_GENERIC_USER_3       "ant_generic_user_3"

class RemoteCmd
{
    private:
        int     cmdId;
        QString cmdStr;
        QString displayStr;

    public:
        void    setCmdId(int);
        int     getCmdId(void) const;
        void    setCmdStr(QString);
        QString getCmdStr(void) const;
        void    setDisplayStr(QString);
        QString getDisplayStr(void) const;
};

class CmdMap
{
    private:
        int     nativeCmdId;
        int     antCmdId;

    public:
        CmdMap();
        void    setNativeCmdId(int);
        int     getNativeCmdId(void) const;
        void    setAntCmdId(int);
        int     getAntCmdId(void) const;
};

class RemoteControl
{
    Q_DECLARE_TR_FUNCTIONS(RemoteControl)

    private:
        QList<CmdMap>    _cmdMaps;
        QList<RemoteCmd> _antCmdList;
        QList<RemoteCmd> _nativeCmdList;

    public:
        RemoteControl();
        void             writeConfig(QList<CmdMap>) const;
        QList<CmdMap>    readConfig();
        QList<CmdMap>    getMappings() const   { return _cmdMaps; }
        QList<RemoteCmd> getAntCmds() const    { return _antCmdList; }
        QList<RemoteCmd> getNativeCmds() const { return _nativeCmdList; }

        QString          getCmdStr(int, QList<RemoteCmd>) const;  // return the matching command string for an id
        int              getNativeCmdId(int) const;               // return the matching native command id for an ant id
};

#endif // _GC_RemoteControl_h
