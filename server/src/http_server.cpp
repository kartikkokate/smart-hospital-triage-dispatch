#include "http_server.hpp"

#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>

#ifdef _WIN32
  #include <winsock2.h>
  typedef SOCKET sock_t;
  static const sock_t kInvalidSocket = INVALID_SOCKET;
  static void closeSocket(sock_t s) { closesocket(s); }
  static void shutdownSend(sock_t s) { shutdown(s, SD_SEND); }
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  static const sock_t kInvalidSocket = -1;
  static void closeSocket(sock_t s) { close(s); }
  static void shutdownSend(sock_t s) { shutdown(s, SHUT_WR); }
#endif

namespace {

volatile std::sig_atomic_t g_stopRequested = 0;

struct Connection {
    sock_t sock;
    std::string buffer;
    std::time_t lastActive;
    bool draining = false;     // response already sent; discarding the rest of the request
    size_t drainedBytes = 0;
};

constexpr int kDrainTimeoutSeconds = 3;
constexpr size_t kMaxDrainBytes = 4 * 1024 * 1024;

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 422: return "Unprocessable Entity";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default:  return "Unknown";
    }
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string percentDecode(const std::string& in, bool plusIsSpace) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if (c == '%' && i + 2 < in.size() && hexVal(in[i + 1]) >= 0 && hexVal(in[i + 2]) >= 0) {
            out.push_back(static_cast<char>(hexVal(in[i + 1]) * 16 + hexVal(in[i + 2])));
            i += 2;
        } else if (c == '+' && plusIsSpace) {
            out.push_back(' ');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

void parseQuery(const std::string& raw, std::map<std::string, std::string>& out) {
    size_t pos = 0;
    while (pos <= raw.size()) {
        size_t amp = raw.find('&', pos);
        std::string pair = raw.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
        if (!pair.empty()) {
            size_t eq = pair.find('=');
            std::string k = percentDecode(pair.substr(0, eq), true);
            std::string v = eq == std::string::npos ? "" : percentDecode(pair.substr(eq + 1), true);
            if (!k.empty()) out[k] = v;
        }
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
}

HttpResponse errorResponse(int status, const std::string& message) {
    HttpResponse r;
    r.status = status;
    r.body = std::string("{\"error\":{\"code\":\"") + statusText(status) + "\",\"message\":\"" + message + "\"}}";
    return r;
}

void sendAll(sock_t s, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        #ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;  // never raise SIGPIPE if the client disconnected
#else
        const int flags = 0;
#endif
        int n = send(s, data.data() + sent, static_cast<int>(data.size() - sent), flags);
        if (n <= 0) return;
        sent += static_cast<size_t>(n);
    }
}

void writeResponse(sock_t s, const HttpResponse& r) {
    std::string out = "HTTP/1.1 " + std::to_string(r.status) + " " + statusText(r.status) + "\r\n";
    out += "Content-Type: " + r.contentType + "; charset=utf-8\r\n";
    out += "Content-Length: " + std::to_string(r.body.size()) + "\r\n";
    out += "Connection: close\r\n";
    out += "Cache-Control: no-store\r\n";
    out += "X-Content-Type-Options: nosniff\r\n";
    for (const auto& h : r.headers) out += h.first + ": " + h.second + "\r\n";
    out += "\r\n";
    if (r.status != 204) out += r.body;
    sendAll(s, out);
}

enum class ParseState { NeedMore, Ready, Rejected };

/** Parses buffer into req. On Rejected, rejection holds the response to send. */
ParseState tryParse(const std::string& buf, HttpRequest& req, HttpResponse& rejection) {
    size_t headerEnd = buf.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        if (buf.size() > HttpServer::kMaxHeaderBytes) {
            rejection = errorResponse(431, "Request headers too large.");
            return ParseState::Rejected;
        }
        return ParseState::NeedMore;
    }
    if (headerEnd > HttpServer::kMaxHeaderBytes) {
        rejection = errorResponse(431, "Request headers too large.");
        return ParseState::Rejected;
    }

    std::string head = buf.substr(0, headerEnd);
    size_t lineEnd = head.find("\r\n");
    std::string requestLine = head.substr(0, lineEnd);

    size_t sp1 = requestLine.find(' ');
    size_t sp2 = sp1 == std::string::npos ? std::string::npos : requestLine.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        rejection = errorResponse(400, "Malformed request line.");
        return ParseState::Rejected;
    }
    req.method = requestLine.substr(0, sp1);
    std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
    std::string version = requestLine.substr(sp2 + 1);
    if (version.compare(0, 5, "HTTP/") != 0 || target.empty() || target[0] != '/') {
        rejection = errorResponse(400, "Malformed request line.");
        return ParseState::Rejected;
    }

    size_t qm = target.find('?');
    std::string rawPath = target.substr(0, qm);
    req.rawQuery = qm == std::string::npos ? "" : target.substr(qm + 1);
    req.path = percentDecode(rawPath, false);
    parseQuery(req.rawQuery, req.query);

    size_t pos = lineEnd == std::string::npos ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        size_t e = head.find("\r\n", pos);
        std::string line = head.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos && colon > 0) {
            req.headers[toLower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
        }
        if (e == std::string::npos) break;
        pos = e + 2;
    }

    if (!req.header("transfer-encoding").empty()) {
        rejection = errorResponse(501, "Chunked request bodies are not supported.");
        return ParseState::Rejected;
    }

    size_t contentLength = 0;
    std::string cl = req.header("content-length");
    if (!cl.empty()) {
        if (cl.size() > 9 || cl.find_first_not_of("0123456789") != std::string::npos) {
            rejection = errorResponse(400, "Invalid Content-Length.");
            return ParseState::Rejected;
        }
        contentLength = static_cast<size_t>(std::strtoul(cl.c_str(), nullptr, 10));
    }
    if (contentLength > HttpServer::kMaxBodyBytes) {
        rejection = errorResponse(413, "Request body too large.");
        return ParseState::Rejected;
    }

    size_t bodyStart = headerEnd + 4;
    if (buf.size() - bodyStart < contentLength) return ParseState::NeedMore;
    req.body = buf.substr(bodyStart, contentLength);
    return ParseState::Ready;
}

void onSignal(int) { g_stopRequested = 1; }

} // namespace

HttpServer::HttpServer(std::string host, int port, Handler handler)
    : host_(std::move(host)), port_(port), handler_(std::move(handler)),
      listenSocket_(static_cast<long long>(kInvalidSocket)), wsaStarted_(false) {}

HttpServer::~HttpServer() {
    if (static_cast<sock_t>(listenSocket_) != kInvalidSocket) closeSocket(static_cast<sock_t>(listenSocket_));
#ifdef _WIN32
    if (wsaStarted_) WSACleanup();
#endif
}

void HttpServer::requestStop() noexcept { g_stopRequested = 1; }

bool HttpServer::start(std::string& outError) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        outError = "WSAStartup failed.";
        return false;
    }
    wsaStarted_ = true;
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == kInvalidSocket) {
        outError = "Could not create socket.";
        return false;
    }
#ifndef _WIN32
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#endif
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port_));
    addr.sin_addr.s_addr = inet_addr(host_.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        closeSocket(s);
        outError = "Invalid IPv4 listen address '" + host_ + "'.";
        return false;
    }
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(s);
        outError = "Could not bind " + host_ + ":" + std::to_string(port_) + " (port already in use?).";
        return false;
    }
    if (listen(s, 16) != 0) {
        closeSocket(s);
        outError = "listen() failed.";
        return false;
    }
    listenSocket_ = static_cast<long long>(s);
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    return true;
}

void HttpServer::run() {
    sock_t listener = static_cast<sock_t>(listenSocket_);
    std::vector<Connection> conns;

    while (!g_stopRequested) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listener, &readSet);
        sock_t maxFd = listener;
        for (const Connection& c : conns) {
            FD_SET(c.sock, &readSet);
            if (c.sock > maxFd) maxFd = c.sock;
        }

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ready = select(static_cast<int>(maxFd) + 1, &readSet, nullptr, nullptr, &tv);
        std::time_t now = std::time(nullptr);
        if (ready < 0) continue; // interrupted by signal; loop condition re-checks stop flag

        if (ready > 0 && FD_ISSET(listener, &readSet)) {
            sock_t cs = accept(listener, nullptr, nullptr);
            if (cs != kInvalidSocket) {
                if (conns.size() >= kMaxConnections) {
                    closeSocket(cs);
                } else {
                    Connection c;
                    c.sock = cs;
                    c.lastActive = now;
                    conns.push_back(c);
                }
            }
        }

        for (size_t i = 0; i < conns.size();) {
            Connection& c = conns[i];
            bool close = false;

            if (ready > 0 && FD_ISSET(c.sock, &readSet)) {
                char chunk[4096];
                int n = recv(c.sock, chunk, sizeof(chunk), 0);
                if (n <= 0) {
                    close = true;
                } else if (c.draining) {
                    // Discard the remainder of a rejected request so closing does not reset the connection
                    // before the client has read our error response.
                    c.drainedBytes += static_cast<size_t>(n);
                    if (c.drainedBytes > kMaxDrainBytes) close = true;
                } else {
                    c.buffer.append(chunk, static_cast<size_t>(n));
                    c.lastActive = now;

                    HttpRequest req;
                    HttpResponse rejection;
                    ParseState st = tryParse(c.buffer, req, rejection);
                    if (st == ParseState::Rejected) {
                        writeResponse(c.sock, rejection);
                        shutdownSend(c.sock);
                        c.draining = true;
                        c.buffer.clear();
                    } else if (st == ParseState::Ready) {
                        HttpResponse resp;
                        try {
                            resp = handler_(req);
                        } catch (const std::exception&) {
                            resp = errorResponse(500, "Internal server error.");
                        } catch (...) {
                            resp = errorResponse(500, "Internal server error.");
                        }
                        writeResponse(c.sock, resp);
                        close = true;
                    }
                }
            } else if (now - c.lastActive > (c.draining ? kDrainTimeoutSeconds : kIdleTimeoutSeconds)) {
                close = true;
            }

            if (close) {
                closeSocket(c.sock);
                conns.erase(conns.begin() + static_cast<std::ptrdiff_t>(i));
            } else {
                ++i;
            }
        }
    }

    for (Connection& c : conns) closeSocket(c.sock);
}
