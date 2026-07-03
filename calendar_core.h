#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>

// Excel serial date number to struct tm
// Excel epoch: 1900-01-01 = serial 1 (with the Lotus 123 bug: 1900-02-29 exists)
struct tm serial_to_date(int serial);
int date_to_serial(struct tm date);

struct CalendarDay {
    int serial;       // Excel serial date
    int day;          // day of month (1-31)
    int month;        // month (1-12)
    int year;         // year
    int dow;          // day of week (0=Sun, 1=Mon, ..., 6=Sat)
    std::string dow_str; // Korean day name
    std::string schedule; // schedule text
    bool is_holiday;
    std::string holiday_name;
};

struct CalendarQuarter {
    int year;
    int start_month; // 1 or 7
    std::vector<std::vector<CalendarDay>> months; // 6 months worth
};

class CalendarApp {
public:
    CalendarApp();
    
    void load_schedule(const std::string& filename);
    void save_schedule(const std::string& filename);
    void load_holidays(const std::string& filename);
    
    void set_year(int year);
    void set_start_month(int month); // 1 or 7
    int get_year() const { return year_; }
    int get_start_month() const { return start_month_; }
    
    void move_next_quarter();
    void move_prev_quarter();
    
    void set_schedule(int serial, const std::string& text);
    std::string get_schedule(int serial) const;
    
    const std::vector<std::vector<CalendarDay>>& get_calendar_data() const { return months_; }
    
    
    // Predefined South Korean holidays
    static std::map<int, std::string> get_korean_holidays(int year);

private:
    int year_;
    int start_month_; // 1 or 7
    std::vector<std::vector<CalendarDay>> months_; // 6 months
    std::map<int, std::string> schedule_; // serial -> schedule text
    
    void rebuild_calendar();
};
