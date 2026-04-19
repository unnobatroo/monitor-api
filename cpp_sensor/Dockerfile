FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ libcurl4-openssl-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /sensor

COPY src/main.cpp ./src/main.cpp
RUN g++ -std=c++17 -O2 -Wall -Wextra -o cpp_sensor src/main.cpp -lcurl

CMD ["./cpp_sensor"]
