static int in_touch = 0; // 1 = in a touch
static int slide_right = 0;
static int slide_left = 0;
static int touch_x = 0;
static int touch_y = 0;
static int old_x = 0;
static int old_y = 0;
static int diff_x = 0;
static int diff_y = 0;
static int min_x_swipe_px = 100;
static int min_y_swipe_px = 80;

#ifdef RECOVERY_TOUCH_GESTURE
static long s_last_fake_event_ms = 0;

static long getCurrentMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif	//#ifdef RECOVERY_TOUCH_GESTURE

static void set_min_swipe_lengths() {
    char value[PROPERTY_VALUE_MAX];
    property_get("ro.sf.lcd_density", value, "0");
    int screen_density = atoi(value);
    if(screen_density > 0) {
        min_x_swipe_px = (int)(0.5 * screen_density); // Roughly 0.5in
        min_y_swipe_px = (int)(0.3 * screen_density); // Roughly 0.3in
    }
}

static void reset_gestures() {
    diff_x = 0;
    diff_y = 0;
    old_x = 0;
    old_y = 0;
    touch_x = 0;
    touch_y = 0;

    ui_clear_key_queue();
}

void swipe_handle_input(int fd, struct input_event *ev) {
    int abs_store[6] = {0};
    int k;
    set_min_swipe_lengths();

    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), abs_store);
    int max_x_touch = abs_store[2];

    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), abs_store);
    int max_y_touch = abs_store[2];

    if(ev->type == EV_ABS && ev->code == ABS_MT_TRACKING_ID) {
        if(in_touch == 0) {
            in_touch = 1;
#ifdef RECOVERY_TOUCH_GESTURE
            s_last_fake_event_ms = getCurrentMs();
#endif
            reset_gestures();
        } else { // finger lifted
            ev->type = EV_KEY;
            if(slide_right == 1) {
                ev->code = KEY_POWER;
                slide_right = 0;
            } else if(slide_left == 1) {
                ev->code = KEY_BACK;
                slide_left = 0;
#ifdef RECOVERY_TOUCH_GESTURE
            } else if( (abs(diff_x) <= 10) && (abs(diff_y) <= 10) ) {
              if ((s_last_fake_event_ms + 300) < getCurrentMs()) {
                 ev->code = KEY_POWER;
              }
#endif	//#ifdef RECOVERY_TOUCH_GESTURE
            }
            ev->value = 1;
            in_touch = 0;
            reset_gestures();
        }
    } else if(ev->type == EV_ABS && ev->code == ABS_MT_POSITION_X) {
        old_x = touch_x;
        float touch_x_rel = (float)ev->value / (float)max_x_touch;
        touch_x = touch_x_rel * gr_fb_width();

        if(old_x != 0) diff_x += touch_x - old_x;

        if(diff_x > min_x_swipe_px) {
            slide_right = 1;
            reset_gestures();
        } else if(diff_x < -min_x_swipe_px) {
            slide_left = 1;
            reset_gestures();
        }
    } else if(ev->type == EV_ABS && ev->code == ABS_MT_POSITION_Y) {
        old_y = touch_y;
        float touch_y_rel = (float)ev->value / (float)max_y_touch;
        touch_y = touch_y_rel * gr_fb_height();

        if(old_y != 0) diff_y += touch_y - old_y;

        if(diff_y > min_y_swipe_px) {
            ev->code = KEY_VOLUMEDOWN;
            ev->type = EV_KEY;
            reset_gestures();
        } else if(diff_y < -min_y_swipe_px) {
            ev->code = KEY_VOLUMEUP;
            ev->type = EV_KEY;
            reset_gestures();
        }
    }

    return;
}
