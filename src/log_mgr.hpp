#ifndef YKM22_LUA_SFTP_SFTP_LOG_MGR_HPP
#define YKM22_LUA_SFTP_SFTP_LOG_MGR_HPP

#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <fmt/format.h>

namespace lua_sftp
{

class Log
{
  public:
    enum class Level
    {
        Info,
        Debug,
        Warn,
        Error,
    };

    struct Message
    {
        Level level;
        std::string message;
    };
    static Log& Ins();

    static Message build_msg(Level level, std::string_view msg);

    class Buf
    {
      public:
        template <typename... T> inline Buf& info(fmt::format_string<T...> fmt, T&&... args)
        {
            messages.emplace_back(build_msg(Level::Info, fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}})));
            return *this;
        }
        template <typename... T> inline Buf& debug(fmt::format_string<T...> fmt, T&&... args)
        {
            messages.emplace_back(build_msg(Level::Debug, fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}})));
            return *this;
        }
        template <typename... T> inline Buf& warn(fmt::format_string<T...> fmt, T&&... args)
        {
            messages.emplace_back(build_msg(Level::Warn, fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}})));
            return *this;
        }
        template <typename... T> inline Buf& error(fmt::format_string<T...> fmt, T&&... args)
        {
            messages.emplace_back(build_msg(Level::Error, fmt::vformat(fmt.str, fmt::vargs<T...>{{args...}})));
            return *this;
        }

        inline void apply()
        {
            Ins().apply_buf(messages);
            messages.clear();
        }

      private:
        std::vector<Message> messages;
    };

    std::vector<Message> get_messages();
    

    void apply_buf(std::vector<Message>& msgs);

  private:
    std::mutex mutex;
    std::vector<Message> messages;
};

} // namespace lua_sftp
#endif
