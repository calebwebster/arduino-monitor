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
int timesSentScreen = 0;

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

void formatCurrentTimeString(char *timeString, int timeStringLength) {
    time_t t = time(NULL);
    struct tm *localTime = localtime(&t);
    if (localTime == NULL)
        strncpy(timeString, "00:00", timeStringLength);
    strftime(timeString, timeStringLength, "%H:%M", localTime);
}

void updateArduino() {
    if (!Serial.connected())
        return;
    printArduinoOutput();
    if (timesSentScreen == 4) {
        timesSentScreen = 0;
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
    char scrollText[SCROLL_TEXT_LENGTH + 1];
    char timeString[5 + 1];
    formatCurrentTimeString(timeString, sizeof(timeString));
    switch (whichScreen) {
    case 0:
        success = createScreen1(screen, sizeof(screen), jsonObject);
        break;
    case 1:
        success = createScreen2(screen, sizeof(screen), jsonObject);
        break;
    default:
        break;
    }

    json_value_free(jsonObject);
    printf("%s\n", screen);
    if (Serial.print(screen) == false) {
        Serial.end();
        return;
    }
    timesSentScreen++;
    clock_t t2 = clock();
    printf("Completed in %fs\n", ((double)(t2 - t1) / CLOCKS_PER_SEC));
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
        Sleep(1000);
    }
    Serial.end();
    curl_global_cleanup();
    return 0;
}