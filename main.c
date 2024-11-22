#include "weather.h"

// Initialize the string to handle the curl response
void init_string(struct string* s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

// Callback function for curl to write the response into the string
size_t writefunc(void* ptr, size_t size, size_t nmemb, struct string* s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->len = new_len;
    s->ptr[s->len] = '\0';

    return size * nmemb;
}


void send_notification(const char* title, const char* message) {
    char command[512];

    // Format the Zenity command 
    snprintf(command, sizeof(command), "zenity --info --title='%s' --text='%s' 2>/dev/null", title, message);

   
    printf("Executing: %s\n", command);
    fflush(stdout); 

    int result = system(command);


    if (result != 0) {
        fprintf(stderr, "Failed to send notification.\n");
    }
}




// Save parsed data to CSV file
void save_to_csv(const char* city, const char* country_code, const char* date, const char* description, double temp_max, double temp_min, double feels_like, int humidity, double wind_speed) {
    FILE* file = fopen("parsed_data.csv", "a");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        if (file_size == 0) {
            fprintf(file, "City,Country Code,Date,Weather,Max Temperature (°C),Min Temperature (°C),Feels Like (°C),Humidity (%%),Wind Speed (m/s)\n");
        }
        fprintf(file, "%s,%s,%s,%s,%.2lf,%.2lf,%.2lf,%d,%.2lf\n", city, country_code, date, description, temp_max, temp_min, feels_like, humidity, wind_speed);
        fclose(file);
    }
    else {
        fprintf(stderr, "Error opening file for writing\n");
    }

}
void sanitize_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < 32 || str[i] > 126) {
            str[i] = ' ';
        }
    }
}



void calculate_and_display_averages(cJSON* weather_json, int days_count) {
    double total_temp_max = 0, total_temp_min = 0, total_wind_speed = 0;
    int total_humidity = 0;

    cJSON* forecast_array = cJSON_GetObjectItem(weather_json, "forecast");
    cJSON* days_array = cJSON_GetObjectItem(forecast_array, "forecastday");

    if (days_array == NULL) {
        fprintf(stderr, "Error: No forecast days found.\n");
        return;
    }

    for (int i = 0; i < days_count; i++) {
        cJSON* day_data = cJSON_GetArrayItem(days_array, i);
        cJSON* day = cJSON_GetObjectItem(day_data, "day");

        if (day == NULL) {
            fprintf(stderr, "Error: Day data not found for day %d\n", i + 1);
            continue;
        }

        double max_temp = cJSON_GetObjectItem(day, "maxtemp_c") ? cJSON_GetObjectItem(day, "maxtemp_c")->valuedouble : 0;
        double min_temp = cJSON_GetObjectItem(day, "mintemp_c") ? cJSON_GetObjectItem(day, "mintemp_c")->valuedouble : 0;
        double wind_speed = cJSON_GetObjectItem(day, "maxwind_kph") ? cJSON_GetObjectItem(day, "maxwind_kph")->valuedouble : 0;
        int humidity = cJSON_GetObjectItem(day, "avghumidity") ? cJSON_GetObjectItem(day, "avghumidity")->valueint : 0;

        total_temp_max += max_temp;
        total_temp_min += min_temp;
        total_wind_speed += wind_speed;
        total_humidity += humidity;

        printf("Day %d: Max Temp = %.2lf, Min Temp = %.2lf, Wind Speed = %.2lf kph, Humidity = %d\n",
            i + 1, max_temp, min_temp, wind_speed, humidity);
    }

   
    if (days_count == 0) {
        fprintf(stderr, "No days to calculate averages.\n");
        return;
    }

    double avg_temp_max = total_temp_max / days_count;
    double avg_temp_min = total_temp_min / days_count;
    double avg_wind_speed = total_wind_speed / days_count; 
    double avg_humidity = (double)total_humidity / days_count;

   
    printf("\n=======================================\n");
    printf("               AVERAGE WEATHER\n");
    printf("=======================================\n");
    printf("| %-35s | %.2lf °C      |\n", "Average Max Temperature", avg_temp_max);
    printf("| %-35s | %.2lf °C      |\n", "Average Min Temperature", avg_temp_min);
    printf("| %-35s | %.2lf kph     |\n", "Average Wind Speed", avg_wind_speed); 
    printf("| %-35s | %.2lf %%      |\n", "Average Humidity", avg_humidity);
    printf("=======================================\n\n");
    printf("Total Max Temp: %.2lf, Total Wind Speed: %.2lf\n", total_temp_max, total_wind_speed);

    const double temp_threshold = 25.0;  // Adjusted temperature threshold (°C)
    const double wind_speed_threshold = 27.0;  // Adjusted wind speed threshold




    if (avg_temp_max > temp_threshold) {

        char message[256];
        snprintf(message, sizeof(message), "The average temperature has exceeded %.2lf°C. Current: %.2lf°C", temp_threshold, avg_temp_max);

        // Sanitize the message to remove non-printable characters
        sanitize_string(message);

      
        printf("Temperature Alert Message: %s\n", message);
        printf("Message Length: %zu\n", strlen(message));

        // Now send the sanitized message
        send_notification("Temperature Alert", message);
    }

    if (avg_wind_speed > wind_speed_threshold) {
        char message[256];
        snprintf(message, sizeof(message), "The average wind speed has exceeded %.2lf m/s. Current: %.2lf m/s", wind_speed_threshold, avg_wind_speed);
        send_notification("Wind Speed Alert", message);
    }
}



// Extract and display weather details for the forecast
void extract_weather_details(cJSON* weather_json, char* city, char* country_code) {
    cJSON* forecast_array = cJSON_GetObjectItem(weather_json, "forecast");
    if (!forecast_array) {
        fprintf(stderr, "Error: Forecast data not found in JSON.\n");
        return;
    }

    cJSON* days_array = cJSON_GetObjectItem(forecast_array, "forecastday");
    if (!days_array) {
        fprintf(stderr, "Error: Forecast day data not found in JSON.\n");
        return;
    }

    int days_count = cJSON_GetArraySize(days_array);
    for (int i = 0; i < days_count; i++) {
        cJSON* day_data = cJSON_GetArrayItem(days_array, i);
        const char* date = cJSON_GetObjectItem(day_data, "date")->valuestring;
        cJSON* day = cJSON_GetObjectItem(day_data, "day");
        cJSON* condition = cJSON_GetObjectItem(day, "condition");

        const char* description = cJSON_GetObjectItem(condition, "text")->valuestring;
        double temp_max = cJSON_GetObjectItem(day, "maxtemp_c")->valuedouble;
        double temp_min = cJSON_GetObjectItem(day, "mintemp_c")->valuedouble;
        double feels_like = cJSON_GetObjectItem(day, "avgtemp_c")->valuedouble;
        int humidity = cJSON_GetObjectItem(day, "avghumidity")->valueint;
        double wind_speed = cJSON_GetObjectItem(day, "maxwind_kph")->valuedouble / 3.6;

        printf("Date: %s\n", date);
        printf("City: %s, Country Code: %s\n", city, country_code);
        printf("Weather: %s\n", description);
        printf("Max Temperature: %.2lf°C\n", temp_max);
        printf("Min Temperature: %.2lf°C\n", temp_min);
        printf("Feels Like: %.2lf°C\n", feels_like);
        printf("Humidity: %d%%\n", humidity);
        printf("Wind Speed: %.2lf m/s\n\n", wind_speed);

        save_to_csv(city, country_code, date, description, temp_max, temp_min, feels_like, humidity, wind_speed);
    }

    // Call function to calculate and display averages
    calculate_and_display_averages(weather_json, days_count);
}

// Save raw JSON response to file
void save_raw_json(const char* json_data) {
    FILE* file = fopen("unparsed.json", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", json_data);
        fclose(file);
        printf("Raw JSON data saved to unparsed.json\n");
    }
}

// Fetch weather data using the WeatherAPI
void fetch_data(char* city, char* country_code, char* api_key) {
    CURL* curl;
    CURLcode res;
    struct string s;
    char url[256];

    snprintf(url, sizeof(url), "https://api.weatherapi.com/v1/forecast.json?key=%s&q=%s&days=7", api_key, city);

    init_string(&s);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            save_raw_json(s.ptr);

            cJSON* weather_json = cJSON_Parse(s.ptr);
            if (weather_json == NULL) {
                fprintf(stderr, "JSON parsing error\n");
            }
            else {
                char view_forecast;
                printf("Do you want to see the forecast? (y/n): ");
                scanf(" %c", &view_forecast);

                if (view_forecast == 'y' || view_forecast == 'Y') {
                    printf("Displaying forecast...\n");
                    extract_weather_details(weather_json, city, country_code);
                }
                else {
                    printf("Forecast display skipped.\n");
                }

                cJSON_Delete(weather_json);
            }
        }
        curl_easy_cleanup(curl);
        free(s.ptr);
    }
}

int main(void) {
    char city[100];
    char country_code[3];
    char api_key[] = "0d02b9c3a02b45d4a91165418242110"; 

    printf("Enter city name: ");
    fgets(city, sizeof(city), stdin);
    city[strcspn(city, "\n")] = 0;

    printf("Enter country code: ");
    fgets(country_code, sizeof(country_code), stdin);
    country_code[strcspn(country_code, "\n")] = 0;
  

    fetch_data(city, country_code, api_key);

    return 0;
}