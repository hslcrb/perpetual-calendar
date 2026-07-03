import React from 'react';
import { CalendarDay } from '../wasm/calendar.d.ts';

interface MonthViewProps {
  year: number;
  month: number;
  days: CalendarDay[];
  onDayDoubleClick: (serial: number) => void;
}

const DOW = ['일', '월', '화', '수', '목', '금', '토'];
const MONTH_NAMES = ['1월', '2월', '3월', '4월', '5월', '6월', '7월', '8월', '9월', '10월', '11월', '12월'];

const MonthView: React.FC<MonthViewProps> = ({ year, month, days, onDayDoubleClick }) => {
  if (days.length === 0) return null;
  const firstDow = days[0].dow;
  const totalCells = firstDow + days.length;
  const weeks = Math.ceil(totalCells / 7);
  const grid: (CalendarDay | null)[] = [];

  for (let i = 0; i < firstDow; i++) grid.push(null);
  for (const d of days) grid.push(d);
  while (grid.length < weeks * 7) grid.push(null);

  return (
    <div className="month-frame">
      <div className="month-header">{year}년 {MONTH_NAMES[month - 1]}</div>
      <div className="dow-row">
        {DOW.map((d, i) => (
          <div key={i} className={`dow-cell dow-${i}`}>{d}</div>
        ))}
      </div>
      <div className="days-grid">
        {grid.map((day, idx) => {
          if (!day) return <div key={idx} className="day-cell day-blank" />;
          const isSun = day.dow === 0;
          const isSat = day.dow === 6;
          const isHoliday = day.is_holiday;
          const cls = [
            'day-cell',
            isHoliday ? 'day-holiday' : isSun ? 'day-sun' : isSat ? 'day-sat' : 'day-normal',
          ].join(' ');

          const displayText = day.is_holiday
            ? day.holiday_name
            : day.schedule.length > 15
              ? day.schedule.slice(0, 15) + '..'
              : day.schedule;

          return (
            <div
              key={idx}
              className={cls}
              onDoubleClick={() => onDayDoubleClick(day.serial)}
              title={`${day.year}년 ${day.month}월 ${day.day}일 (${day.dow_str})${day.schedule ? '\n' + day.schedule : ''}`}
            >
              <span className={`day-num dow-${day.dow}`}>{day.day}</span>
              {displayText && (
                <span className={`day-sched ${isHoliday ? 'sched-holiday' : 'sched-normal'}`}>
                  {displayText}
                </span>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
};

export default MonthView;
