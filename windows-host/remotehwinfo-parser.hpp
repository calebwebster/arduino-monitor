#include "json-parser/json.h"
#include "json-parser/json.h"
#include <stdio.h>
#include <stdlib.h>

double getHwinfoSensorValue(json_value *hwinfo, const char *entryName, const char *groupName);
double getAfterburnerSensorValue(json_value *afterburner, const char *name);
bool jsonValueHasType(json_value *jsonValue, json_type jsonType);
double whicheverIsLower(double n1, double n2);
json_value *getValueOfKey(json_value *jsonObject, const char *key);
json_value *getValueOfKeyIfHasType(json_value *jsonObject, const char *key, json_type type);