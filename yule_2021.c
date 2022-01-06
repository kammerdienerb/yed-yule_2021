#include <yed/plugin.h>

static yed_plugin        *Self;
static int                snowing;
static yed_event_handler  epump;
static yed_event_handler  ekey;
static array_t            dds;
static int                flakes_per_s = 50;
static u64                ts_ms;
static int                wobbling;
static yed_direct_draw_t *label_dd;
static yed_direct_draw_t *buttons_dd;

void pump_handler(yed_event *event) {
    u64                 now;
    int                 i;
    yed_direct_draw_t **dit;
    yed_direct_draw_t  *dd;

    now = measure_time_now_ms();

    if (now - ts_ms >= (1000/flakes_per_s)) {
        array_traverse(dds, dit) {
            dd       = *dit;
            dd->row += 1;
            dd->col += (rand() & 1  ? -1 : 1) * (rand() & 1);
        }

        if (wobbling) {
            array_traverse(dds, dit) {
                dd       = *dit;
                dd->col += wobbling > 0 ? 1 : -1;
            }
            if (wobbling < 0) { wobbling += 1; }
            else              { wobbling -= 1; }
        } else {
            if (rand() % 10 == 0) {
                #define WOBBLE (16)
                wobbling = -(WOBBLE/2) + (rand() % WOBBLE);
            }
        }

again:;
        i = 0;
        array_traverse(dds, dit) {
            dd = *dit;
            if (dd->row < 1 || dd->row == ys->term_rows - 2
            ||  dd->col < 1 || dd->col > ys->term_cols) {
                yed_kill_direct_draw(dd);
                array_delete(dds, i);
                goto again;
            }
            i += 1;
        }


        dd = yed_direct_draw(1, 1 + (rand() % ys->term_cols), yed_parse_attrs("&active.fg"), "❅");

        array_push(dds, dd);

        ts_ms = now;
    }
}

void key_handler(yed_event *event) {
    if (IS_MOUSE(event->key)
    &&  MOUSE_KIND(event->key) == MOUSE_PRESS
    &&  MOUSE_BUTTON(event->key) == MOUSE_BUTTON_LEFT
    &&  MOUSE_ROW(event->key) == ys->term_rows - 3) {

        if (MOUSE_COL(event->key) >= ys->term_cols - 10 && MOUSE_COL(event->key) <= ys->term_cols - 7) {
            flakes_per_s -= 1;
            if (flakes_per_s < 1) { flakes_per_s = 1; }
            event->cancel = 1;
            LOG_CMD_ENTER("let-it-snow");
            yed_cprint("speed: %d flakes/s", flakes_per_s);
            LOG_EXIT();
        } else if (MOUSE_COL(event->key) >= ys->term_cols - 5 && MOUSE_COL(event->key) <= ys->term_cols - 2) {
            flakes_per_s += 1;
            if (flakes_per_s > 100) { flakes_per_s = 100; }
            event->cancel = 1;
            LOG_CMD_ENTER("let-it-snow");
            yed_cprint("speed: %d flakes/s", flakes_per_s);
            LOG_EXIT();
        }
    }
}

void let_it_snow(int n_args, char **args) {
    yed_direct_draw_t **dit;

    if (snowing) {
        yed_delete_event_handler(epump);
        yed_delete_event_handler(ekey);
        array_traverse(dds, dit) {
            yed_kill_direct_draw(*dit);
        }
        array_clear(dds);
        yed_plugin_request_no_mouse_reporting(Self);
        yed_kill_direct_draw(label_dd);
        yed_kill_direct_draw(buttons_dd);
        yed_set_update_hz(0);
        snowing = 0;
    } else {
        epump.kind = EVENT_PRE_PUMP;
        epump.fn   = pump_handler;
        yed_plugin_add_event_handler(Self, epump);
        ekey.kind = EVENT_KEY_PRESSED;
        ekey.fn   = key_handler;
        yed_plugin_add_event_handler(Self, ekey);
        yed_plugin_request_mouse_reporting(Self);
        label_dd   = yed_direct_draw_style(ys->term_rows - 4, ys->term_cols - 10, STYLE_associate, " ❅ speed ");
        buttons_dd = yed_direct_draw_style(ys->term_rows - 3, ys->term_cols - 10, STYLE_associate, "  - | +  ");
        yed_set_update_hz(100);
        snowing    = 1;
    }
}

void unload(yed_plugin *self) {
    yed_direct_draw_t **dit;

    if (snowing) {
        array_traverse(dds, dit) {
            yed_kill_direct_draw(*dit);
        }
        yed_kill_direct_draw(label_dd);
        yed_kill_direct_draw(buttons_dd);
        yed_plugin_request_no_mouse_reporting(Self);
    }

    array_free(dds);

    yed_set_update_hz(0);
}

int yed_plugin_boot(yed_plugin *self) {
    YED_PLUG_VERSION_CHECK();

    Self = self;

    yed_plugin_set_unload_fn(self, unload);
    yed_plugin_set_command(self, "let-it-snow", let_it_snow);

    dds = array_make(yed_direct_draw_t*);

    return 0;
}
