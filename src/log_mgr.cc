#include "log_mgr.hpp"
#include <vector>

namespace lua_sftp
{
Log& Log::Ins()
{
    static Log ins;
    return ins;
}
Log::Message Log::build_msg(Log::Level level, std::string_view msg)
{
    std::string l;
    l.reserve(8);

    switch (level)
    {
    case Level::Info:
        l.append("INFO");
        break;
    case Level::Debug:
        l.append("DEBUG");
        break;
    case Level::Warn:
        l.append("WARN");
        break;
    case Level::Error:
        l.append("ERROR");
        break;
    }

    return {level, fmt::format("[time] {} {}", l, msg)};
};

std::vector<Log::Message> Log::get_messages()
{
    std::vector<Message> result;
    {
        std::lock_guard<std::mutex> lock(mutex);
        result = std::move(messages);
    }
    return result;
}

void Log::apply_buf(std::vector<Message>& msgs)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto&& m : msgs) { messages.emplace_back(std::move(m)); }
    }
}
} // namespace lua_sftp
