/*
 * Copyright (c) 2017 Stefan Schake
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
#include "GarminServiceHelper.h"
#include <QDebug>

#if defined(_MSC_VER)
#pragma comment(lib, "advapi32.lib")
#include <windows.h>

static LPCWSTR GARMIN_SERVICE_NAME = L"Garmin Device Interaction Service";

bool GarminServiceHelper::isServiceRunning()
{
    SC_HANDLE manager = OpenSCManager(NULL, NULL, 0);
    if (manager == NULL)
        goto fail;

    SC_HANDLE service = OpenService(manager, GARMIN_SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (service == NULL)
        goto fail;

    SERVICE_STATUS serviceStatus;
    if (QueryServiceStatus(service, &serviceStatus) == 0)
        goto fail;

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return serviceStatus.dwCurrentState == SERVICE_RUNNING
            || serviceStatus.dwCurrentState == SERVICE_START_PENDING;

    fail:
    if (service != NULL)
        CloseServiceHandle(service);
    if (manager != NULL)
        CloseServiceHandle(manager);
    return false;
}

static void restartGarminService()
{
    SC_HANDLE manager = OpenSCManager(NULL, NULL, 0);
    if (manager == NULL)
        goto cleanup;

    SC_HANDLE service = OpenService(manager, GARMIN_SERVICE_NAME, SERVICE_START);
    if (service == NULL)
        goto cleanup;

    // we don't care if this works out
    StartService(service, 0, NULL);

    cleanup:
    if (service != NULL)
        CloseServiceHandle(service);
    if (manager != NULL)
        CloseServiceHandle(manager);
}

bool GarminServiceHelper::stopService()
{
    SC_HANDLE manager = OpenSCManager(NULL, NULL, 0);
    if (manager == NULL)
        goto fail;

    SC_HANDLE service = OpenService(manager, GARMIN_SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == NULL)
        goto fail;

    SERVICE_STATUS serviceStatus;
    // this unfortunately blocks until the service is stopped, but the Garmin service is generally well behaved
    if (ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus) == 0)
        goto fail;

    while (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        Sleep(serviceStatus.dwWaitHint);
        if (QueryServiceStatus(service, &serviceStatus) == 0)
            goto fail;
    }

    bool success = serviceStatus.dwCurrentState == SERVICE_STOPPED;

    if (success)
    {
        atexit(restartGarminService);
    }

    CloseServiceHandle(manager);
    CloseServiceHandle(service);
    // We need to wait some more time to make sure Windows catches up
    Sleep(3000);
    return success;

    fail:
    if (service != NULL)
        CloseServiceHandle(service);
    if (manager != NULL)
        CloseServiceHandle(manager);
    return false;
}

#else

bool GarminServiceHelper::isServiceRunning()
{
    return false;
}

bool GarminServiceHelper::stopService()
{
    return false;
}

#endif
