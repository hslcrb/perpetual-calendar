import React, { useState } from 'react';
import MonthView from './components/MonthView';
import { EditDialog } from './components/EditDialog';
import { useCalendar } from './hooks/useCalendar';
import './Calendar.css';

const App: React.FC = () => {
  const cal = useCalendar();
  const [editSerial, setEditSerial] = useState<number | null>(null);
  const [loading, setLoading] = useState(true);

  // Store module reference for EditDialog date decoding
  React.useEffect(() => {
    if (cal.mod.current) {
      (window as any).__calendarMod = cal.mod.current;
      setLoading(false);
    }
  }, [cal.mod.current]);

  const yearLabel = cal.startMonth === 1
    ? `${cal.year}년 (상반기)`
    : `${cal.year}년 (하반기)`;

  // Calculate month display for each of 6 slots
  const getMonthInfo = (idx: number) => {
    let m = cal.startMonth + idx;
    let y = cal.year;
    if (m > 12) { m -= 12; y++; }
    return { year: y, month: m };
  };

  if (loading) {
    return (
      <div className="loading">
        <div className="loading-text">달력 엔진 로딩 중...</div>
        <div className="loading-bar"><div className="loading-fill" /></div>
      </div>
    );
  }

  return (
    <div className="app">
      <div className="toolbar">
        <button className="nav-btn" onClick={cal.movePrevQuarter}>
          ◀ 이전 분기
        </button>
        <h2 className="year-label">{yearLabel}</h2>
        <button className="nav-btn" onClick={cal.moveNextQuarter}>
          다음 분기 ▶
        </button>
      </div>
      <div className="calendar-grid">
        {[0, 1, 2, 3, 4, 5].map((i) => {
          const { year: y, month: m } = getMonthInfo(i);
          return (
            <MonthView
              key={i}
              year={y}
              month={m}
              days={cal.months[i]}
              onDayDoubleClick={(serial) => setEditSerial(serial)}
            />
          );
        })}
      </div>
      <div className="status-bar">
        일정은 날짜를 더블클릭하여 추가/수정
      </div>

      {editSerial !== null && (
        <EditDialog
          serial={editSerial}
          currentText={cal.getSchedule(editSerial)}
          onSave={(serial, text) => cal.setSchedule(serial, text)}
          onClose={() => setEditSerial(null)}
        />
      )}
    </div>
  );
};

export default App;
