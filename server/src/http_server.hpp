#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * Minimal single-threaded HTTP/1.1 server used to expose the (single-threaded)
 * HospitalSystem over REST.
 *
 * DESIGN NOTES:
 * - One select() event loop handles every connection, so requests are processed
 *   strictly one at a time. This matches HospitalSystem, which is not thread-safe,
 *   and needs no locking.
 * - Idle or half-open connections (browsers open speculative sockets) never block
 *   the loop and are dropped after a timeout.
 * - Hard limits on header size, body size and open connections.
 * - One request per connection (Connection: close). Keep-alive is deliberately not
 *   implemented; loopback connection setup cost is negligible.
 * - Only the request line, headers and Content-Length body are parsed. Chunked
 *   request bodies are rejected.
 */
struct HttpRequest {
    std::string method;
    std::string path;      // percent-decoded, without query string
    std::string rawQuery;
    std::map<std::string, std::string> query;    // percent-decoded
    std::map<std::string, std::string> headers;  // lower-cased names
    std::string body;

    std::string header(const std::string& lowerName) const {
        auto it = headers.find(lowerName);
        return it == headers.end() ? std::string() : it->second;
    }
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    static constexpr size_t kMaxHeaderBytes = 16 * 1024;
    static constexpr size_t kMaxBodyBytes = 64 * 1024;
    static constexpr size_t kMaxConnections = 48;
    static constexpr int kIdleTimeoutSeconds = 10;

    HttpServer(std::string host, int port, Handler handler);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    /** Binds and listens. Returns false with outError on failure. */
    bool start(std::string& outError);

    /** Runs the event loop until requestStop() is called. */
    void run();

    /** Safe to call from a signal handler. */
    static void requestStop() noexcept;

private:
    std::string host_;
    int port_;
    Handler handler_;
    long long listenSocket_;
    bool wsaStarted_;
};
