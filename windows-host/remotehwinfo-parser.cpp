#include "json-parser/json.h"
#include <stdio.h>
#include <stdlib.h>
#include "remotehwinfo-parser.hpp"

bool jsonValueHasType(json_value *jsonValue, json_type jsonType)
{
    return (jsonValue != NULL && jsonValue->type == jsonType);
}

json_value *getValueOfKey(json_value *jsonObject, const char *key)
{
    json_object_entry *possibleValues = jsonObject->u.object.values;
    for (int i = 0; i < jsonObject->u.object.length; i++)
    {
        if (strcmp(possibleValues[i].name, key) == 0)
            return possibleValues[i].value;
    }
    return NULL;
}

json_value *getValueOfKeyIfHasType(json_value *jsonObject, const char *key, json_type type)
{
    json_value *entry = getValueOfKey(jsonObject, key);
    if (!jsonValueHasType(entry, type))
        return NULL;
    return entry;
}

json_value *getAfterburnerEntries(json_value *afterburner)
{
    return getValueOfKeyIfHasType(afterburner, "entries", json_array);
}

bool afterburnerEntryHasName(json_value *entry, const char *name)
{
    json_value *entryName = getValueOfKeyIfHasType(entry, "name", json_string);
    if (entryName == NULL)
        return false;
    return (strcmp(entryName->u.string.ptr, name) == 0);
}

json_value *getAfterburnerEntry(json_value *afterburner, const char *name)
{
    json_value *afterburnerEntries = getAfterburnerEntries(afterburner);
    if (afterburnerEntries == NULL)
        return NULL;
    for (int i = 0; i < afterburnerEntries->u.array.length; i++)
    {
        json_value *entry = afterburnerEntries->u.array.values[i];
        if (afterburnerEntryHasName(entry, name))
            return entry;
    }
    return NULL;
}

bool doesHwinfoEntryHaveName(json_value *entry, const char *name)
{
    json_value *entryName = getValueOfKeyIfHasType(entry, "labelOriginal", json_string);
    if (entryName == NULL)
        return false;
    return (strcmp(entryName->u.string.ptr, name) == 0);
}

bool doesHwinfoGroupHaveName(json_value *group, const char *name)
{
    json_value *groupName = getValueOfKeyIfHasType(group, "sensorNameOriginal", json_string);
    if (groupName == NULL)
        return false;
    return (strcmp(groupName->u.string.ptr, name) == 0);
}

json_value *getHwinfoGroups(json_value *hwinfo)
{
    return getValueOfKeyIfHasType(hwinfo, "sensors", json_array);
}

json_value *getHwinfoEntries(json_value *hwinfo)
{
    return getValueOfKeyIfHasType(hwinfo, "readings", json_array);
}

json_value *getAfterburnerEntryValue(json_value *entry)
{
    return getValueOfKeyIfHasType(entry, "data", json_double);
}

json_value *getHwinfoEntryValue(json_value *entry)
{
    return getValueOfKeyIfHasType(entry, "value", json_double);
}

bool isHwinfoEntryInGroup(json_value *entry, json_value *group)
{
    json_value *groupNumber = getValueOfKeyIfHasType(entry, "sensorIndex", json_integer);
    if (groupNumber == NULL)
        return false;
    json_value *entryGroupNumber = getValueOfKeyIfHasType(group, "entryIndex", json_integer);
    if (entryGroupNumber == NULL)
        return false;
    return entryGroupNumber->u.integer == entryGroupNumber->u.integer;
}

json_value *getHwinfoEntryByName(json_value *hwinfoEntries, const char *name)
{
    for (int i = 0; i < hwinfoEntries->u.array.length; i++)
    {
        json_value *entry = hwinfoEntries->u.array.values[i];
        if (doesHwinfoEntryHaveName(entry, name))
            return entry;
    }
    return NULL;
}

json_value *getHwinfoGroup(json_value *hwinfo, const char *name)
{
    json_value *groups = getHwinfoGroups(hwinfo);
    if (groups == NULL)
        return NULL;
    for (int i = 0; i < groups->u.array.length; i++)
    {
        json_value *group = groups->u.array.values[i];
        if (doesHwinfoGroupHaveName(group, name))
            return group;
    }
    return NULL;
}

json_value *getHwinfoEntryByNameInGroup(json_value *hwinfo, const char *entryName, const char *groupName)
{
    json_value *hwinfoEntries = getHwinfoEntries(hwinfo);
    if (hwinfoEntries == NULL)
        return NULL;
    json_value *hwinfoGroups = getHwinfoGroups(hwinfo);
    if (hwinfoGroups == NULL)
        return NULL;
    json_value *entry = getHwinfoEntryByName(hwinfoEntries, entryName);
    json_value *group = getHwinfoGroup(hwinfoGroups, groupName);
    if (isHwinfoEntryInGroup(entry, group))
        return entry;
    return NULL;
}

long long getGroupIndex(json_value *group)
{
    json_value *groupIndex = getValueOfKeyIfHasType(group, "entryIndex", json_integer);
    if (groupIndex == NULL)
        return -1;
    return groupIndex->u.integer;
}

json_value *getHwinfoEntryInGroup(json_value *hwinfo, const char *entryName, json_value *group)
{
    json_value *entries = getHwinfoEntries(hwinfo);
    if (entries == NULL)
        return NULL;
    long long groupIndex = getGroupIndex(group);
    if (groupIndex == -1)
        return NULL;
    for (int i = 0; i < entries->u.array.length; i++)
    {
        json_value *entry = entries->u.array.values[i];
        if (doesHwinfoEntryHaveName(entry, entryName))
            return entry;
    }
    return NULL;
}

double getAfterburnerSensorValue(json_value *afterburner, const char *name)
{
    json_value *entry = getAfterburnerEntry(afterburner, name);
    if (entry == NULL)
        return 0;
    json_value *value = getAfterburnerEntryValue(entry);
    if (value == NULL)
        return 0;
    return value->u.dbl;
}

double getHwinfoSensorValue(json_value *hwinfo, const char *entryName, const char *groupName)
{
    json_value *group = getHwinfoGroup(hwinfo, groupName);
    if (group == NULL)
        return 0;
    json_value *entry = getHwinfoEntryInGroup(hwinfo, entryName, group);
    if (entry == NULL)
        return 0;
    json_value *value = getHwinfoEntryValue(entry);
    if (value == NULL)
        return 0;
    return value->u.dbl;
}

double whicheverIsLower(double n1, double n2)
{
    if (n1 > n2)
        return n2;
    return n1;
}
