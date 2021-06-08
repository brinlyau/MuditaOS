// Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "time_conversion.hpp"
#include <algorithm>
#include <array>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <locale>
#include "i18n/i18n.hpp"

#include "time_locale.hpp"
#include "DateAndTimeSettings.hpp"
#include <Utils.hpp>

namespace utils::time
{
    namespace
    {
        constexpr auto verboseConversion = false; // debug switch

        constexpr auto abbrev_len = 3U;

        /// order matters, it's used in replace_locale with enum Replacements
        const std::vector<std::string> specifiers_replacement = {"%a",  // day abbrew
                                                                 "%A",  // day long
                                                                 "%b",  // month abbrew
                                                                 "%B",  // month long
                                                                 "%Z"}; // timezone
        constexpr auto hoursMinFormat12H                      = "%I:%M";
        constexpr auto hoursMinFormat24H                      = "%H:%M";

        struct Format
        {
            const std::string lowFormat;  /// format used if duration is below 1 hour
            const std::string highFormat; /// format used if duration exceeds 1 hour
        };
        constexpr auto durationFormatH0M0S = "duration_hour_0min_0sec";
        constexpr auto durationFormat0M0S  = "duration_0min_0sec";
        constexpr auto durationFormat0N0S  = "duration_0hmin_0sec";
        constexpr auto durationFormatM0S   = "duration_min_0sec";

        using FormatMap           = std::map<const Duration::DisplayedFormat, const Format>;
        const FormatMap formatMap = {
            {Duration::DisplayedFormat::Fixed0M0S, {durationFormat0N0S, durationFormat0N0S}},
            {Duration::DisplayedFormat::FixedH0M0S, {durationFormatH0M0S, durationFormatH0M0S}},
            {Duration::DisplayedFormat::AutoM, {durationFormatM0S, durationFormatH0M0S}},
            {Duration::DisplayedFormat::Auto0M, {durationFormat0M0S, durationFormatH0M0S}}};
    } // namespace

    Locale tlocale;

    UTF8 Localer::get_replacement(Replacements val, const struct tm &timeinfo)
    {
        UTF8 retval = "";
        switch (val) {
        case DayLong:
            retval = Locale::get_day(Locale::Day(timeinfo.tm_wday));
            break;
        case DayAbbrev:
            retval = Locale::get_day(Locale::Day(timeinfo.tm_wday)).substr(0, abbrev_len);
            break;
        case MonthLong:
            retval = Locale::get_month(Locale::Month(timeinfo.tm_mon));
            break;
        case MonthAbbrev:
            retval = Locale::get_month(Locale::Month(timeinfo.tm_mon)).substr(0, abbrev_len);
            break;
        case Timezone:
            break;
        }
        return retval;
    }

    Timestamp::Timestamp()
    {
        time     = 0;
        timeinfo = *std::localtime(&time);
    }

    void Timestamp::set_time(time_t newtime)
    {
        time     = newtime;
        timeinfo = *localtime(&time);
    }

    void Timestamp::set_time(std::string timestr, const char *fmt)
    {

        std::stringstream stream(timestr);
        try {
            stream >> std::get_time(&(this->timeinfo), "%y/%m/%d %H:%M:%S");

            // add missing years, otherwise mktime returns bad timestamp
            timeinfo.tm_year += 100;
            this->time = mktime(&timeinfo);
        }
        catch (std::exception &e) {
            LOG_ERROR("Time::set_time error %s", e.what());
        }
    }

    constexpr uint32_t datasize = 128;
    UTF8 Timestamp::str(std::string fmt)
    {
        if (fmt.compare("") != 0) {
            this->format = fmt;
        }
        UTF8 datetimestr = "";
        auto replaceFunc = [&](int idx) { return get_replacement(Replacements(idx), timeinfo); };
        utils::findAndReplaceAll(this->format, specifiers_replacement, replaceFunc);
        auto data = std::unique_ptr<char[]>(new char[datasize]);
        std::strftime(data.get(), datasize, this->format.c_str(), &timeinfo);
        datetimestr = UTF8(data.get());
        return datetimestr;
    }

    UTF8 Timestamp::day(bool abbrev)
    {
        if (abbrev) {
            return get_replacement(Replacements::DayAbbrev, timeinfo);
        }
        else {
            return get_replacement(Replacements::DayLong, timeinfo);
        }
    }

    UTF8 Timestamp::month(bool abbrev)
    {
        if (abbrev) {
            return get_replacement(Replacements::MonthAbbrev, timeinfo);
        }
        else {
            return get_replacement(Replacements::MonthLong, timeinfo);
        }
    }

    Duration operator-(const Timestamp &lhs, const Timestamp &rhs)
    {
        return Duration(lhs.time, rhs.time);
    };
    Timestamp operator-(const Timestamp &lhs, const Duration &rhs)
    {
        return Timestamp(lhs.time < rhs.duration ? 0 : lhs.time - rhs.duration);
    };
    Timestamp operator+(const Timestamp &lhs, const Duration &rhs)
    {
        return Timestamp(lhs.time + rhs.duration);
    };
    Timestamp operator+(const Duration &lhs, const Timestamp &rhs)
    {
        return Timestamp(lhs.duration + rhs.time);
    }

    void DateTime::before_n_sec(time_t val)
    {
        local_time = time;
        if (val) {
            set_time(val);
        }
    }

    bool DateTime::isToday()
    {
        auto newer_timeinfo = *localtime(&local_time);
        return (newer_timeinfo.tm_yday == timeinfo.tm_yday && newer_timeinfo.tm_year == timeinfo.tm_year);
    }

    bool DateTime::isYesterday()
    {
        auto newer_timeinfo = *localtime(&local_time);
        bool is_leap_year   = (timeinfo.tm_year % 4 == 0 && timeinfo.tm_year % 100 != 0) || timeinfo.tm_year % 400 == 0;

        return (((newer_timeinfo.tm_yday - timeinfo.tm_yday == 1) &&
                 (newer_timeinfo.tm_year == timeinfo.tm_year)) // day difference
                ||
                (timeinfo.tm_year == 0 && newer_timeinfo.tm_year + 364 + is_leap_year) // day next year day difference
        );
    }

    UTF8 DateTime::str(std::string format)
    {
        if (format.compare("") != 0) {
            return Timestamp::str(format);
        }
        if (isToday()) // if the same computer day, then return hour.
        {
            return Timestamp::str(Locale::format(Locale::TimeFormat::FormatTime12H));
        }
        else if (show_textual_past && isYesterday()) {
            return Locale::yesterday();
        }
        else {
            return Timestamp::str(Locale::format(date_format_long ? Locale::TimeFormat::FormatLocaleDateFull
                                                                  : Locale::TimeFormat::FormatLocaleDateShort));
        }
    }
    UTF8 Timestamp::get_date_time_substr(GetParameters param)
    {
        auto value = get_date_time_sub_value(param);
        if (value != UINT32_MAX) {
            return UTF8(std::to_string(value));
        }
        return UTF8();
    }

    uint32_t Timestamp::get_date_time_sub_value(GetParameters param)
    {
        switch (param) {
        case GetParameters::Hour:
            return timeinfo.tm_hour;
            break;
        case GetParameters::Minute:
            return timeinfo.tm_min;
            break;
        case GetParameters::Day:
            return timeinfo.tm_mday;
            break;
        case GetParameters::Month:
            return timeinfo.tm_mon + 1;
            break;
        case GetParameters::Year:
            return timeinfo.tm_year + 1900;
            break;
        }
        return UINT32_MAX;
    }
    uint32_t Timestamp::get_UTC_date_time_sub_value(GetParameters param)
    {
        std::tm tm = *std::gmtime(&time);
        switch (param) {
        case GetParameters::Hour:
            return tm.tm_hour;
        case GetParameters::Minute:
            return tm.tm_min;
        case GetParameters::Day:
            return tm.tm_mday;
        case GetParameters::Month:
            return tm.tm_mon + 1;
        case GetParameters::Year:
            return tm.tm_year + 1900;
        }
        return UINT32_MAX;
    }

    UTF8 Date::str(std::string format)
    {
        if (!format.empty()) {
            return Timestamp::str(format);
        }
        else if (show_textual_past) {
            if (isToday()) {
                return Locale::today();
            }
            else if (isYesterday()) {
                return Locale::yesterday();
            }
        }

        return Timestamp::str(Locale::format(date_format_long ? Locale::TimeFormat::FormatLocaleDateFull
                                                              : Locale::TimeFormat::FormatLocaleDateShort));
    }

    UTF8 Time::str(std::string format)
    {
        if (!format.empty()) {
            return Timestamp::str(format);
        }
        else {
            return Timestamp::str(Locale::format(
                Locale::TimeFormat::FormatTime12HShort)); // @TODO: M.G. FormatLocaleTime which actually works
        }
    }

    Duration::Duration(time_t duration) : duration(duration)
    {
        calculate();
    }

    Duration::Duration(time_t stop, time_t start) : Duration(stop - start > 0 ? stop - start : 0)
    {}

    Duration::Duration(const Timestamp &stop, const Timestamp &start) : Duration(stop.getTime(), start.getTime())
    {}

    void Duration::fillStr(std::string &format) const
    {
        constexpr auto numberOfLeadingDigits = 2;
        utils::findAndReplaceAll(format, "%H", utils::to_string(hours));
        utils::findAndReplaceAll(format, "%M", utils::to_string(minutes));
        utils::findAndReplaceAll(format, "%N", utils::to_string(hmminutes));
        utils::findAndReplaceAll(format, "%S", utils::to_string(seconds));
        utils::findAndReplaceAll(format, "%0H", utils::addLeadingZeros(utils::to_string(hours), numberOfLeadingDigits));
        utils::findAndReplaceAll(
            format, "%0M", utils::addLeadingZeros(utils::to_string(minutes), numberOfLeadingDigits));
        utils::findAndReplaceAll(
            format, "%0N", utils::addLeadingZeros(utils::to_string(hmminutes), numberOfLeadingDigits));
        utils::findAndReplaceAll(
            format, "%0S", utils::addLeadingZeros(utils::to_string(seconds), numberOfLeadingDigits));
    }

    void Duration::calculate()
    {
        hours     = this->duration / secondsInHour;
        hmminutes = this->duration / secondsInMinute;
        minutes   = (this->duration % secondsInHour) / secondsInMinute;
        seconds   = (this->duration % secondsInHour) % secondsInMinute;

        if (verboseConversion) {
            LOG_DEBUG("duration %" PRIu64 " - %lu hours %lu minutes %lu seconds", duration, hours, minutes, seconds);
        }
    }

    UTF8 Duration::str(DisplayedFormat displayedFormat) const
    {
        // switch between format low and hig
        std::string data = utils::translate(hours != 0 ? formatMap.at(displayedFormat).highFormat
                                                       : formatMap.at(displayedFormat).lowFormat);
        fillStr(data);

        return data;
    }

    Timestamp getCurrentTimestamp()
    {
        return Timestamp{std::time(nullptr)};
    }
}; // namespace utils::time