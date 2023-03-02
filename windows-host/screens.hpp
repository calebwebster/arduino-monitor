#include "json-parser/json.h"
#include <stdio.h>
#include <stdlib.h>

const int SCREEN_TEXT_LENGTH = 63;
const char BLANK_SCREEN[] = "SCN                                                            ";
const char SCROLL_TEXT_LENGTH = 63;

bool createScreen1(char *screen, int screenTextLength, json_value *jsonData);
bool createScreen2(char *screen, int screenTextLength, json_value *jsonData);