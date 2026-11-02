#pragma once

#include <memory>
#include <string>
#include <vector>

#include "http_server.hpp"

struct ApiConfig {
    std::string dataDir = "data/";
    std::vector<std::string> allowedOrigins;  // "*" allows any origin
    bool allowReset = true;                   // enables POST /api/admin/reset
};

/**
 * REST adapter over the original HospitalSystem facade.
 *
 * This class contains NO hospital business logic. Every endpoint validates the
 * shape of its input, calls the existing C++ facade, and serialises the result.
 * HospitalSystem remains the single source of truth.
 */
class HospitalApi {
public:
    explicit HospitalApi(ApiConfig config);
    ~HospitalApi();

    HospitalApi(const HospitalApi&) = delete;
    HospitalApi& operator=(const HospitalApi&) = delete;

    /** Loads the seed data into the HospitalSystem. */
    bool initialize(std::string& outError);

    HttpResponse handle(const HttpRequest& request);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
