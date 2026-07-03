#include "calendar_core.h"
#include <gtk/gtk.h>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>

CalendarApp g_app;

static GtkWidget* g_year_label = nullptr;
static GtkWidget* g_month_grid = nullptr;
static GtkWidget* g_scroll = nullptr;
static GtkWidget* g_edit_dialog = nullptr;
static GtkWidget* g_edit_entry = nullptr;
static int g_edit_serial = 0;

static void refresh_calendar();

static std::string month_name_korean(int m) {
    static const char* names[12] = {"1월","2월","3월","4월","5월","6월",
                                    "7월","8월","9월","10월","11월","12월"};
    return names[m-1];
}

static void on_prev_clicked(GtkButton*, gpointer) {
    g_app.move_prev_quarter();
    refresh_calendar();
}

static void on_next_clicked(GtkButton*, gpointer) {
    g_app.move_next_quarter();
    refresh_calendar();
}

static gboolean on_day_clicked(GtkWidget* w, GdkEventButton* ev, gpointer) {
    if (ev->type == GDK_2BUTTON_PRESS) {
        int* data = (int*)g_object_get_data(G_OBJECT(w), "serial");
        if (data) {
            g_edit_serial = *data;
            std::string text = g_app.get_schedule(g_edit_serial);
            if (g_edit_dialog) {
                gtk_window_present(GTK_WINDOW(g_edit_dialog));
            } else {
                auto day = serial_to_date(g_edit_serial);
                char title[128];
                snprintf(title, sizeof(title), "%d년 %d월 %d일 일정 수정",
                         day.tm_year+1900, day.tm_mon+1, day.tm_mday);
                g_edit_dialog = gtk_dialog_new_with_buttons(title, NULL, GTK_DIALOG_MODAL,
                    "취소", GTK_RESPONSE_REJECT,
                    "저장", GTK_RESPONSE_ACCEPT, NULL);
                gtk_window_set_default_size(GTK_WINDOW(g_edit_dialog), 400, 120);
                gtk_window_set_position(GTK_WINDOW(g_edit_dialog), GTK_WIN_POS_CENTER);
                GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(g_edit_dialog));
                g_edit_entry = gtk_entry_new();
                gtk_entry_set_text(GTK_ENTRY(g_edit_entry), text.c_str());
                gtk_widget_set_hexpand(g_edit_entry, TRUE);
                gtk_container_add(GTK_CONTAINER(content), g_edit_entry);
                g_signal_connect(g_edit_dialog, "response", G_CALLBACK(+[](GtkDialog* dlg, gint resp, gpointer){
                    if (resp == GTK_RESPONSE_ACCEPT) {
                        const char* t = gtk_entry_get_text(GTK_ENTRY(g_edit_entry));
                        g_app.set_schedule(g_edit_serial, t ? t : "");
                        refresh_calendar();
                    }
                    gtk_widget_destroy(GTK_WIDGET(dlg));
                    g_edit_dialog = nullptr;
                    g_edit_entry = nullptr;
                }), NULL);
                g_signal_connect(g_edit_dialog, "delete-event", G_CALLBACK(gtk_true), NULL);
                gtk_widget_show_all(g_edit_dialog);
            }
        }
    }
    return FALSE;
}

static GtkWidget* make_day_widget(const CalendarDay& day) {
    GtkWidget* eb = gtk_event_box_new();
    gtk_widget_set_size_request(eb, 170, 55);
    
    GdkRGBA bg;
    if (day.is_holiday) gdk_rgba_parse(&bg, "#FFE0E0");
    else if (day.dow == 0) gdk_rgba_parse(&bg, "#FFEEEE");
    else if (day.dow == 6) gdk_rgba_parse(&bg, "#EEEEFF");
    else gdk_rgba_parse(&bg, "#FFFFFF");
    gtk_widget_override_background_color(eb, GTK_STATE_FLAG_NORMAL, &bg);
    
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(eb), box);
    gtk_widget_set_margin_start(box, 2);
    
    GtkWidget* top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    char buf[16]; snprintf(buf, sizeof(buf), "%d", day.day);
    GtkWidget* dl = gtk_label_new(buf);
    PangoAttrList* a = pango_attr_list_new();
    pango_attr_list_insert(a, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(dl), a);
    pango_attr_list_unref(a);
    gtk_box_pack_start(GTK_BOX(top), dl, FALSE, FALSE, 0);
    
    GdkRGBA dc;
    if (day.dow == 0) gdk_rgba_parse(&dc, "#CC0000");
    else if (day.dow == 6) gdk_rgba_parse(&dc, "#0000CC");
    else gdk_rgba_parse(&dc, "#333333");
    GtkWidget* dow = gtk_label_new(day.dow_str.c_str());
    gtk_widget_override_color(dow, GTK_STATE_FLAG_NORMAL, &dc);
    gtk_box_pack_start(GTK_BOX(top), dow, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), top, FALSE, FALSE, 0);
    
    std::string display;
    if (day.is_holiday) display = day.holiday_name;
    else if (!day.schedule.empty()) {
        display = day.schedule;
        if (display.size() > 14) display = display.substr(0, 14) + "..";
    }
    if (!display.empty()) {
        GtkWidget* s = gtk_label_new(display.c_str());
        gtk_label_set_xalign(GTK_LABEL(s), 0.0);
        gtk_label_set_ellipsize(GTK_LABEL(s), PANGO_ELLIPSIZE_END);
        GdkRGBA sc;
        if (day.is_holiday) gdk_rgba_parse(&sc, "#CC0000");
        else gdk_rgba_parse(&sc, "#006600");
        gtk_widget_override_color(s, GTK_STATE_FLAG_NORMAL, &sc);
        PangoAttrList* sa = pango_attr_list_new();
        pango_attr_list_insert(sa, pango_attr_scale_new(PANGO_SCALE_SMALL));
        gtk_label_set_attributes(GTK_LABEL(s), sa);
        pango_attr_list_unref(sa);
        gtk_box_pack_start(GTK_BOX(box), s, FALSE, FALSE, 0);
    }
    
    int* sp = g_new(int, 1); *sp = day.serial;
    g_object_set_data_full(G_OBJECT(eb), "serial", sp, g_free);
    g_signal_connect(eb, "button-press-event", G_CALLBACK(on_day_clicked), NULL);
    
    char tip[256];
    auto t = serial_to_date(day.serial);
    snprintf(tip, sizeof(tip), "%d년 %d월 %d일 (%s)",
             t.tm_year+1900, t.tm_mon+1, t.tm_mday, day.dow_str.c_str());
    gtk_widget_set_tooltip_text(eb, tip);
    
    return eb;
}

static void refresh_calendar() {
    if (g_month_grid) gtk_widget_destroy(g_month_grid);
    
    g_month_grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(g_month_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(g_month_grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(g_month_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(g_month_grid), 2);
    
    auto& data = g_app.get_calendar_data();
    int year = g_app.get_year();
    int start_month = g_app.get_start_month();
    
    char ys[64];
    snprintf(ys, sizeof(ys), "%d년 (%s)", year,
             start_month == 1 ? "상반기" : "하반기");
    gtk_label_set_text(GTK_LABEL(g_year_label), ys);
    
    for (int mi = 0; mi < 6; mi++) {
        int m = start_month + mi;
        int y = year;
        if (m > 12) { m -= 12; y++; }
        
        GtkWidget* frame = gtk_frame_new(NULL);
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        char mh[32]; snprintf(mh, sizeof(mh), "%d년 %s", y, month_name_korean(m).c_str());
        GtkWidget* ml = gtk_label_new(mh);
        PangoAttrList* ba = pango_attr_list_new();
        pango_attr_list_insert(ba, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        pango_attr_list_insert(ba, pango_attr_size_new(13*PANGO_SCALE));
        gtk_label_set_attributes(GTK_LABEL(ml), ba);
        pango_attr_list_unref(ba);
        gtk_label_set_xalign(GTK_LABEL(ml), 0.5);
        gtk_box_pack_start(GTK_BOX(vbox), ml, FALSE, FALSE, 2);
        
        static const char* dows[7] = {"일","월","화","수","목","금","토"};
        for (int di = 0; di < 7; di++) {
            GtkWidget* dl = gtk_label_new(dows[di]);
            gtk_label_set_xalign(GTK_LABEL(dl), 0.5);
            GdkRGBA dc2;
            if (di==0) gdk_rgba_parse(&dc2, "#CC0000");
            else if (di==6) gdk_rgba_parse(&dc2, "#0000CC");
            else gdk_rgba_parse(&dc2, "#666666");
            gtk_widget_override_color(dl, GTK_STATE_FLAG_NORMAL, &dc2);
            PangoAttrList* da = pango_attr_list_new();
            pango_attr_list_insert(da, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
            pango_attr_list_insert(da, pango_attr_scale_new(PANGO_SCALE_SMALL));
            gtk_label_set_attributes(GTK_LABEL(dl), da);
            pango_attr_list_unref(da);
            gtk_widget_set_size_request(dl, 24, 18);
        }
        
        auto& days = data[mi];
        if (days.empty()) continue;
        int first_dow = days[0].dow;
        
        GtkWidget* dgrid = gtk_grid_new();
        
        for (size_t di = 0; di < days.size(); di++) {
            int idx = first_dow + (int)di;
            int row = idx / 7;
            int col = idx % 7;
            GtkWidget* dw = make_day_widget(days[di]);
            gtk_grid_attach(GTK_GRID(dgrid), dw, col, row, 1, 1);
        }
        
        gtk_box_pack_start(GTK_BOX(vbox), dgrid, TRUE, TRUE, 0);
        
        int grid_row = mi / 3;
        int grid_col = mi % 3;
        gtk_grid_attach(GTK_GRID(g_month_grid), frame, grid_col, grid_row, 1, 1);
    }
    
    if (g_scroll) {
        GtkWidget* child = gtk_bin_get_child(GTK_BIN(g_scroll));
        if (child) gtk_widget_destroy(child);
        gtk_container_add(GTK_CONTAINER(g_scroll), g_month_grid);
        gtk_widget_show_all(g_scroll);
    }
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    g_app.load_schedule("schedule.csv");
    
    GtkWidget* win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "만년 달력 - Perpetual Calendar");
    gtk_window_set_default_size(GTK_WINDOW(win), 1350, 820);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    g_signal_connect(win, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), vbox);
    
    GtkWidget* tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(tb, 8); gtk_widget_set_margin_end(tb, 8);
    gtk_widget_set_margin_top(tb, 6); gtk_widget_set_margin_bottom(tb, 6);
    
    GtkWidget* pbtn = gtk_button_new_with_label("◀ 이전 분기");
    g_signal_connect(pbtn, "clicked", G_CALLBACK(on_prev_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(tb), pbtn, FALSE, FALSE, 0);
    
    g_year_label = gtk_label_new("");
    PangoAttrList* ya = pango_attr_list_new();
    pango_attr_list_insert(ya, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(ya, pango_attr_size_new(20*PANGO_SCALE));
    gtk_label_set_attributes(GTK_LABEL(g_year_label), ya);
    pango_attr_list_unref(ya);
    gtk_box_pack_start(GTK_BOX(tb), g_year_label, TRUE, TRUE, 0);
    
    GtkWidget* nbtn = gtk_button_new_with_label("다음 분기 ▶");
    g_signal_connect(nbtn, "clicked", G_CALLBACK(on_next_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(tb), nbtn, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
    
    g_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), g_scroll, TRUE, TRUE, 0);
    
    GtkWidget* status = gtk_label_new("일정은 날짜를 더블클릭하여 추가/수정");
    gtk_widget_set_margin_start(status, 6); gtk_widget_set_margin_end(status, 6);
    gtk_widget_set_margin_top(status, 2); gtk_widget_set_margin_bottom(status, 2);
    gtk_label_set_xalign(GTK_LABEL(status), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 0);
    
    refresh_calendar();
    gtk_widget_show_all(win);
    gtk_main();
    
    g_app.save_schedule("schedule.csv");
    return 0;
}
