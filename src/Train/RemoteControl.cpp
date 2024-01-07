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

#include "RemoteControl.h"

void RemoteCmd::setCmdId(int id)
{
    cmdId = id;
}

int RemoteCmd::getCmdId()
{
    return cmdId;
}

void RemoteCmd::setCmdStr(QString string)
{
    cmdStr = string;
}

QString RemoteCmd::getCmdStr()
{
    return cmdStr;
}

void RemoteCmd::setDisplayStr(QString string)
{
    displayStr = string;
}

QString RemoteCmd::getDisplayStr()
{
    return displayStr;
}

CmdMap::CmdMap()
{
    nativeCmdId = antCmdId = 0;
}

void CmdMap::setNativeCmdId(int id)
{
    nativeCmdId = id;
}

int CmdMap::getNativeCmdId()
{
    return nativeCmdId;
}

void CmdMap::setAntCmdId(int id)
{
    antCmdId = id;
}

int CmdMap::getAntCmdId()
{
    return antCmdId;
}

RemoteControl::RemoteControl()
{
    //qDebug() << "RemoteControl::RemoteControl()";

    RemoteCmd remoteCmd;

    //
    // Build list of native command ids and config strings
    //
    _nativeCmdList.clear();

    remoteCmd.setCmdId(GC_REMOTE_CMD_START);
    remoteCmd.setCmdStr(GC_REMOTE_START);
    remoteCmd.setDisplayStr(tr("Start"));
    _nativeCmdList.append(remoteCmd);

    remoteCmd.setCmdId(GC_REMOTE_CMD_STOP);
    remoteCmd.setCmdStr(GC_REMOTE_STOP);
    remoteCmd.setDisplayStr(tr("Stop"));
    _nativeCmdList.append(remoteCmd);

    remoteCmd.setCmdId(GC_REMOTE_CMD_LAP);
    remoteCmd.setCmdStr(GC_REMOTE_LAP);
    remoteCmd.setDisplayStr(tr("Lap"));
    _nativeCmdList.append(remoteCmd);

    remoteCmd.setCmdId(GC_REMOTE_CMD_HIGHER);
    remoteCmd.setCmdStr(GC_REMOTE_HIGHER);
    remoteCmd.setDisplayStr(tr("Higher"));
    _nativeCmdList.append(remoteCmd);

    remoteCmd.setCmdId(GC_REMOTE_CMD_LOWER);
    remoteCmd.setCmdStr(GC_REMOTE_LOWER);
    remoteCmd.setDisplayStr(tr("Lower"));
    _nativeCmdList.append(remoteCmd);

    remoteCmd.setCmdId(GC_REMOTE_CMD_CALIBRATE);
    remoteCmd.setCmdStr(GC_REMOTE_CALIBRATE);
    remoteCmd.setDisplayStr(tr("Calibrate"));
    _nativeCmdList.append(remoteCmd);

    //
    // build list of ANT command id's and config strings
    //
    _antCmdList.clear();

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_MENU_UP);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_MENU_UP);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_MENU_DOWN);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_MENU_DOWN);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_MENU_SELECT);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_MENU_SELECT);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_MENU_BACK);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_MENU_BACK);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_HOME);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_HOME);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_START);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_START);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_STOP);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_STOP);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_RESET);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_RESET);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_LENGTH);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_LENGTH);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_LAP);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_LAP);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_USER_1);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_USER_1);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_USER_2);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_USER_2);
    _antCmdList.append(remoteCmd);

    remoteCmd.setCmdId(ANT_CONTROL_GENERIC_CMD_USER_3);
    remoteCmd.setCmdStr(ANT_CONTROL_GENERIC_USER_3);
    _antCmdList.append(remoteCmd);

    //
    // build list of mappings
    //
    _cmdMaps.clear();

    QList<RemoteCmd> nativeCmds = getNativeCmds();
    foreach (RemoteCmd cmd, nativeCmds) {

        CmdMap cmdMap;
        cmdMap.setNativeCmdId(cmd.getCmdId());
        cmdMap.setAntCmdId(0xFFFF); // initially set to 'no command'

        //qDebug() << "Appending:" << cmd.getCmdId() << cmd.getCmdStr();
        _cmdMaps.append(cmdMap);
    }

    // apply command mappings from the config file
    readConfig();
}

QList<CmdMap>
RemoteControl::readConfig()
{
    //qDebug() << "RemoteControl::ReadConfig()";

    // todo: what to do with empty config, apply defaults?

    QString queryStr;
    QList<RemoteCmd> antCmds = getAntCmds();
    QList<RemoteCmd> nativeCmds = getNativeCmds();

    // populate mappings from config file
    // iterate over the mappings, setting the ANT command if defined
    for (int i=0; i < _cmdMaps.size(); i++) {

        // look up the native command string for this id
        foreach (RemoteCmd nativeCmd, nativeCmds) {
            if (nativeCmd.getCmdId() == _cmdMaps[i].getNativeCmdId())
                queryStr = nativeCmd.getCmdStr();
        }

        if (!queryStr.isEmpty()) {

            // search for a setting in the config file
            QString resultStr = appsettings->value(NULL, queryStr).toString();

            if (!resultStr.isEmpty()) {

                foreach (RemoteCmd antCmd, antCmds) {

                    // lookup the id for this ANT command string
                    if (resultStr == antCmd.getCmdStr()) {

                        // Populate the assigned ANT command id
                        _cmdMaps[i].setAntCmdId(antCmd.getCmdId());

                        //qDebug() << _cmdMaps[i].getNativeCmdId() << _cmdMaps[i].getAntCmdId();
                    }
                }
            }
        }
    }
    return _cmdMaps;
}

void
RemoteControl::writeConfig(QList<CmdMap> mappings)
{
    QString key, value;

    // save current mappings to config file
    for (int i=0; i < mappings.size(); i++) {

        key = getCmdStr(mappings[i].getNativeCmdId(), _nativeCmdList);
        value = getCmdStr(mappings[i].getAntCmdId(), _antCmdList);

        //qDebug() << mappings[i].nativeCmdId << mappings[i].antCmdId;
        //qDebug() << key << value;

        appsettings->setValue(key,value);
    }
}

QString
RemoteControl::getCmdStr(int index, QList<RemoteCmd> cmdList)
{
    foreach(RemoteCmd cmd, cmdList) {
        if (cmd.getCmdId() == index)
            return cmd.getCmdStr();
    }
    return NULL;
}

int
RemoteControl::getNativeCmdId(int antCmd)
{
    foreach(CmdMap map, _cmdMaps) {
        if (map.getAntCmdId() == antCmd) {
            return map.getNativeCmdId();
        }
    }
    return 0;
}
