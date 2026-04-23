#include <chrono>
#include <cmath>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>

using namespace std;

int envToInt(const char *name, int fallback)
{
    const char *value = getenv(name);
    if (value == nullptr)
    {
        return fallback;
    }

    try
    {
        return stoi(value);
    }
    catch (...)
    {
        return fallback;
    }
}

string envToString(const char *name, const string &fallback)
{
    const char *value = getenv(name);
    if (value == nullptr)
    {
        return fallback;
    }
    return string(value);
}

bool postReading(CURL *curl, const string &url, const string &payload)
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
        cerr << "cpp_sensor request failed: " << curl_easy_strerror(result) << endl;
        return false;
    }

    if (responseCode < 200 || responseCode >= 300)
    {
        cerr << "cpp_sensor server returned status " << responseCode << endl;
        return false;
    }

    return true;
}

int main()
{
    string apiBase = envToString("API_BASE_URL", "http://api:8000");
    int nodeId = envToInt("SENSOR_NODE_ID", 1);
    int intervalSeconds = envToInt("SEND_INTERVAL_SECONDS", 8);
    string readingsUrl = apiBase + "/readings/";

    mt19937 rng(random_device{}());
    uniform_real_distribution<double> voltageDist(98.0, 235.0);
    uniform_real_distribution<double> loadDist(120.0, 820.0);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (curl == nullptr)
    {
        cerr << "cpp_sensor failed to initialize curl" << endl;
        return 1;
    }

    cout << "cpp_sensor started. Sending readings to " << readingsUrl << endl;

    while (true)
    {
        double voltage = round(voltageDist(rng) * 100.0) / 100.0;
        double load = round(loadDist(rng) * 100.0) / 100.0;

        // handwritten payload so it's obvious how each field maps to the API schema
        string payload =
            "{\"node_id\":" + to_string(nodeId) +
            ",\"voltage\":" + to_string(voltage) +
            ",\"load\":" + std::to_string(load) + "}";

        if (postReading(curl, readingsUrl, payload))
        {
            cout << "cpp_sensor sent reading node=" << nodeId
                 << " voltage=" << voltage
                 << " load=" << load << endl;
        }

        this_thread::sleep_for(chrono::seconds(intervalSeconds));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
