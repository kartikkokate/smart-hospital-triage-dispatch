#include "api.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <set>

#include "json.hpp"
#include "services/HospitalSystem.hpp"
#include "services/ReportingService.hpp"

using json = nlohmann::json;

namespace {

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

struct ApiError {
    int status;
    std::string message;
};

[[noreturn]] void fail(int status, const std::string& message) {
    throw ApiError{status, message};
}

/** Translates a facade rejection (outError) into an HTTP error. */
[[noreturn]] void domainFail(const std::string& message) {
    fail(message.find("not found") != std::string::npos ? 404 : 422, message);
}

const char* errorCode(int status) {
    switch (status) {
        case 400: return "BAD_REQUEST";
        case 403: return "FORBIDDEN";
        case 404: return "NOT_FOUND";
        case 405: return "METHOD_NOT_ALLOWED";
        case 422: return "REJECTED";
        default:  return "ERROR";
    }
}

// ---------------------------------------------------------------------------
// Request helpers (strict, typed input validation)
// ---------------------------------------------------------------------------

constexpr size_t kMaxIdLength = 64;

struct Ctx {
    const HttpRequest& req;
    std::vector<std::string> params;
    int status = 200;
    mutable json parsedBody;
    mutable bool bodyParsed = false;

    explicit Ctx(const HttpRequest& r) : req(r) {}

    const json& body() const {
        if (!bodyParsed) {
            json parsed = json::parse(req.body, nullptr, false);
            if (parsed.is_discarded() || !parsed.is_object()) {
                fail(400, "Request body must be a JSON object.");
            }
            parsedBody = std::move(parsed);
            bodyParsed = true;
        }
        return parsedBody;
    }
};

std::string bodyString(const json& b, const char* key, size_t maxLen) {
    auto it = b.find(key);
    if (it == b.end() || !it->is_string()) fail(400, std::string("Field '") + key + "' is required and must be a string.");
    std::string s = it->get<std::string>();
    if (s.size() > maxLen) fail(400, std::string("Field '") + key + "' is too long.");
    return s;
}

int bodyInt(const json& b, const char* key, long long lo, long long hi) {
    auto it = b.find(key);
    if (it == b.end() || !it->is_number_integer()) fail(400, std::string("Field '") + key + "' is required and must be an integer.");
    long long v;
    if (it->is_number_unsigned()) {
        unsigned long long u = it->get<unsigned long long>();
        if (u > static_cast<unsigned long long>(hi)) fail(400, std::string("Field '") + key + "' is out of range.");
        v = static_cast<long long>(u);
    } else {
        v = it->get<long long>();
    }
    if (v < lo || v > hi) {
        fail(400, std::string("Field '") + key + "' must be between " + std::to_string(lo) + " and " + std::to_string(hi) + ".");
    }
    return static_cast<int>(v);
}

double bodyNumber(const json& b, const char* key) {
    auto it = b.find(key);
    if (it == b.end() || !it->is_number()) fail(400, std::string("Field '") + key + "' is required and must be a number.");
    return it->get<double>();
}

bool parseIntStrict(const std::string& s, long long lo, long long hi, long long& out) {
    if (s.empty() || s.size() > 10) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size()) return false;
    for (size_t k = i; k < s.size(); ++k) {
        if (s[k] < '0' || s[k] > '9') return false;
    }
    out = std::strtoll(s.c_str(), nullptr, 10);
    return out >= lo && out <= hi;
}

int queryInt(const Ctx& c, const char* key, long long lo, long long hi) {
    auto it = c.req.query.find(key);
    if (it == c.req.query.end()) fail(400, std::string("Query parameter '") + key + "' is required.");
    long long v;
    if (!parseIntStrict(it->second, lo, hi, v)) {
        fail(400, std::string("Query parameter '") + key + "' must be an integer between " + std::to_string(lo) + " and " + std::to_string(hi) + ".");
    }
    return static_cast<int>(v);
}

std::string queryString(const Ctx& c, const char* key) {
    auto it = c.req.query.find(key);
    return it == c.req.query.end() ? std::string() : it->second;
}

const std::string& pathId(const Ctx& c, size_t index) {
    const std::string& id = c.params.at(index);
    if (id.empty() || id.size() > kMaxIdLength) fail(400, "Invalid identifier in URL.");
    return id;
}

/** Validates 1..5 before converting: intToUrgency() silently maps invalid values to NON_URGENT. */
UrgencyLevel urgencyFromBody(const json& b, const char* key = "urgency") {
    return intToUrgency(bodyInt(b, key, 1, 5));
}

// ---------------------------------------------------------------------------
// Serialisation of existing C++ model types
// ---------------------------------------------------------------------------

std::string urgencyName(UrgencyLevel level) {
    std::string s = urgencyToString(level);
    size_t pos = s.find(" - ");
    return pos == std::string::npos ? s : s.substr(pos + 3);
}

json patientJson(const HospitalSystem& sys, const Patient& p) {
    PatientSnapshot snap(p, sys.getSimulationTime());
    json j;
    j["id"] = p.patientId;
    j["name"] = p.fullName;
    j["age"] = p.age;
    j["complaint"] = p.complaint;
    j["urgency"] = urgencyToInt(p.urgency);
    j["urgencyName"] = urgencyName(p.urgency);
    j["state"] = patientStateToString(p.state);
    j["arrivalSequence"] = p.arrivalSequence;
    j["arrivalTimeSeconds"] = p.simulatedArrivalTime;
    j["locationNode"] = p.currentLocationNode;
    j["locationName"] = sys.getNodeName(p.currentLocationNode);
    j["targetNode"] = p.targetLocationNode;
    j["targetName"] = sys.getNodeName(p.targetLocationNode);
    j["assignedResourceId"] = p.assignedResourceId;
    j["billing"] = p.billingAmount;
    j["waitingSeconds"] = snap.isWaitingDurationApplicable ? json(snap.waitingDurationSeconds) : json(nullptr);
    return j;
}

json resourceJson(const HospitalSystem& sys, const DispatchResource& r) {
    json j;
    j["id"] = r.resourceId;
    j["name"] = r.name;
    j["state"] = resourceStateToString(r.state);
    j["nodeId"] = r.currentNodeId;
    j["nodeName"] = sys.getNodeName(r.currentNodeId);
    j["assignedPatientId"] = r.assignedPatientId;
    return j;
}

json routeJson(const HospitalSystem& sys, int from, int to) {
    DijkstraResult r = sys.findRoute(from, to);
    json j;
    j["from"] = from;
    j["to"] = to;
    j["fromName"] = sys.getNodeName(from);
    j["toName"] = sys.getNodeName(to);
    j["reachable"] = r.isReachable;
    j["costSeconds"] = r.isReachable ? json(static_cast<long long>(r.totalCost)) : json(nullptr);
    json path = json::array();
    json names = json::array();
    json segments = json::array();
    if (r.isReachable) {
        for (size_t i = 0; i < r.path.size(); ++i) {
            path.push_back(r.path[i]);
            names.push_back(sys.getNodeName(r.path[i]));
            if (i + 1 < r.path.size()) {
                json seg;
                seg["u"] = r.path[i];
                seg["v"] = r.path[i + 1];
                seg["seconds"] = static_cast<long long>(sys.getMapManager().getCorridorWeight(r.path[i], r.path[i + 1]));
                segments.push_back(seg);
            }
        }
    }
    j["path"] = path;
    j["pathNames"] = names;
    j["segments"] = segments;
    return j;
}

json auditJson(const HospitalSystem& sys, const AuditEvent& e, size_t index) {
    json j;
    j["index"] = index;
    j["timestampSeconds"] = e.simulatedTimestamp;
    j["patientId"] = e.patientId;
    const Patient* p = e.patientId.empty() ? nullptr : sys.findPatient(e.patientId);
    j["patientName"] = p ? p->fullName : std::string();
    j["action"] = e.action;
    j["priorState"] = patientStateToString(e.priorState);
    j["newState"] = patientStateToString(e.newState);
    j["urgency"] = urgencyToInt(e.urgencyAtEvent);
    j["locationNode"] = e.locationAtEvent;
    j["locationName"] = sys.getNodeName(e.locationAtEvent);
    j["resourceId"] = e.resourceIdAtEvent;
    j["details"] = e.details;
    return j;
}

json metricsJson(const ds_algorithms::SortMetrics& m) {
    json j;
    j["algorithm"] = m.algorithmName;
    j["elementCount"] = m.elementCount;
    j["comparisons"] = m.comparisons;
    j["swaps"] = m.swaps;
    j["moves"] = m.moves;
    j["durationMicroseconds"] = m.durationMicroseconds;
    j["stable"] = m.isStable;
    j["timeBest"] = m.timeComplexityBest;
    j["timeAverage"] = m.timeComplexityAverage;
    j["timeWorst"] = m.timeComplexityWorst;
    j["spaceAuxiliary"] = m.spaceComplexityAuxiliary;
    return j;
}

json edgeJson(const HospitalSystem& sys, const CorridorEdge& e) {
    json j;
    j["u"] = e.u;
    j["v"] = e.v;
    j["uName"] = sys.getNodeName(e.u);
    j["vName"] = sys.getNodeName(e.v);
    j["seconds"] = static_cast<long long>(e.transitTimeSeconds);
    j["blocked"] = e.isBlocked;
    return j;
}

json mstJson(const HospitalSystem& sys, const char* algorithm, const MSTResult& r) {
    json j;
    j["algorithm"] = algorithm;
    j["connected"] = r.isConnectedSpanningTree;
    j["totalWeightSeconds"] = static_cast<long long>(r.totalTransitWeight);
    j["componentCount"] = r.componentCount;
    j["overflow"] = r.hasOverflow;
    json edges = json::array();
    for (const CorridorEdge& e : r.edges) edges.push_back(edgeJson(sys, e));
    j["edges"] = edges;
    return j;
}

bool lessUrgent(const Patient* a, const Patient* b) {
    int ua = urgencyToInt(a->urgency), ub = urgencyToInt(b->urgency);
    if (ua != ub) return ua < ub;
    return a->arrivalSequence < b->arrivalSequence;
}

} // namespace

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

struct HospitalApi::Impl {
    using Handler = std::function<json(Ctx&)>;
    struct Route {
        std::string method;
        std::vector<std::string> segments;
        Handler handler;
    };

    ApiConfig config;
    HospitalSystem sys;
    std::vector<Route> routes;

    explicit Impl(ApiConfig c) : config(std::move(c)) { registerRoutes(); }

    // ---- routing ----------------------------------------------------------

    static std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> out;
        size_t pos = 0;
        while (pos < path.size()) {
            if (path[pos] == '/') { ++pos; continue; }
            size_t next = path.find('/', pos);
            out.push_back(path.substr(pos, next == std::string::npos ? std::string::npos : next - pos));
            if (next == std::string::npos) break;
            pos = next;
        }
        return out;
    }

    void add(const std::string& method, const std::string& pattern, Handler h) {
        routes.push_back(Route{method, splitPath(pattern), std::move(h)});
    }

    bool originAllowed(const std::string& origin) const {
        for (const std::string& o : config.allowedOrigins) {
            if (o == "*" || o == origin) return true;
        }
        return false;
    }

    HttpResponse dispatch(const HttpRequest& req) {
        HttpResponse resp;
        const std::string origin = req.header("origin");
        const bool isRead = (req.method == "GET" || req.method == "HEAD" || req.method == "OPTIONS");

        // Cross-origin state changes are refused server-side (not only by the browser).
        if (!origin.empty() && !originAllowed(origin) && !isRead) {
            resp.status = 403;
            resp.body = json{{"error", {{"code", errorCode(403)}, {"message", "Origin not allowed."}}}}.dump();
            return resp;
        }
        if (!origin.empty() && originAllowed(origin)) {
            resp.headers.push_back({"Access-Control-Allow-Origin", origin});
            resp.headers.push_back({"Vary", "Origin"});
        }

        if (req.method == "OPTIONS") {
            resp.status = 204;
            resp.headers.push_back({"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"});
            resp.headers.push_back({"Access-Control-Allow-Headers", "Content-Type"});
            resp.headers.push_back({"Access-Control-Max-Age", "600"});
            return resp;
        }

        try {
            std::vector<std::string> segs = splitPath(req.path);
            bool pathMatched = false;
            for (const Route& route : routes) {
                if (route.segments.size() != segs.size()) continue;
                Ctx ctx(req);
                bool match = true;
                for (size_t i = 0; i < segs.size(); ++i) {
                    const std::string& pat = route.segments[i];
                    if (!pat.empty() && pat[0] == ':') {
                        ctx.params.push_back(segs[i]);
                    } else if (pat != segs[i]) {
                        match = false;
                        break;
                    }
                }
                if (!match) continue;
                pathMatched = true;
                if (route.method != req.method) continue;

                json out = route.handler(ctx);
                resp.status = ctx.status;
                resp.body = out.dump();
                return resp;
            }
            if (pathMatched) fail(405, "Method not allowed for this endpoint.");
            fail(404, "Unknown endpoint: " + req.path);
        } catch (const ApiError& e) {
            resp.status = e.status;
            resp.body = json{{"error", {{"code", errorCode(e.status)}, {"message", e.message}}}}.dump();
        }
        return resp;
    }

    // ---- shared lookups ----------------------------------------------------

    const Patient& requirePatient(const std::string& id) const {
        const Patient* p = sys.findPatient(id);
        if (!p) fail(404, "Patient ID '" + id + "' not found in registry.");
        return *p;
    }

    json patientById(const std::string& id) const { return patientJson(sys, requirePatient(id)); }

    /**
     * Priority-ordered triage queue obtained from the REAL TriageMinHeap: the live heap's
     * records are loaded into a scratch heap, then extractMin() is called repeatedly.
     * The live heap is never modified.
     */
    std::vector<TriageRecord> triageOrder() const {
        const std::vector<TriageRecord>& raw = sys.getTriageManager().getHeap().getRawRecords();
        std::vector<TriageRecord> ordered;
        TriageMinHeap scratch;
        if (!scratch.buildHeap(raw)) return ordered;
        TriageRecord rec;
        while (scratch.extractMin(rec)) ordered.push_back(rec);
        return ordered;
    }

    size_t countAction(const std::vector<AuditEvent>& events, const char* action) const {
        size_t n = 0;
        for (const AuditEvent& e : events) {
            if (e.action == action) ++n;
        }
        return n;
    }

    json dispatchJson(const Patient& p) const {
        json j;
        j["patient"] = patientJson(sys, p);
        const bool assigned = !p.assignedResourceId.empty();
        std::string status = p.state == PatientState::IN_TRANSIT ? "IN_TRANSIT"
                           : (assigned ? "RESOURCE_ASSIGNED" : "AWAITING_ASSIGNMENT");
        j["status"] = status;
        j["pickupNode"] = p.currentLocationNode;
        j["destinationNode"] = p.targetLocationNode;
        j["resource"] = nullptr;
        j["leg1"] = nullptr;
        j["leg2"] = nullptr;
        j["totalCostSeconds"] = nullptr;

        long long total = 0;
        bool totalValid = false;
        if (assigned) {
            const DispatchResource* r = sys.findResource(p.assignedResourceId);
            if (r) {
                j["resource"] = resourceJson(sys, *r);
                if (p.state == PatientState::DISPATCH_PENDING) {
                    json leg1 = routeJson(sys, r->currentNodeId, p.currentLocationNode);
                    j["leg1"] = leg1;
                    if (leg1["reachable"].get<bool>()) {
                        total += leg1["costSeconds"].get<long long>();
                        totalValid = true;
                    } else {
                        totalValid = false;
                    }
                } else {
                    totalValid = true; // pickup leg already executed
                }
            }
        }
        if (p.state == PatientState::DISPATCH_PENDING || p.state == PatientState::IN_TRANSIT) {
            json leg2 = routeJson(sys, p.currentLocationNode, p.targetLocationNode);
            j["leg2"] = leg2;
            if (assigned && totalValid && leg2["reachable"].get<bool>()) {
                total += leg2["costSeconds"].get<long long>();
                j["totalCostSeconds"] = total;
            }
        }
        return j;
    }

    json fleetSummary() const {
        json j;
        size_t available = 0, enRoute = 0, transporting = 0;
        std::vector<DispatchResource> all = sys.getAllResources();
        for (const DispatchResource& r : all) {
            if (r.state == ResourceState::AVAILABLE) ++available;
            else if (r.state == ResourceState::EN_ROUTE_PICKUP) ++enRoute;
            else ++transporting;
        }
        j["total"] = all.size();
        j["available"] = available;
        j["enRoutePickup"] = enRoute;
        j["transporting"] = transporting;
        return j;
    }

    std::vector<const Patient*> activeDispatchPatients() const {
        std::vector<const Patient*> out;
        for (const Patient* p : sys.getAllPatients()) {
            if (p && (p->state == PatientState::DISPATCH_PENDING || p->state == PatientState::IN_TRANSIT)) out.push_back(p);
        }
        std::sort(out.begin(), out.end(), lessUrgent);
        return out;
    }

    ReportingService::SortField parseSortField(const std::string& s) const {
        if (s == "age") return ReportingService::SortField::AGE;
        if (s == "arrival") return ReportingService::SortField::ARRIVAL_TIME;
        if (s == "urgency" || s.empty()) return ReportingService::SortField::URGENCY;
        if (s == "billing") return ReportingService::SortField::BILLING_AMOUNT;
        if (s == "waiting") return ReportingService::SortField::WAITING_DURATION;
        fail(400, "sortBy must be one of: age, arrival, urgency, billing, waiting.");
    }

    ReportingService::SortAlgorithm parseSortAlgorithm(const std::string& s) const {
        if (s == "merge" || s.empty()) return ReportingService::SortAlgorithm::MERGE_SORT;
        if (s == "quick") return ReportingService::SortAlgorithm::QUICK_SORT;
        if (s == "heap") return ReportingService::SortAlgorithm::HEAP_SORT;
        fail(400, "algorithm must be one of: merge, quick, heap.");
    }

    json snapshotJson(const PatientSnapshot& s) const {
        json j;
        j["id"] = s.patientId;
        j["name"] = s.fullName;
        j["age"] = s.age;
        j["complaint"] = s.complaint;
        j["urgency"] = urgencyToInt(s.urgency);
        j["urgencyName"] = urgencyName(s.urgency);
        j["state"] = patientStateToString(s.state);
        j["arrivalSequence"] = s.arrivalSequence;
        j["arrivalTimeSeconds"] = s.simulatedArrivalTime;
        j["billing"] = s.billingAmount;
        j["waitingSeconds"] = s.isWaitingDurationApplicable ? json(s.waitingDurationSeconds) : json(nullptr);
        return j;
    }

    // ---- routes ------------------------------------------------------------

    void registerRoutes() {
        // ===== Health & admin =====
        add("GET", "/api/health", [this](Ctx&) {
            json j;
            j["status"] = sys.isInitialized() ? "ok" : "uninitialized";
            j["initialized"] = sys.isInitialized();
            j["patients"] = sys.getPatientCount();
            j["simulationTimeSeconds"] = sys.getSimulationTime();
            j["resetEnabled"] = config.allowReset;
            return j;
        });

        add("POST", "/api/admin/reset", [this](Ctx&) {
            if (!config.allowReset) fail(403, "Reset is disabled on this server.");
            sys.reset();
            std::string err;
            if (!sys.initializeFromSeed(config.dataDir, err)) fail(422, "Reset failed: " + err);
            json j;
            j["status"] = "reset";
            j["patients"] = sys.getPatientCount();
            return j;
        });

        // ===== Patients =====
        add("GET", "/api/patients", [this](Ctx& c) {
            std::string q = queryString(c, "q");
            std::string state = queryString(c, "state");
            std::string urgencyStr = queryString(c, "urgency");
            int urgencyFilter = 0;
            if (!urgencyStr.empty()) {
                long long v;
                if (!parseIntStrict(urgencyStr, 1, 5, v)) fail(400, "urgency must be an integer between 1 and 5.");
                urgencyFilter = static_cast<int>(v);
            }
            auto lower = [](std::string s) {
                for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                return s;
            };
            std::string qLower = lower(q);
            json list = json::array();
            size_t total = 0;
            for (const Patient* p : sys.getAllPatients()) {
                if (!p) continue;
                ++total;
                if (!state.empty() && patientStateToString(p->state) != state) continue;
                if (urgencyFilter != 0 && urgencyToInt(p->urgency) != urgencyFilter) continue;
                if (!qLower.empty() && lower(p->patientId + " " + p->fullName).find(qLower) == std::string::npos) continue;
                list.push_back(patientJson(sys, *p));
            }
            json j;
            j["patients"] = list;
            j["total"] = total;
            j["matched"] = list.size();
            return j;
        });

        add("GET", "/api/patients/:id", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            const Patient& p = requirePatient(id);
            json j;
            j["patient"] = patientJson(sys, p);
            json history = json::array();
            std::vector<AuditEvent> events = sys.getAuditEvents();
            for (size_t i = 0; i < events.size(); ++i) {
                if (events[i].patientId == id) history.push_back(auditJson(sys, events[i], i));
            }
            j["history"] = history;
            const bool active = p.state == PatientState::DISPATCH_PENDING || p.state == PatientState::IN_TRANSIT;
            j["dispatch"] = active ? dispatchJson(p) : json(nullptr);
            return j;
        });

        add("POST", "/api/patients", [this](Ctx& c) {
            const json& b = c.body();
            std::string id = bodyString(b, "id", kMaxPatientIdLength);
            std::string name = bodyString(b, "name", kMaxPatientNameLength);
            int age = bodyInt(b, "age", 0, 150);
            std::string complaint = bodyString(b, "complaint", kMaxComplaintLength);
            UrgencyLevel urgency = urgencyFromBody(b);
            int loc = bodyInt(b, "locationNode", 0, 1000000);
            int target = bodyInt(b, "targetNode", 0, 1000000);
            double billing = bodyNumber(b, "billing");
            std::string err;
            if (!sys.registerNewPatient(id, name, age, complaint, urgency, loc, target, billing, err)) domainFail(err);
            c.status = 201;
            return json{{"patient", patientById(id)}};
        });

        add("PUT", "/api/patients/:id", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            const json& b = c.body();
            for (auto it = b.begin(); it != b.end(); ++it) {
                if (it.key() != "urgency") fail(400, "Only 'urgency' can be updated on an existing patient.");
            }
            UrgencyLevel urgency = urgencyFromBody(b);
            std::string err;
            if (!sys.updateUrgency(id, urgency, err)) domainFail(err);
            return json{{"patient", patientById(id)}};
        });

        add("POST", "/api/patients/:id/discharge", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            std::string err;
            if (!sys.dischargePatient(id, err)) domainFail(err);
            return json{{"patient", patientById(id)}};
        });

        // ===== Emergency triage =====
        add("GET", "/api/triage/queue", [this](Ctx&) {
            json queue = json::array();
            std::vector<TriageRecord> ordered = triageOrder();
            for (size_t i = 0; i < ordered.size(); ++i) {
                const Patient* p = sys.findPatient(ordered[i].patientId);
                if (!p) continue;
                json row;
                row["rank"] = i + 1;
                row["patient"] = patientJson(sys, *p);
                queue.push_back(row);
            }
            json heap = json::array();
            const std::vector<TriageRecord>& raw = sys.getTriageManager().getHeap().getRawRecords();
            for (size_t i = 0; i < raw.size(); ++i) {
                json n;
                n["index"] = i;
                n["patientId"] = raw[i].patientId;
                n["urgency"] = urgencyToInt(raw[i].urgency);
                n["arrivalSequence"] = raw[i].arrivalSequence;
                heap.push_back(n);
            }
            std::vector<AuditEvent> events = sys.getAuditEvents();
            json j;
            j["size"] = ordered.size();
            j["queue"] = queue;
            j["heapArray"] = heap;
            j["urgencyUpdates"] = countAction(events, "URGENCY_UPDATED");
            j["policy"] = "Lower urgency number first; equal urgency resolved by earlier arrival sequence (TriageMinHeap).";
            return j;
        });

        add("GET", "/api/triage/next", [this](Ctx&) {
            Patient* p = nullptr;
            json j;
            if (sys.peekTriageHighestPriority(p) && p) {
                j["patient"] = patientJson(sys, *p);
            } else {
                j["patient"] = nullptr;
            }
            j["queueSize"] = sys.getTriageQueueSize();
            return j;
        });

        add("POST", "/api/triage/admit", [this](Ctx& c) {
            const json& b = c.body();
            std::string id = bodyString(b, "patientId", kMaxIdLength);
            UrgencyLevel urgency = urgencyFromBody(b);
            std::string err;
            if (!sys.triagePatient(id, urgency, err)) domainFail(err);
            json j;
            j["patient"] = patientById(id);
            std::vector<TriageRecord> ordered = triageOrder();
            for (size_t i = 0; i < ordered.size(); ++i) {
                if (ordered[i].patientId == id) j["rank"] = i + 1;
            }
            j["queueSize"] = ordered.size();
            c.status = 201;
            return j;
        });

        add("PUT", "/api/triage/:id/urgency", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            UrgencyLevel urgency = urgencyFromBody(c.body());
            std::string err;
            if (!sys.updateUrgency(id, urgency, err)) domainFail(err);
            json j;
            j["patient"] = patientById(id);
            std::vector<TriageRecord> ordered = triageOrder();
            j["rank"] = nullptr;
            for (size_t i = 0; i < ordered.size(); ++i) {
                if (ordered[i].patientId == id) j["rank"] = i + 1;
            }
            return j;
        });

        // ===== Appointments (FIFO queue) =====
        add("GET", "/api/appointments", [this](Ctx&) {
            json queue = json::array();
            std::vector<std::string> ids = sys.getScheduledAppointmentPatientIds();
            for (size_t i = 0; i < ids.size(); ++i) {
                const Patient* p = sys.findPatient(ids[i]);
                if (!p) continue;
                json row;
                row["position"] = i + 1;
                row["patient"] = patientJson(sys, *p);
                queue.push_back(row);
            }
            std::vector<AuditEvent> events = sys.getAuditEvents();
            json stats;
            stats["scheduledTotal"] = countAction(events, "APPOINTMENT_SCHEDULED");
            stats["called"] = countAction(events, "APPOINTMENT_CALLED");
            stats["cancelled"] = countAction(events, "APPOINTMENT_CANCELLED");
            stats["convertedToEmergency"] = countAction(events, "APPOINTMENT_CONVERTED_TO_EMERGENCY");
            json j;
            j["size"] = ids.size();
            j["queue"] = queue;
            j["stats"] = stats;
            return j;
        });

        add("POST", "/api/appointments", [this](Ctx& c) {
            std::string id = bodyString(c.body(), "patientId", kMaxIdLength);
            std::string err;
            if (!sys.scheduleAppointment(id, err)) domainFail(err);
            c.status = 201;
            json j;
            j["patient"] = patientById(id);
            j["queueSize"] = sys.getAppointmentQueueSize();
            return j;
        });

        add("DELETE", "/api/appointments/:id", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            std::string err;
            if (!sys.cancelAppointment(id, err)) domainFail(err);
            json j;
            j["patient"] = patientById(id);
            j["queueSize"] = sys.getAppointmentQueueSize();
            return j;
        });

        add("POST", "/api/appointments/next", [this](Ctx&) {
            Patient* p = nullptr;
            std::string err;
            if (!sys.callNextAppointment(p, err) || !p) domainFail(err.empty() ? "No appointment to call." : err);
            json j;
            j["patient"] = patientJson(sys, *p);
            j["queueSize"] = sys.getAppointmentQueueSize();
            return j;
        });

        add("POST", "/api/appointments/:id/convert", [this](Ctx& c) {
            const std::string& id = pathId(c, 0);
            UrgencyLevel urgency = urgencyFromBody(c.body());
            std::string err;
            if (!sys.convertAppointmentToEmergency(id, urgency, err)) domainFail(err);
            json j;
            j["patient"] = patientById(id);
            j["triageQueueSize"] = sys.getTriageQueueSize();
            return j;
        });

        // ===== Route dispatch =====
        add("GET", "/api/dispatch/resources", [this](Ctx&) {
            json list = json::array();
            for (const DispatchResource& r : sys.getAllResources()) list.push_back(resourceJson(sys, r));
            json j;
            j["resources"] = list;
            j["summary"] = fleetSummary();
            return j;
        });

        add("GET", "/api/dispatch/pending", [this](Ctx&) {
            json list = json::array();
            for (const Patient* p : activeDispatchPatients()) list.push_back(dispatchJson(*p));
            json j;
            j["dispatches"] = list;
            j["summary"] = fleetSummary();
            return j;
        });

        // Moves the highest-priority waiting patient into the dispatch pool
        // (HospitalSystem::selectNextPatientForDispatch: arbitrates triage heap root vs pending pool).
        add("POST", "/api/dispatch/select-next", [this](Ctx&) {
            std::set<std::string> pendingBefore;
            for (const Patient* p : sys.getUnassignedPendingPatients()) {
                if (p) pendingBefore.insert(p->patientId);
            }
            std::string err;
            Patient* p = sys.selectNextPatientForDispatch(err);
            if (!p) domainFail(err.empty() ? "No eligible emergency patients waiting for dispatch." : err);
            json j;
            j["patient"] = patientJson(sys, *p);
            j["source"] = pendingBefore.count(p->patientId) ? "PENDING_POOL" : "TRIAGE_QUEUE";
            j["triageQueueSize"] = sys.getTriageQueueSize();
            return j;
        });

        add("POST", "/api/dispatch/assign", [this](Ctx& c) {
            std::string id = bodyString(c.body(), "patientId", kMaxIdLength);
            std::string err;
            if (!sys.assignDispatch(id, err)) domainFail(err);
            return json{{"dispatch", dispatchJson(requirePatient(id))}};
        });

        add("POST", "/api/dispatch/pickup", [this](Ctx& c) {
            std::string id = bodyString(c.body(), "patientId", kMaxIdLength);
            std::string err;
            if (!sys.simulationStepPickup(id, err)) domainFail(err);
            json j;
            j["dispatch"] = dispatchJson(requirePatient(id));
            j["simulationTimeSeconds"] = sys.getSimulationTime();
            return j;
        });

        add("POST", "/api/dispatch/complete", [this](Ctx& c) {
            std::string id = bodyString(c.body(), "patientId", kMaxIdLength);
            std::string err;
            if (!sys.simulationStepTransportAndHandover(id, err)) domainFail(err);
            json j;
            j["patient"] = patientById(id);
            j["simulationTimeSeconds"] = sys.getSimulationTime();
            return j;
        });

        add("POST", "/api/dispatch/cancel", [this](Ctx& c) {
            std::string id = bodyString(c.body(), "patientId", kMaxIdLength);
            std::string err;
            if (!sys.cancelPickup(id, err)) domainFail(err);
            return json{{"patient", patientById(id)}};
        });

        // ===== Campus map =====
        add("GET", "/api/campus/nodes", [this](Ctx&) {
            json list = json::array();
            for (const CampusLocation& l : sys.getAllLocations()) {
                json n;
                n["id"] = l.nodeId;
                n["name"] = l.name;
                n["wing"] = l.buildingWing;
                n["isTreatmentUnit"] = l.isTreatmentUnit;
                list.push_back(n);
            }
            return json{{"nodes", list}};
        });

        add("GET", "/api/campus/edges", [this](Ctx&) {
            json list = json::array();
            size_t blocked = 0;
            for (const CorridorEdge& e : sys.getAllCorridors()) {
                list.push_back(edgeJson(sys, e));
                if (e.isBlocked) ++blocked;
            }
            json j;
            j["edges"] = list;
            j["blockedCount"] = blocked;
            return j;
        });

        add("GET", "/api/campus/route", [this](Ctx& c) {
            int from = queryInt(c, "from", 0, 1000000);
            int to = queryInt(c, "to", 0, 1000000);
            if (!sys.isValidNode(from) || !sys.isValidNode(to)) fail(404, "Unknown campus node.");
            return routeJson(sys, from, to);
        });

        add("GET", "/api/campus/traversal", [this](Ctx& c) {
            std::string algo = queryString(c, "algorithm");
            int start = queryInt(c, "start", 0, 1000000);
            if (!sys.isValidNode(start)) fail(404, "Unknown campus node.");
            std::vector<int> order;
            if (algo == "bfs") order = sys.getBFSOrder(start);
            else if (algo == "dfs") order = sys.getDFSOrder(start);
            else fail(400, "algorithm must be 'bfs' or 'dfs'.");
            json ids = json::array();
            json names = json::array();
            for (int n : order) {
                ids.push_back(n);
                names.push_back(sys.getNodeName(n));
            }
            json j;
            j["algorithm"] = algo;
            j["start"] = start;
            j["order"] = ids;
            j["orderNames"] = names;
            j["visitedCount"] = order.size();
            return j;
        });

        add("GET", "/api/campus/mst", [this](Ctx& c) {
            std::string algo = queryString(c, "algorithm");
            if (algo == "prim") return mstJson(sys, "prim", sys.computePrimMST());
            if (algo == "kruskal") return mstJson(sys, "kruskal", sys.computeKruskalMST());
            fail(400, "algorithm must be 'prim' or 'kruskal'.");
        });

        add("POST", "/api/campus/corridor/block", [this](Ctx& c) {
            int u = bodyInt(c.body(), "u", 0, 1000000);
            int v = bodyInt(c.body(), "v", 0, 1000000);
            std::string err;
            if (!sys.blockCorridor(u, v, err)) domainFail(err);
            return json{{"u", u}, {"v", v}, {"blocked", true}};
        });

        add("POST", "/api/campus/corridor/reopen", [this](Ctx& c) {
            int u = bodyInt(c.body(), "u", 0, 1000000);
            int v = bodyInt(c.body(), "v", 0, 1000000);
            std::string err;
            if (!sys.reopenCorridor(u, v, err)) domainFail(err);
            return json{{"u", u}, {"v", v}, {"blocked", false}};
        });

        // ===== Analytics & audit =====
        add("GET", "/api/analytics/overview", [this](Ctx&) {
            std::map<std::string, size_t> byState;
            std::vector<const Patient*> all;
            for (const Patient* p : sys.getAllPatients()) {
                if (!p) continue;
                all.push_back(p);
                ++byState[patientStateToString(p->state)];
            }
            json states = json::object();
            for (const auto& kv : byState) states[kv.first] = kv.second;

            json j;
            j["totalPatients"] = all.size();
            j["byState"] = states;
            j["triageQueueSize"] = sys.getTriageQueueSize();
            j["appointmentQueueSize"] = sys.getAppointmentQueueSize();
            j["activeDispatches"] = activeDispatchPatients().size();
            j["fleet"] = fleetSummary();
            j["simulationTimeSeconds"] = sys.getSimulationTime();

            Patient* top = nullptr;
            j["highestPriority"] = (sys.peekTriageHighestPriority(top) && top) ? patientJson(sys, *top) : json(nullptr);

            std::vector<AuditEvent> events = sys.getAuditEvents();
            json recent = json::array();
            size_t start = events.size() > 6 ? events.size() - 6 : 0;
            for (size_t i = events.size(); i > start; --i) recent.push_back(auditJson(sys, events[i - 1], i - 1));
            j["recentActivity"] = recent;

            size_t counts[6] = {0, 0, 0, 0, 0, 0};
            for (const Patient* p : all) ++counts[urgencyToInt(p->urgency)];
            json dist = json::array();
            for (int level = 1; level <= 5; ++level) {
                dist.push_back(json{{"level", level}, {"name", urgencyName(intToUrgency(level))}, {"count", counts[level]}});
            }
            j["urgencyDistribution"] = dist;
            return j;
        });

        add("GET", "/api/analytics/urgency", [this](Ctx&) {
            size_t all[6] = {0, 0, 0, 0, 0, 0};
            size_t waiting[6] = {0, 0, 0, 0, 0, 0};
            for (const Patient* p : sys.getAllPatients()) {
                if (!p) continue;
                int u = urgencyToInt(p->urgency);
                ++all[u];
                if (p->state == PatientState::TRIAGED_WAITING) ++waiting[u];
            }
            json dist = json::array();
            for (int level = 1; level <= 5; ++level) {
                dist.push_back(json{{"level", level}, {"name", urgencyName(intToUrgency(level))},
                                    {"allPatients", all[level]}, {"waitingInTriage", waiting[level]}});
            }
            return json{{"distribution", dist}};
        });

        // Patients sorted by the project's custom Merge / Quick / Heap sort (ReportingService).
        add("GET", "/api/analytics/patients", [this](Ctx& c) {
            ReportingService::SortField field = parseSortField(queryString(c, "sortBy"));
            ReportingService::SortAlgorithm algo = parseSortAlgorithm(queryString(c, "algorithm"));
            std::vector<PatientSnapshot> snaps = ReportingService::captureSnapshots(sys);
            ds_algorithms::SortMetrics metrics = ReportingService::sortSnapshots(snaps, field, algo);
            json rows = json::array();
            for (const PatientSnapshot& s : snaps) rows.push_back(snapshotJson(s));
            json j;
            j["sortBy"] = ReportingService::fieldToString(field);
            j["algorithm"] = ReportingService::algorithmToString(algo);
            j["metrics"] = metricsJson(metrics);
            j["patients"] = rows;
            return j;
        });

        // Runs all three custom sorts and the graph algorithms on the live data.
        add("GET", "/api/analytics/algorithms", [this](Ctx& c) {
            ReportingService::SortField field = parseSortField(queryString(c, "sortBy").empty() ? "billing" : queryString(c, "sortBy"));
            json sorts = json::array();
            const ReportingService::SortAlgorithm algos[3] = {
                ReportingService::SortAlgorithm::MERGE_SORT,
                ReportingService::SortAlgorithm::QUICK_SORT,
                ReportingService::SortAlgorithm::HEAP_SORT};
            for (ReportingService::SortAlgorithm a : algos) {
                std::vector<PatientSnapshot> snaps = ReportingService::captureSnapshots(sys);
                sorts.push_back(metricsJson(ReportingService::sortSnapshots(snaps, field, a)));
            }

            // Graph algorithms: average wall time over repeated runs (single runs are below clock resolution).
            const int iterations = 500;
            int nodeCount = static_cast<int>(sys.getAllLocations().size());
            int last = nodeCount > 0 ? nodeCount - 1 : 0;
            json graph = json::array();
            auto timeIt = [&](const char* name, const char* complexity, const std::function<void()>& fn) {
                std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < iterations; ++i) fn();
                std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
                double totalUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
                graph.push_back(json{{"algorithm", name}, {"complexity", complexity}, {"iterations", iterations},
                                     {"averageMicroseconds", totalUs / iterations}});
            };
            timeIt("Dijkstra", "O((V + E) log V)", [&]() { (void)sys.findRoute(0, last); });
            timeIt("BFS", "O(V + E)", [&]() { (void)sys.getBFSOrder(0); });
            timeIt("DFS", "O(V + E)", [&]() { (void)sys.getDFSOrder(0); });
            timeIt("Prim MST", "O((V + E) log V)", [&]() { (void)sys.computePrimMST(); });
            timeIt("Kruskal MST", "O(E log E)", [&]() { (void)sys.computeKruskalMST(); });

            json j;
            j["sortBy"] = ReportingService::fieldToString(field);
            j["patientCount"] = sys.getPatientCount();
            j["sorts"] = sorts;
            j["graph"] = graph;
            return j;
        });

        add("GET", "/api/audit", [this](Ctx& c) {
            std::string patientId = queryString(c, "patientId");
            long long limit = 200;
            std::string limitStr = queryString(c, "limit");
            if (!limitStr.empty() && !parseIntStrict(limitStr, 1, 1000, limit)) fail(400, "limit must be an integer between 1 and 1000.");
            std::vector<AuditEvent> events = sys.getAuditEvents();
            json list = json::array();
            std::vector<size_t> picked;
            for (size_t i = 0; i < events.size(); ++i) {
                if (!patientId.empty() && events[i].patientId != patientId) continue;
                picked.push_back(i);
            }
            size_t from = picked.size() > static_cast<size_t>(limit) ? picked.size() - static_cast<size_t>(limit) : 0;
            for (size_t k = from; k < picked.size(); ++k) list.push_back(auditJson(sys, events[picked[k]], picked[k]));
            json j;
            j["events"] = list;
            j["total"] = events.size();
            j["returned"] = list.size();
            j["order"] = "chronological";
            return j;
        });
    }
};

// ---------------------------------------------------------------------------
// Public wrapper
// ---------------------------------------------------------------------------

HospitalApi::HospitalApi(ApiConfig config) : impl_(new Impl(std::move(config))) {}
HospitalApi::~HospitalApi() = default;

bool HospitalApi::initialize(std::string& outError) {
    return impl_->sys.initializeFromSeed(impl_->config.dataDir, outError);
}

HttpResponse HospitalApi::handle(const HttpRequest& request) {
    return impl_->dispatch(request);
}
