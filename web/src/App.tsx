import React, { useState } from 'react';
import MonthView from './components/MonthView';
import { EditDialog } from './components/EditDialog';
import { useCalendar } from './hooks/useCalendar';
import './Calendar.css';

const SPLASH_DURATION = 3000;
const FADE_DURATION = 500;

const App: React.FC = () => {
  const cal = useCalendar();
  const [editSerial, setEditSerial] = useState<number | null>(null);
  const [wasmReady, setWasmReady] = useState(false);
  const [splashDone, setSplashDone] = useState(false);
  const [fadeIn, setFadeIn] = useState(false);

  React.useEffect(() => {
    if (cal.mod.current) {
      (window as any).__calendarMod = cal.mod.current;
      setWasmReady(true);
    }
  }, [cal.mod.current]);

  React.useEffect(() => {
    if (wasmReady) {
      const splashTimer = setTimeout(() => setSplashDone(true), SPLASH_DURATION);
      return () => clearTimeout(splashTimer);
    }
  }, [wasmReady]);

  React.useEffect(() => {
    if (splashDone) {
      const fadeTimer = setTimeout(() => setFadeIn(true), 50);
      return () => clearTimeout(fadeTimer);
    }
  }, [splashDone]);

  if (!splashDone) {
    return (
      <div style={{
        position: 'fixed', inset: 0,
        display: 'flex', flexDirection: 'column',
        alignItems: 'center', justifyContent: 'center',
        background: '#fff', zIndex: 9999,
        fontFamily: 'system-ui, -apple-system, sans-serif',
      }}>
        <div style={{
          fontSize: 24, color: '#1f91df',
          marginBottom: 12, fontWeight: 400,
        }}>
          현재 데모 상태입니다.
        </div>
        <div style={{
          fontSize: 16, color: '#0e1e52',
          fontWeight: 400,
        }}>
          방문해주셔서 감사합니다.
        </div>
      </div>
    );
  }

  return (
    <div className="app" style={{
      opacity: fadeIn ? 1 : 0,
      transition: `opacity ${FADE_DURATION}ms ease-in-out`,
    }}>
      <div className="toolbar">
        <button className="nav-btn" onClick={cal.movePrevQuarter}>
          ◀ 이전 분기
        </button>
        <h2 className="year-label">
          {cal.startMonth === 1 ? `${cal.year}년 (상반기)` : `${cal.year}년 (하반기)`}
        </h2>
        <button className="nav-btn" onClick={cal.moveNextQuarter}>
          다음 분기 ▶
        </button>
      </div>
      <div className="calendar-grid">
        {[0, 1, 2, 3, 4, 5].map((i) => {
          let m = cal.startMonth + i;
          let y = cal.year;
          if (m > 12) { m -= 12; y++; }
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
