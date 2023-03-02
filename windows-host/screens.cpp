#include "screens.hpp"
#include "json-parser/json.h"
#include "remotehwinfo-parser.hpp"

const char DEGREE_SYMBOL = 178;

/*
GPU 00° 00% FPS 0000
CPU 00° 00% FAN 00%
CORE 0000   MEM 0000
*/
bool createScreen1(char *screen, int screenLength, json_value *jsonData, char *error,
                   int errorLength) {
    char defaultScreen[] = "SCNGPU --%c --%% FPS ----CPU --%c --%% FAN ---%%RAM -----MB/-----MB ";
    snprintf(screen, screenLength, defaultScreen, DEGREE_SYMBOL, DEGREE_SYMBOL);
    if (!jsonValueHasType(jsonData, json_object)) {
        strncpy(error, "Error: Failed to parse JSON", errorLength);
        return false;
    }
    json_value *afterburner = getValueOfKeyIfHasType(jsonData, "afterburner", json_object);
    json_value *hwinfo = getValueOfKeyIfHasType(jsonData, "hwinfo", json_object);
    if (afterburner == NULL) {
        strncpy(error, "Error: Afterburner is not running", errorLength);
        return false;
    }
    if (hwinfo == NULL) {
        strncpy(error, "Error: HWInfo is not running", errorLength);
        return false;
    }

    double gpuTemp = getAfterburnerSensorValue(afterburner, "GPU temperature");
    double gpuUsage = getAfterburnerSensorValue(afterburner, "GPU usage");
    double framerate = getAfterburnerSensorValue(afterburner, "Framerate");
    double cpuTemp = getAfterburnerSensorValue(afterburner, "CPU temperature");
    double cpuUsage = getAfterburnerSensorValue(afterburner, "CPU usage");
    double fanSpeed = getAfterburnerSensorValue(afterburner, "Fan speed");
    double memoryUsed = getHwinfoSensorValue(hwinfo, "Physical Memory Used", "System");
    double memoryAvailable = getHwinfoSensorValue(hwinfo, "Physical Memory Available", "System");
    double totalMemory = memoryUsed + memoryAvailable;

    gpuTemp = whicheverIsLower(gpuTemp, 99);
    gpuUsage = whicheverIsLower(gpuUsage, 99);
    framerate = whicheverIsLower(framerate, 9999);
    cpuTemp = whicheverIsLower(cpuTemp, 99);
    cpuUsage = whicheverIsLower(cpuUsage, 99);
    fanSpeed = whicheverIsLower(fanSpeed, 999);
    memoryUsed = whicheverIsLower(memoryUsed, 999999);
    totalMemory = whicheverIsLower(totalMemory, 999999);

    char screenFormatting[] =
        "SCNGPU %2.0f%c %2.0f%% FPS %-4.0fCPU %2.0f%c %2.0f%% FAN %3.0f%%RAM %5.0fMB/%5.0fMB ";
    snprintf(screen, screenLength, screenFormatting, gpuTemp, DEGREE_SYMBOL, gpuUsage, framerate,
             cpuTemp, DEGREE_SYMBOL, cpuUsage, fanSpeed, memoryUsed, totalMemory);
    return true;
}

/*
CORE 0000   MEM 0000
PUMP 0000   CPU 0000
UP 00000K  DN 00000K
*/
bool createScreen2(char *screen, int screenTextLength, json_value *jsonData, char *error,
                   int errorLength) {
    char defaultScreen[] = "SCNCORE ----   MEM ----PUMP ----   CPU ----UP -----K  DN -----K";
    snprintf(screen, screenTextLength, defaultScreen);
    json_value *afterburner = getValueOfKeyIfHasType(jsonData, "afterburner", json_object);
    json_value *hwinfo = getValueOfKeyIfHasType(jsonData, "hwinfo", json_object);
    if (afterburner == NULL)
        return false;
    if (hwinfo == NULL)
        return false;
    double coreClock = getAfterburnerSensorValue(afterburner, "Core clock");
    double memoryClock = getAfterburnerSensorValue(afterburner, "Memory clock");
    double pumpSpeed =
        getHwinfoSensorValue(hwinfo, "CPU2", "ASRock X570 Steel Legend (Nuvoton NCT6796D)");
    double cpuClock = getAfterburnerSensorValue(afterburner, "CPU clock");
    double upload = getHwinfoSensorValue(
        hwinfo, "Current UP rate", "Network: Broadcom 802.11ac Wireless PCIE Full Dongle Adapter");
    double download = getHwinfoSensorValue(
        hwinfo, "Current DL rate", "Network: Broadcom 802.11ac Wireless PCIE Full Dongle Adapter");

    coreClock = whicheverIsLower(coreClock, 9999);
    memoryClock = whicheverIsLower(memoryClock, 9999);
    pumpSpeed = whicheverIsLower(pumpSpeed, 9999);
    cpuClock = whicheverIsLower(cpuClock, 9999);
    upload = whicheverIsLower(upload, 99999);
    download = whicheverIsLower(download, 99999);

    char screenFormatting[] = "SCNCORE %4.0f   MEM %4.0fPUMP %4.0f   CPU %4.0fUP %5.0fK  DN %5.0fK";
    snprintf(screen, screenTextLength, screenFormatting, coreClock, memoryClock, pumpSpeed,
             cpuClock, upload, download);
    return true;
}
