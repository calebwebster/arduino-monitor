#include "ArduSerial/ArduSerial.h"
#include "ArduSerial/pch.h"
#include "json-parser/json.h"
#include "screens.hpp"
#include <conio.h>
#include <curl/curl.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

char jsonDataBuffer[135000];
int whichScreen = 1;
int screenCounter = 0;

struct HTTPResponse {
    char *buffer;
    CURLcode code;
    size_t size;
    char error[CURL_ERROR_SIZE];
};

static size_t writeCallback(char *contents, size_t chunkSize, size_t chunksCount,
                            void *writeDestination) {
    size_t totalSize = chunkSize * chunksCount;
    struct HTTPResponse *response = (struct HTTPResponse *)writeDestination;

    void *largerBuffer = realloc(response->buffer, response->size + totalSize + 1);
    if (largerBuffer == NULL) {
        printf("ERROR: could not allocate enough memory for remotehwinfo response");
        return 0;
    }

    response->buffer = (char *)largerBuffer;
    memcpy(&(response->buffer[response->size]), contents, totalSize);
    response->size += totalSize;
    response->buffer[response->size] = '\0';

    return totalSize;
}

bool requestRemoteHwinfoData(uint16_t remoteHwinfoPort, struct HTTPResponse *response) {
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return false;
    }
    char url[32 + 1];
    char urlFormat[] = "http://localhost:%d/json.json";
    snprintf(url, sizeof(url), urlFormat, remoteHwinfoPort);
    CURLcode result;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, response->error);
    result = curl_easy_perform(curl);
    response->code = result;
    if (strlen(response->error) == 0) {
        strncpy(response->error, curl_easy_strerror(response->code), CURL_ERROR_SIZE);
        response->error[CURL_ERROR_SIZE] = 0;
    }
    curl_easy_cleanup(curl);
    return true;
}

void printArduinoOutput() {
    printf("\n");
    printf("\033[0;32m");
    while (Serial.available())
        printf("%c", Serial.read());
    printf("\033[0m\n");
}

void changeScreen() { ++whichScreen %= 2; }

void updateArduino() {
    if (!Serial.connected())
        return;
    printArduinoOutput();
    if (screenCounter == 4) {
        screenCounter = 0;
        changeScreen();
        bool success = Serial.print(BLANK_SCREEN);
        if (!success) {
            Serial.end();
            return;
        }
        Sleep(300);
    }

    clock_t t1 = clock();

    struct HTTPResponse response;
    response.buffer = (char *)malloc(0);
    response.buffer[0] = 0;
    response.size = 0;
    response.code = CURLE_OK;
    response.error[0] = 0;

    bool success = requestRemoteHwinfoData(27008, &response);
    if (!success) {
        fprintf(stderr, "ERROR: Failed to invoke curl");
        free(response.buffer);
        return;
    }
    if (response.code != CURLE_OK) {
        fprintf(stderr, "ERROR: %s\n", response.error);
        free(response.buffer);
        return;
    }

    json_value *jsonObject = json_parse((json_char *)response.buffer, response.size);
    free(response.buffer);
    if (jsonObject == NULL) {
        fprintf(stderr, "ERROR: Failed to parse JSON response\n");
        return;
    }

    char screen[SCREEN_TEXT_LENGTH + 1];
    char errorMessage[33 + 1];
    strncpy(errorMessage, "Happy gaming!", sizeof(errorMessage));

    switch (whichScreen) {
    case 0:
        success =
            createScreen1(screen, sizeof(screen), jsonObject, errorMessage, sizeof(errorMessage));
        break;
    case 1:
        success =
            createScreen2(screen, sizeof(screen), jsonObject, errorMessage, sizeof(errorMessage));
        break;
    default:
        break;
    }

    char scrollText[SCROLL_TEXT_LENGTH + 1];
    char timeString[5 + 1];
    time_t t = time(NULL);
    struct tm *time = localtime(&t);
    if (time == NULL) {
        time->tm_hour = 0;
        time->tm_min = 0;
    }
    char greeting[15 + 1];
    if (time->tm_hour < 12)
        strncpy(greeting, "morning", sizeof(greeting));
    else if (time->tm_hour < 18)
        strncpy(greeting, "afternoon", sizeof(greeting));
    else
        strncpy(greeting, "evening", sizeof(greeting));
    snprintf(scrollText, sizeof(scrollText), "SCLGood %s! It is %02d:%02d  |  %s", greeting,
             time->tm_hour, time->tm_min, errorMessage);

    if (strlen(scrollText) < SCROLL_TEXT_LENGTH)
        strncat(scrollText, "\n", 1);

    while (strlen(scrollText) < SCROLL_TEXT_LENGTH)
        strncat(scrollText, " ", 1);

    json_value_free(jsonObject);
    printf("%s\n", screen);
    if (Serial.print(screen) == false) {
        Serial.end();
        return;
    }
    screenCounter++;
    clock_t t2 = clock();
    printf("Completed in %fs\n", ((double)(t2 - t1) / CLOCKS_PER_SEC));
    Sleep(500);
    if (Serial.print(scrollText) == false) {
        Serial.end();
        return;
    }
    Sleep(500);
}

int main(int argc, char **argv) {
    if (argc > 3 || argc == 0) {
        fprintf(stderr, "Usage: windows_host.exe arduino_com_port remotehwinfo_port");
    }
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != 0) {
        fprintf(stderr, "ERROR: Curl failed to initialize: code %d", code);
        return 1;
    }
    Serial.begin(9600, 5);
    while (!kbhit()) {
        updateArduino();
    }
    Serial.end();
    curl_global_cleanup();
    return 0;
}