#ifndef GUI_GOODS_WAITING_TIME_INFO_H
#define GUI_GOODS_WAITING_TIME_INFO_H

#include "components/gui_aligned_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "gui_frame.h"
#include "../linehandle_t.h"

class schedule_t;
class player_t;

class gui_goods_waiting_time_stat_t : public gui_aligned_container_t {
private:
    player_t* player;
    linehandle_t line;

public:
    gui_goods_waiting_time_stat_t(player_t* player, linehandle_t line): player(player), line(line) {};
    void update();
};

class gui_goods_waiting_time_t : public gui_frame_t
{
private:
    gui_goods_waiting_time_stat_t stat;
    gui_scrollpane_t scrolly;

    linehandle_t line;
    schedule_t* schedule; // the schedule the shown data was taken from
    cbuffer_t title_buf;

public:
    gui_goods_waiting_time_t(linehandle_t, player_t*);

    void update();
    void update_title();
    void draw(scr_coord pos, scr_size size) OVERRIDE;
};


#endif