#include <chrono>
#include <cmath>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>

int envToInt(const char *name, int fallback)
{
    const char *value = std::getenv(name);
    if (value == nullptr)
    {
        return fallback;
    }

    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return fallback;
    }
}

std::string envToString(const char *name, const std::string &fallback)
{
    const char *value = std::getenv(name);
    if (value == nullptr)
    {
        return fallback;
    }
    return std::string(value);
}

bool postReading(CURL *curl, const std::string &url, const std::string &payload)
{
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode result = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    curl_slist_free_all(headers);

    if (result != CURLE_OK)
    {
        std::cerr << "cpp_sensor request failed: " << curl_easy_strerror(result) << std::endl;
        return false;
    }

    if (responseCode < 200 || responseCode >= 300)
    {
        std::cerr << "cpp_sensor server returned status " << responseCode << std::endl;
        return false;
    }

    return true;
}

int main()
{
    std::string apiBase = envToString("API_BASE_URL", "http://api:8000");
    int nodeId = envToInt("SENSOR_NODE_ID", 1);
    int intervalSeconds = envToInt("SEND_INTERVAL_SECONDS", 8);
    std::string readingsUrl = apiBase + "/readings/";

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> voltageDist(98.0, 235.0);
    std::uniform_real_distribution<double> loadDist(120.0, 820.0);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (curl == nullptr)
    {
        std::cerr << "cpp_sensor failed to initialize curl" << std::endl;
        return 1;
    }

    std::cout << "cpp_sensor started. Sending readings to " << readingsUrl << std::endl;

    while (true)
    {
        double voltage = std::round(voltageDist(rng) * 100.0) / 100.0;
        double load = std::round(loadDist(rng) * 100.0) / 100.0;

        // We keep this payload hand-written so it is obvious how each field maps to the API schema.
        std::string payload =
            "{\"node_id\":" + std::to_string(nodeId) +
            ",\"voltage\":" + std::to_string(voltage) +
            ",\"load\":" + std::to_string(load) + "}";

        if (postReading(curl, readingsUrl, payload))
        {
            std::cout << "cpp_sensor sent reading node=" << nodeId
                      << " voltage=" << voltage
                      << " load=" << load << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
