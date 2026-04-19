FROM eclipse-temurin:21-jdk-jammy

WORKDIR /sensor

COPY src/SensorClient.java ./SensorClient.java
RUN javac SensorClient.java

CMD ["java", "SensorClient"]
