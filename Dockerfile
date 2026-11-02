# Builds and runs the C++ REST API (Render "Docker" runtime).
FROM gcc:13 AS build
WORKDIR /src
COPY include ./include
COPY src ./src
COPY server ./server
RUN g++ -std=c++17 -O2 -I include -I server/third_party -I server/src \
      server/src/server_main.cpp server/src/http_server.cpp server/src/api.cpp \
      src/models/Patient.cpp src/models/DispatchResource.cpp src/services/DataLoader.cpp \
      -static-libstdc++ -static-libgcc -o /HospitalServer

FROM debian:bookworm-slim
RUN useradd --system --no-create-home app
WORKDIR /app
COPY --from=build /HospitalServer ./HospitalServer
COPY data ./data
USER app
# Render supplies PORT; the server reads it automatically.
ENV HOSPITAL_HOST=0.0.0.0 HOSPITAL_DATA_DIR=data/ HOSPITAL_ALLOW_RESET=0
EXPOSE 8080
CMD ["./HospitalServer"]
