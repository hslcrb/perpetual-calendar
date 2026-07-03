/// <reference types="vite/client" />

declare module '*/calendar.js' {
  import type { CalendarModule } from './wasm/calendar.d.ts';
  const createModule: () => Promise<CalendarModule>;
  export default createModule;
}
