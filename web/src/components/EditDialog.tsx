import React from 'react';
import { CalendarDay } from '../wasm/calendar.d.ts';

interface EditDialogProps {
  serial: number;
  currentText: string;
  onSave: (serial: number, text: string) => void;
  onClose: () => void;
}

export const EditDialog: React.FC<EditDialogProps> = ({ serial, currentText, onSave, onClose }) => {
  const [text, setText] = React.useState(currentText);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSave(serial, text);
    onClose();
  };

  // Decode serial number to date for display
  const mod = (window as any).__calendarMod;
  let dateStr = '';
  if (mod && mod.serial_to_date) {
    const d = mod.serial_to_date(serial);
    if (d) dateStr = `${d.year}년 ${d.month}월 ${d.day}일`;
  }

  return (
    <div className="dialog-overlay" onClick={onClose}>
      <div className="dialog" onClick={(e) => e.stopPropagation()}>
        <h3>{dateStr || `${serial}`} 일정 수정</h3>
        <form onSubmit={handleSubmit}>
          <input
            type="text"
            value={text}
            onChange={(e) => setText(e.target.value)}
            placeholder="일정을 입력하세요"
            autoFocus
          />
          <div className="dialog-buttons">
            <button type="submit" className="btn-primary">저장</button>
            <button type="button" className="btn-secondary" onClick={onClose}>취소</button>
          </div>
        </form>
      </div>
    </div>
  );
};
