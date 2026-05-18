#pragma once
#include "msg.h"
#include <string>
#include <vector>

using StrPairs = std::vector<std::pair<std::string_view, std::string_view>>;

namespace Status {

static constexpr const char* OK                     {"200"};
static constexpr const char* CREATED                {"201"};
static constexpr const char* NO_CONTENT             {"204"};

static constexpr const char* BAD_REQUEST            {"400"};
static constexpr const char* UNAUTHORIZED           {"401"};
static constexpr const char* FORBIDDEN              {"403"};
static constexpr const char* NOT_FOUND              {"404"};
static constexpr const char* METHOD_NOT_ALLOWED     {"405"};
static constexpr const char* REQUEST_TIMEOUT        {"408"};
static constexpr const char* PAYLOAD_TOO_LARGE      {"413"};
static constexpr const char* URI_TOO_LONG           {"414"};

static constexpr const char* INTERNAL_ERROR         {"500"};
static constexpr const char* NOT_IMPLEMENTED        {"501"};
static constexpr const char* BAD_GATEWAY            {"502"};
static constexpr const char* SERVICE_UNAVAILABLE    {"503"};

struct Error {
    ConnID              id      = 0;
    std::string_view    status  = OK;
    std::string         reason  = "OK";

    inline bool is_ok() { return status == OK; }
};

static constexpr Error OK_E {0, OK, "OK"};


} // namespace HTTP_RES
