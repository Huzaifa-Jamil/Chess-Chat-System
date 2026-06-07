#pragma once
#include <string>

static const std::string TAG_AUTH  = "AUTH";
static const std::string TAG_CHAT  = "CHAT";
static const std::string TAG_GAME  = "GAME";   // chess moves
static const std::string TAG_START = "START"; // start gaem
static const std::string TAG_WIN   = "WIN";
static const std::string TAG_LOSS  = "LOSS";
static const std::string TAG_TIE   = "TIE";
static const std::string TAG_END   = "END";
static const std::string TAG_WAIT  = "WAIT";

static const char PROTOCOL_SEP = '|';

class Protocol
{
public:

    static std::string build(const std::string &tag, const std::string &data)
    {
        return tag + PROTOCOL_SEP + data + "\n";
    }

    static std::string getTag(const std::string &msg)
    {
        size_t pos = msg.find(PROTOCOL_SEP);
        if (pos == std::string::npos) return "";
        return msg.substr(0, pos);
    }

    static std::string getData(const std::string &msg)
    {
        size_t pos = msg.find(PROTOCOL_SEP);
        if (pos == std::string::npos) return "";
        return msg.substr(pos + 1);
    }

    static std::string clean(const std::string &msg)
    {
        std::string s = msg;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        return s;
    }
};
