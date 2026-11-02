#include <cstdlib>
#include <iostream>
#include <string>

#include "api.hpp"
#include "http_server.hpp"

namespace {

std::string envOr(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v && *v) ? std::string(v) : fallback;
}

std::vector<std::string> splitCommas(const std::string& s) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t c = s.find(',', pos);
        std::string item = s.substr(pos, c == std::string::npos ? std::string::npos : c - pos);
        size_t b = item.find_first_not_of(" \t");
        size_t e = item.find_last_not_of(" \t");
        if (b != std::string::npos) out.push_back(item.substr(b, e - b + 1));
        if (c == std::string::npos) break;
        pos = c + 1;
    }
    return out;
}

void printUsage() {
    std::cout <<
        "Smart Hospital REST API server\n"
        "Usage: HospitalServer [--port N] [--host IPv4] [--data-dir DIR] [--allow-origin URL]...\n"
        "\n"
        "  --port N           Listen port (default 8080, env HOSPITAL_PORT, then PORT)\n"
        "  --host IPv4        Listen address (default 127.0.0.1, env HOSPITAL_HOST)\n"
        "  --data-dir DIR     Seed data directory (default data/, env HOSPITAL_DATA_DIR)\n"
        "  --allow-origin URL Allowed browser origin for CORS; repeatable.\n"
        "                     env HOSPITAL_ALLOWED_ORIGINS (comma separated)\n"
        "  --no-reset         Disable POST /api/admin/reset (env HOSPITAL_ALLOW_RESET=0)\n"
        "  --help             Show this help\n";
}

} // namespace

int main(int argc, char** argv) {
    int port = std::atoi(envOr("HOSPITAL_PORT", envOr("PORT", "8080")).c_str());
    std::string host = envOr("HOSPITAL_HOST", "127.0.0.1");

    ApiConfig config;
    config.dataDir = envOr("HOSPITAL_DATA_DIR", "data/");
    config.allowedOrigins = splitCommas(envOr("HOSPITAL_ALLOWED_ORIGINS",
        "http://localhost:8443,http://127.0.0.1:8443,http://localhost:5173,http://127.0.0.1:5173"));
    config.allowReset = envOr("HOSPITAL_ALLOW_RESET", "1") != "0";

    bool userOrigins = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto needValue = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "[ERROR] " << flag << " requires a value.\n";
                std::exit(1);
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") {
            printUsage();
            return 0;
        } else if (a == "--port") {
            port = std::atoi(needValue("--port").c_str());
        } else if (a == "--host") {
            host = needValue("--host");
        } else if (a == "--data-dir") {
            config.dataDir = needValue("--data-dir");
        } else if (a == "--allow-origin") {
            if (!userOrigins) config.allowedOrigins.clear();
            userOrigins = true;
            config.allowedOrigins.push_back(needValue("--allow-origin"));
        } else if (a == "--no-reset") {
            config.allowReset = false;
        } else {
            std::cerr << "[ERROR] Unknown argument: " << a << "\n";
            printUsage();
            return 1;
        }
    }

    if (port < 1 || port > 65535) {
        std::cerr << "[ERROR] Port must be between 1 and 65535.\n";
        return 1;
    }
    if (!config.dataDir.empty() && config.dataDir.back() != '/' && config.dataDir.back() != '\\') {
        config.dataDir += '/';
    }

    HospitalApi api(config);
    std::string err;
    if (!api.initialize(err)) {
        std::cerr << "[ERROR] Could not load seed data from '" << config.dataDir << "': " << err << "\n";
        return 1;
    }

    HttpServer server(host, port, [&api](const HttpRequest& req) { return api.handle(req); });
    if (!server.start(err)) {
        std::cerr << "[ERROR] " << err << "\n";
        return 1;
    }

    std::cout << "[READY] Smart Hospital API listening on http://" << host << ":" << port << "/api\n";
    std::cout << "        Seed data: " << config.dataDir << "\n";
    std::cout << "        Allowed origins:";
    for (const std::string& o : config.allowedOrigins) std::cout << " " << o;
    std::cout << "\n        Press Ctrl+C to stop.\n" << std::flush;

    server.run();
    std::cout << "[STOPPED]\n";
    return 0;
}
