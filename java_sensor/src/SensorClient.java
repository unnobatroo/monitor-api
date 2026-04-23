import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.Locale;
import java.util.concurrent.ThreadLocalRandom;

public class SensorClient {
    private static final HttpClient HTTP = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(5))
            .build();

    private static int envInt(String name, int fallback) {
        String raw = System.getenv(name);
        if (raw == null || raw.isBlank()) {
            return fallback;
        }

        try {
            return Integer.parseInt(raw);
        } catch (NumberFormatException ex) {
            return fallback;
        }
    }

    private static String envString(String name, String fallback) {
        String raw = System.getenv(name);
        if (raw == null || raw.isBlank()) {
            return fallback;
        }
        return raw;
    }

    private static double randomBetween(double min, double max) {
        return ThreadLocalRandom.current().nextDouble(min, max);
    }

    private static String readingJson(int nodeId, double voltage, double load) {
        // for predictable decimal points
        return String.format(Locale.US,
                "{\"node_id\":%d,\"voltage\":%.2f,\"load\":%.2f}",
                nodeId,
                voltage,
                load);
    }

    private static void sendReading(String readingsUrl, String payload) throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(readingsUrl))
                .timeout(Duration.ofSeconds(10))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(payload))
                .build();

        HttpResponse<String> response = HTTP.send(request, HttpResponse.BodyHandlers.ofString());
        if (response.statusCode() < 200 || response.statusCode() >= 300) {
            throw new IOException("java_sensor received HTTP " + response.statusCode() + " body=" + response.body());
        }
    }

    public static void main(String[] args) {
        String baseUrl = envString("API_BASE_URL", "http://api:8000");
        int nodeId = envInt("SENSOR_NODE_ID", 1);
        int intervalSeconds = envInt("SEND_INTERVAL_SECONDS", 10);
        String readingsUrl = baseUrl + "/readings/";

        System.out.println("java_sensor started! Sending readings to " + readingsUrl);

        while (true) {
            double voltage = randomBetween(100.0, 240.0);
            double load = randomBetween(100.0, 900.0);
            String payload = readingJson(nodeId, voltage, load);

            try {
                sendReading(readingsUrl, payload);
                System.out.println("java_sensor sent reading " + payload);
            } catch (Exception ex) {
                // log and continue so one transient network error doesn't permanently kill the
                // service container
                System.err.println("java_sensor failed to send reading: " + ex.getMessage());
            }

            try {
                Thread.sleep(intervalSeconds * 1000L);
            } catch (InterruptedException ex) {
                Thread.currentThread().interrupt();
                break;
            }
        }
    }
}
