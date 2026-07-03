CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
GTKCFLAGS = $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTKLIBS = $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

# Linux GTK3 build
linux: calendar_core.o main_gtk.o
	$(CXX) $(CXXFLAGS) -o perpetual-calendar calendar_core.o main_gtk.o $(GTKLIBS)

main_gtk.o: main_gtk.cpp calendar_core.h
	$(CXX) $(CXXFLAGS) $(GTKCFLAGS) -c main_gtk.cpp

calendar_core.o: calendar_core.cpp calendar_core.h
	$(CXX) $(CXXFLAGS) -c calendar_core.cpp

# Windows cross-compile with MinGW64 (static link = single binary)
CXX_WIN = x86_64-w64-mingw32-g++
CXXFLAGS_WIN = -std=c++11 -O2 -Wall -Wextra -static -DWIN32_LEAN_AND_MEAN
LIBS_WIN = -lcomctl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32

win: calendar_core_win.o main_win.o
	$(CXX_WIN) $(CXXFLAGS_WIN) -o perpetual-calendar.exe calendar_core_win.o main_win.o $(LIBS_WIN)
	@echo "=== Windows binary created: perpetual-calendar.exe ==="
	@x86_64-w64-mingw32-strip perpetual-calendar.exe

main_win.o: main_win.cpp calendar_core.h
	$(CXX_WIN) $(CXXFLAGS_WIN) -c main_win.cpp -o main_win.o

calendar_core_win.o: calendar_core.cpp calendar_core.h
	$(CXX_WIN) $(CXXFLAGS_WIN) -c calendar_core.cpp -o calendar_core_win.o

# Clean
clean:
	rm -f *.o perpetual-calendar perpetual-calendar.exe

.PHONY: linux win clean all

all: linux win
	@echo "=== Linux: perpetual-calendar ==="
	@ls -la perpetual-calendar 2>/dev/null || echo "(not built)"
	@echo "=== Windows: perpetual-calendar.exe ==="
	@ls -la perpetual-calendar.exe 2>/dev/null || echo "(not built)"
