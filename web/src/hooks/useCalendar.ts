import { useState, useCallback, useEffect, useRef } from 'react';
import createCalendarModule from '../wasm/calendar.js';
import type { CalendarModule, CalendarData, CalendarDay } from '../wasm/calendar.d.ts';

interface CalendarState {
  year: number;
  startMonth: number;
  months: CalendarDay[][];
  schedule: Record<number, string>;
}

export function useCalendar() {
  const modRef = useRef<CalendarModule | null>(null);
  const [ready, setReady] = useState(false);
  const [state, setState] = useState<CalendarState>({
    year: new Date().getFullYear(),
    startMonth: 1,
    months: [[], [], [], [], [], []],
    schedule: loadSchedule(),
  });

  useEffect(() => {
    createCalendarModule().then((mod) => {
      modRef.current = mod;
      setReady(true);
      rebuild();
    });
  }, []);

  const rebuild = useCallback(() => {
    setState((prev) => {
      const mod = modRef.current;
      if (!mod) return prev;
      const data: CalendarData = mod.get_calendar_data(prev.year, prev.startMonth);
      const months: CalendarDay[][] = [];
      for (let i = 0; i < 6; i++) {
        const key = `m${i}` as keyof CalendarData;
        const days = data[key] as CalendarDay[];
        // Apply stored schedules
        for (const day of days) {
          if (prev.schedule[day.serial]) {
            day.schedule = prev.schedule[day.serial];
          }
        }
        months.push(days);
      }
      return { ...prev, months };
    });
  }, []);

  const setYear = useCallback((y: number) => {
    setState((prev) => ({ ...prev, year: y }));
    setTimeout(rebuild, 0);
  }, [rebuild]);

  const setStartMonth = useCallback((m: number) => {
    setState((prev) => ({ ...prev, startMonth: m }));
    setTimeout(rebuild, 0);
  }, [rebuild]);

  const movePrevQuarter = useCallback(() => {
    setState((prev) => {
      let sy = prev.year;
      let sm = prev.startMonth;
      if (sm === 1) { sm = 7; sy--; }
      else { sm = 1; }
      return { ...prev, year: sy, startMonth: sm };
    });
    setTimeout(rebuild, 0);
  }, [rebuild]);

  const moveNextQuarter = useCallback(() => {
    setState((prev) => {
      let sy = prev.year;
      let sm = prev.startMonth;
      if (sm === 7) { sm = 1; sy++; }
      else { sm = 7; }
      return { ...prev, year: sy, startMonth: sm };
    });
    setTimeout(rebuild, 0);
  }, [rebuild]);

  const setSchedule = useCallback((serial: number, text: string) => {
    setState((prev) => {
      const sched = { ...prev.schedule };
      if (text) sched[serial] = text;
      else delete sched[serial];
      saveSchedule(sched);
      return { ...prev, schedule: sched };
    });
    setTimeout(rebuild, 0);
  }, [rebuild]);

  const getSchedule = useCallback((serial: number): string => {
    return state.schedule[serial] || '';
  }, [state.schedule]);

  return {
    ready,
    year: state.year,
    startMonth: state.startMonth,
    months: state.months,
    schedule: state.schedule,
    setYear,
    setStartMonth,
    movePrevQuarter,
    moveNextQuarter,
    setSchedule,
    getSchedule,
    mod: modRef,
  };
}

function loadSchedule(): Record<number, string> {
  try {
    const raw = localStorage.getItem('calendar_schedule');
    if (!raw) return {};
    return JSON.parse(raw);
  } catch {
    return {};
  }
}

function saveSchedule(sched: Record<number, string>) {
  try {
    localStorage.setItem('calendar_schedule', JSON.stringify(sched));
  } catch {}
}
