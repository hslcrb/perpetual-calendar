#include "calendar_core.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

// Excel serial number to date
// Excel 1900 date system: serial 1 = 1900-01-01
// Includes the Lotus 123 bug (Feb 29, 1900 exists)
struct tm serial_to_date(int serial) {
    struct tm result; memset(&result, 0, sizeof(result));
    // Excel epoch: 1899-12-30 (serial 0)
    // Using C time functions
    struct tm base; memset(&base, 0, sizeof(base));
    base.tm_year = 1899 - 1900; // 1899
    base.tm_mon = 11;           // December
    base.tm_mday = 30;
    base.tm_hour = 12;          // avoid date overflow issues
    
    time_t base_time = mktime(&base);
    time_t target_time = base_time + (serial - 1) * 86400;
    
    struct tm* ts = localtime(&target_time);
    if (ts) {
        result = *ts;
    }
    return result;
}

int date_to_serial(struct tm date) {
    struct tm base; memset(&base, 0, sizeof(base));
    base.tm_year = 1899 - 1900;
    base.tm_mon = 11;
    base.tm_mday = 30;
    base.tm_hour = 12;
    
    time_t base_time = mktime(&base);
    date.tm_hour = 12;
    time_t target_time = mktime(&date);
    
    double diff_sec = difftime(target_time, base_time);
    int serial = (int)(diff_sec / 86400.0 + 0.5);
    
    // Adjust for Excel's Lotus 123 bug (Feb 29, 1900)
    struct tm feb28_1900; memset(&feb28_1900, 0, sizeof(feb28_1900));
    feb28_1900.tm_year = 1900 - 1900;
    feb28_1900.tm_mon = 1;
    feb28_1900.tm_mday = 28;
    
    struct tm mar1_1900; memset(&mar1_1900, 0, sizeof(mar1_1900));
    mar1_1900.tm_year = 1900 - 1900;
    mar1_1900.tm_mon = 2;
    mar1_1900.tm_mday = 1;
    
    time_t t_feb = mktime(&feb28_1900);
    time_t t_mar = mktime(&mar1_1900);
    
    if (target_time > t_feb && target_time < t_mar) {
        return serial + 1; // Feb 29, 1900 doesn't actually exist
    }
    if (target_time >= t_mar) {
        return serial + 1;
    }
    
    return serial;
}

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int year, int month) {
    static const int days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month - 1];
}

// Zeller's congruence for day of week (0=Sat, 1=Sun, ..., 6=Fri)
// Converting to: 0=Sun, 1=Mon, ..., 6=Sat
static int day_of_week(int year, int month, int day) {
    if (month < 3) { month += 12; year--; }
    int q = day;
    int m = month;
    int k = year % 100;
    int j = year / 100;
    int h = (q + (13*(m+1))/5 + k + k/4 + j/4 + 5*j) % 7;
    // Zeller: 0=Sat, 1=Sun, 2=Mon, 3=Tue, 4=Wed, 5=Thu, 6=Fri
    // Convert to 0=Sun, 1=Mon, ..., 6=Sat
    return (h + 6) % 7;
}

std::map<int, std::string> CalendarApp::get_korean_holidays(int year) {
    std::map<int, std::string> holidays;
    // Fixed-date holidays
    auto add = [&](int m, int d, const std::string& name) {
        struct tm tm; memset(&tm, 0, sizeof(tm));
        tm.tm_year = year - 1900;
        tm.tm_mon = m - 1;
        tm.tm_mday = d;
        int serial = date_to_serial(tm);
        holidays[serial] = name;
    };
    
    add(1, 1, "신정");
    add(3, 1, "3·1절");
    add(5, 5, "어린이날");
    add(6, 6, "현충일");
    add(8, 15, "광복절");
    add(10, 3, "개천절");
    add(10, 9, "한글날");
    add(12, 25, "성탄절");
    
    // Lunar holidays for common years (approximate)
    // These vary each year; for a production app, use a library
    // 2025-2026 approximate dates
    if (year == 2025) {
        add(1, 28, "설날");  // approximate
        add(1, 29, "설날");
        add(1, 30, "설날");
        add(5, 5, "석가탄신일"); // approximate
        add(10, 5, "추석"); // approximate
        add(10, 6, "추석");
        add(10, 7, "추석");
    } else if (year == 2026) {
        add(2, 16, "설날");
        add(2, 17, "설날");
        add(2, 18, "설날");
        add(5, 24, "석가탄신일");
        add(9, 24, "추석");
        add(9, 25, "추석");
        add(9, 26, "추석");
    } else if (year == 2027) {
        add(2, 6, "설날");
        add(2, 7, "설날");
        add(2, 8, "설날");
        add(5, 13, "석가탄신일");
        add(10, 13, "추석");
        add(10, 14, "추석");
        add(10, 15, "추석");
    } else if (year == 2028) {
        add(1, 26, "설날");
        add(1, 27, "설날");
        add(1, 28, "설날");
        add(6, 3, "석가탄신일");
        add(9, 2, "추석");
        add(9, 3, "추석");
        add(9, 4, "추석");
    } else if (year == 2029) {
        add(2, 13, "설날");
        add(2, 14, "설날");
        add(2, 15, "설날");
        add(5, 23, "석가탄신일");
        add(10, 21, "추석");
        add(10, 22, "추석");
        add(10, 23, "추석");
    } else if (year == 2030) {
        add(2, 2, "설날");
        add(2, 3, "설날");
        add(2, 4, "설날");
        add(5, 12, "석가탄신일");
        add(9, 10, "추석");
        add(9, 11, "추석");
        add(9, 12, "추석");
    } else {
        // Fallback: approximate Seollal (late Jan/early Feb) and Chuseok (Sep/Oct)
        add(1, 29, "설날");
        add(1, 30, "설날");
        add(1, 31, "설날");
        add(9, 28, "추석");
        add(9, 29, "추석");
        add(9, 30, "추석");
    }
    
    return holidays;
}

CalendarApp::CalendarApp()
    : year_(2025), start_month_(1) {
    rebuild_calendar();
}

void CalendarApp::set_year(int year) {
    year_ = year;
    rebuild_calendar();
}

void CalendarApp::set_start_month(int month) {
    start_month_ = month;
    rebuild_calendar();
}

void CalendarApp::move_next_quarter() {
    if (start_month_ == 1) {
        start_month_ = 7;
    } else {
        start_month_ = 1;
        year_++;
    }
    rebuild_calendar();
}

void CalendarApp::move_prev_quarter() {
    if (start_month_ == 7) {
        start_month_ = 1;
    } else {
        start_month_ = 7;
        year_--;
    }
    rebuild_calendar();
}

void CalendarApp::rebuild_calendar() {
    months_.clear();
    
    auto holidays = get_korean_holidays(year_);
    // Also get holidays for next year for Jul-Dec quarter
    auto holidays_next = get_korean_holidays(year_ + 1);
    
    int base_month = start_month_; // 1 or 7
    
    for (int mi = 0; mi < 6; mi++) {
        int m = base_month + mi;
        int y = year_;
        if (m > 12) { m -= 12; y++; }
        
        int dim = days_in_month(y, m);
        // Find first day of week for this month's 1st
        int first_dow = day_of_week(y, m, 1);
        
        std::vector<CalendarDay> days;
        days.resize(dim);
        
        for (int d = 0; d < dim; d++) {
            CalendarDay cd;
            cd.day = d + 1;
            cd.month = m;
            cd.year = y;
            cd.dow = (first_dow + d) % 7;
            static const char* DOW_KOREAN[7] = {"\xEC\x9D\xBC","\xEC\x9B\x94","\xED\x99\x94",
                                                   "\xEC\x88\x98","\xEB\xAA\xA9","\xEA\xB8\x88","\xED\x86\xA0"};
            cd.dow_str = DOW_KOREAN[cd.dow];
            cd.schedule = "";
            cd.is_holiday = false;
            cd.holiday_name = "";
            
            struct tm tm; memset(&tm, 0, sizeof(tm));
            tm.tm_year = y - 1900;
            tm.tm_mon = m - 1;
            tm.tm_mday = d + 1;
            cd.serial = date_to_serial(tm);
            
            // Check schedule
            auto sit = schedule_.find(cd.serial);
            if (sit != schedule_.end()) {
                cd.schedule = sit->second;
            }
            
            // Check holiday
            auto hit = holidays.find(cd.serial);
            if (hit != holidays.end()) {
                cd.is_holiday = true;
                cd.holiday_name = hit->second;
            }
            if (hit == holidays.end()) {
                auto hit2 = holidays_next.find(cd.serial);
                if (hit2 != holidays_next.end()) {
                    cd.is_holiday = true;
                    cd.holiday_name = hit2->second;
                }
            }
            
            days[d] = cd;
        }
        
        months_.push_back(days);
    }
}

void CalendarApp::set_schedule(int serial, const std::string& text) {
    if (text.empty()) {
        schedule_.erase(serial);
    } else {
        schedule_[serial] = text;
    }
    rebuild_calendar();
}

std::string CalendarApp::get_schedule(int serial) const {
    auto it = schedule_.find(serial);
    if (it != schedule_.end()) return it->second;
    return "";
}

void CalendarApp::load_schedule(const std::string& filename) {
    schedule_.clear();
    std::ifstream f(filename);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        size_t comma = line.find(',');
        if (comma == std::string::npos) continue;
        int serial = std::stoi(line.substr(0, comma));
        std::string text = line.substr(comma + 1);
        schedule_[serial] = text;
    }
    rebuild_calendar();
}

void CalendarApp::save_schedule(const std::string& filename) {
    std::ofstream f(filename);
    for (auto& kv : schedule_) {
        f << kv.first << "," << kv.second << "\n";
    }
}

void CalendarApp::load_holidays(const std::string& filename) {
    // Load custom holidays from file (same format: serial, name)
    std::ifstream f(filename);
    if (!f) return;
    (void)f;
    // Custom holidays from file not yet integrated;
    // Korean holidays are defined programmatically in get_korean_holidays()
}
