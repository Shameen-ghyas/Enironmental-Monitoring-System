#ifndef WEATHER_H
#define WEATHER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON/cJSON.h"

// String struct to handle API response
struct string {
    char* ptr;
    size_t len;
};

// Function declarations
void init_string(struct string* s);
size_t writefunc(void* ptr, size_t size, size_t nmemb, struct string* s);
void save_to_csv(const char* city, const char* country_code, const char* date, const char* description, double temp_max, double temp_min, double feels_like, int humidity, double wind_speed);
void extract_weather_details(cJSON* weather_json, char* city, char* country_code);
void save_raw_json(const char* json_data);
void fetch_data(char* city, char* country_code, char* api_key);
void calculate_and_display_averages(cJSON* weather_json, int days_count);

#endif // WEATHER_H