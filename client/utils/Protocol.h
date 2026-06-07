#pragma once
#include <string>

inline const std::string TAG_AUTH = "AUTH";
inline const std::string TAG_CHAT = "CHAT";
inline const std::string TAG_GAME = "GAME";
inline const std::string TAG_START = "START";
inline const std::string TAG_WIN = "WIN";
inline const std::string TAG_LOSS = "LOSS";
inline const std::string TAG_TIE = "TIE";
inline const std::string TAG_END = "END";
inline const std::string TAG_WAIT = "WAIT";

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
        if (pos == std::string::npos)
            return "";
        return msg.substr(0, pos);
    }

    static std::string getData(const std::string &msg)
    {
        size_t pos = msg.find(PROTOCOL_SEP);
        if (pos == std::string::npos)
            return "";
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
