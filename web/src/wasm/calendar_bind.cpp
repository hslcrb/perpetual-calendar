#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <calendar_core.h>

using namespace emscripten;

// Wrapper that returns calendar data as a JS object
val get_calendar_data_js(int year, int start_month) {
    CalendarApp app;
    app.set_year(year);
    app.set_start_month(start_month);
    
    auto& months = app.get_calendar_data();
    val result = val::object();
    
    for (int mi = 0; mi < 6; mi++) {
        val month_arr = val::array();
        auto& days = months[mi];
        
        for (size_t di = 0; di < days.size(); di++) {
            val day_obj = val::object();
            auto& d = days[di];
            day_obj.set("serial", d.serial);
            day_obj.set("day", d.day);
            day_obj.set("month", d.month);
            day_obj.set("year", d.year);
            day_obj.set("dow", d.dow);
            day_obj.set("dow_str", d.dow_str);
            day_obj.set("schedule", d.schedule);
            day_obj.set("is_holiday", d.is_holiday);
            day_obj.set("holiday_name", d.holiday_name);
            month_arr.call<void>("push", day_obj);
        }
        
        char key[8];
        snprintf(key, sizeof(key), "m%d", mi);
        result.set(std::string(key), month_arr);
    }
    
    return result;
}

val serial_to_date_js(int serial) {
    struct tm t = serial_to_date(serial);
    val obj = val::object();
    obj.set("year", t.tm_year + 1900);
    obj.set("month", t.tm_mon + 1);
    obj.set("day", t.tm_mday);
    obj.set("dow", t.tm_wday);
    return obj;
}

int date_to_serial_js(int year, int month, int day) {
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = 12;
    return date_to_serial(t);
}

val get_korean_holidays_js(int year) {
    auto holidays = CalendarApp::get_korean_holidays(year);
    val obj = val::object();
    for (auto it = holidays.begin(); it != holidays.end(); ++it) {
        obj.set(std::to_string(it->first), it->second);
    }
    return obj;
}

EMSCRIPTEN_BINDINGS(calendar_module) {
    function("get_calendar_data", &get_calendar_data_js);
    function("serial_to_date", &serial_to_date_js);
    function("date_to_serial", &date_to_serial_js);
    function("get_korean_holidays", &get_korean_holidays_js);
}
