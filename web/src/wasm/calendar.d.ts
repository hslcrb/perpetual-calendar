declare module '*/calendar.js' {
  interface CalendarModule {
    get_calendar_data(year: number, start_month: number): CalendarData;
    serial_to_date(serial: number): DateInfo;
    date_to_serial(year: number, month: number, day: number): number;
    get_korean_holidays(year: number): Record<string, string>;
  }

  interface CalendarDay {
    serial: number;
    day: number;
    month: number;
    year: number;
    dow: number;
    dow_str: string;
    schedule: string;
    is_holiday: boolean;
    holiday_name: string;
  }

  interface CalendarData {
    m0: CalendarDay[];
    m1: CalendarDay[];
    m2: CalendarDay[];
    m3: CalendarDay[];
    m4: CalendarDay[];
    m5: CalendarDay[];
  }

  interface DateInfo {
    year: number;
    month: number;
    day: number;
    dow: number;
  }

  export default function createCalendarModule(): Promise<CalendarModule>;
}
