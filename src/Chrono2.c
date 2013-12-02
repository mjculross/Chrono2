/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* *                                                                 * */
/* *                            Chrono2                              * */
/* *                                                                 * */
/* *      An "analog sweep" watchface with a smooth second hand      * */
/* *                                                                 * */
/* *                 [ SDK 2.0 compatible version ]                  * */
/* *                                                                 * */
/* * Created from the combination of the Brains watch provided with  * */
/* *    SDK 1.0 & the simple_analog example provided with SDK 2.0    * */
/* *                                                                 * */
/* *                    by Mark J Culross, KD5RXT                    * */
/* *                                                                 * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include "pebble.h"

static GBitmap *back_image;
static BitmapLayer *back_layer;

static Layer *battery_layer;
static Layer *date_layer;
static Layer *hands_layer;

static TextLayer *battery_label;
static TextLayer *day_label;
static TextLayer *num_label;

static char battery_buffer[6];
static char day_buffer[6];
static char num_buffer[4];

static AppTimer *timer_handle;

static GPath *second_hand;
static GPath *minute_hand;
static GPath *hour_hand;

static int partial_second = 0;
static int previous_second = -1;

static const GPoint center = {71, 83};

#define TIMER_MSEC 200

static Window *window;

static const GPathInfo SECOND_HAND_POINTS =
{
   6,
   (GPoint [])
   {
      { 0, 0 },
      { -2, 0 },
      { -2, -80 },
      { 2, -80 },
      { 2, 0 },
      { 0, 0 }
   }
};

static const GPathInfo MINUTE_HAND_POINTS =
{
   5,
   (GPoint [])
   {
      { 0, 0 },
      { -8, 0 },
      { 0, -80 },
      { 8, 0 },
      { 0, 0 }
   }
};

static const GPathInfo HOUR_HAND_POINTS =
{
   5,
   (GPoint [])
   {
      {0, 0},
      {-6, 0},
      {0, -50},
      {6, 0},
      { 0, 0 }
   }
};


static void battery_update_proc(Layer *layer, GContext *ctx);
static void date_update_proc(Layer *layer, GContext *ctx);
static void deinit(void);
static void handle_tick(void *data);
static void hands_update_proc(Layer *layer, GContext *ctx);
static void init(void);
static void set_bitmap_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint this_origin);


static void battery_update_proc(Layer *layer, GContext *ctx)
{
   BatteryChargeState battery_state = battery_state_service_peek();

   if (battery_state.is_charging)
   {
      battery_buffer[0] = '+';
   }
   else
   {
      battery_buffer[0] = ' ';
   }

   if (battery_state.charge_percent / 100)
   {
      battery_buffer[1] = '1';
   }
   else
   {
      battery_buffer[1] = ' ';
   }

   battery_state.charge_percent %= 100;

   battery_buffer[2] = (battery_state.charge_percent / 10) + '0';

   battery_buffer[3] = (battery_state.charge_percent % 10) + '0';

   battery_buffer[4] = '%';

   battery_buffer[5] = 0x00;

   text_layer_set_text(battery_label, battery_buffer);
}  // battery_update_proc()


static void date_update_proc(Layer *layer, GContext *ctx)
{
   time_t now = time(NULL);
   struct tm *t = localtime(&now);

   strftime(day_buffer, sizeof(day_buffer), "%a", t);
   text_layer_set_text(day_label, day_buffer);

   strftime(num_buffer, sizeof(num_buffer), "%d", t);
   text_layer_set_text(num_label, num_buffer);
}  // date_update_proc()


static void deinit(void)
{
   gpath_destroy(second_hand);
   gpath_destroy(minute_hand);
   gpath_destroy(hour_hand);

   text_layer_destroy(battery_label);
   text_layer_destroy(day_label);
   text_layer_destroy(num_label);

   layer_destroy(battery_layer);
   layer_destroy(date_layer);
   layer_destroy(hands_layer);

   gbitmap_destroy(back_image);
   bitmap_layer_destroy(back_layer);

   app_timer_cancel(timer_handle);
   window_destroy(window);
}  // deinit()


static void handle_tick(void *data)
{
   timer_handle = app_timer_register(TIMER_MSEC, handle_tick, NULL);
   layer_mark_dirty(window_get_root_layer(window));
}  // handle_tick()


static void hands_update_proc(Layer *layer, GContext *ctx)
{
   gpath_move_to(second_hand, center);
   gpath_move_to(minute_hand, center);
   gpath_move_to(hour_hand, center);

   time_t now = time(NULL);
   struct tm *t = localtime(&now);

   if (t->tm_sec != previous_second)
   {
      partial_second = 0;
      previous_second = t->tm_sec;
   }
   else
   {
      partial_second++;
   }

   // all three hands
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_context_set_stroke_color(ctx, GColorBlack);

   int32_t second_angle = TRIG_MAX_ANGLE * ((t->tm_sec * (1000 / TIMER_MSEC)) + partial_second) / (60 * (1000 / TIMER_MSEC));
   gpath_rotate_to(second_hand, second_angle);
   gpath_draw_filled(ctx, second_hand);
   gpath_draw_outline(ctx, second_hand);

   int32_t minute_angle = TRIG_MAX_ANGLE * (t->tm_min * 6 + (t->tm_sec / 10)) / 360;
   gpath_rotate_to(minute_hand, minute_angle);
   gpath_draw_filled(ctx, minute_hand);
   gpath_draw_outline(ctx, minute_hand);

   int32_t hour_angle = TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 30) + (t->tm_min / 2)) / 360;
   gpath_rotate_to(hour_hand, hour_angle);
   gpath_draw_filled(ctx, hour_hand);
   gpath_draw_outline(ctx, hour_hand);
 
   // rings/dot in the middle
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_fill_circle(ctx, center, 7);
   graphics_context_set_fill_color(ctx, GColorWhite);
   graphics_fill_circle(ctx, center, 4);
   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_fill_circle(ctx, center, 3);
   graphics_context_set_fill_color(ctx, GColorWhite);
   graphics_fill_circle(ctx, center, 1);
}  // hands_update_proc()


static void init(void)
{
   GRect dummy_frame = { {0, 0}, {0, 0} };

   window = window_create();

   battery_buffer[0] = '\0';
   day_buffer[0] = '\0';
   num_buffer[0] = '\0';

   // init hand paths
   second_hand = gpath_create(&SECOND_HAND_POINTS);
   minute_hand = gpath_create(&MINUTE_HAND_POINTS);
   hour_hand = gpath_create(&HOUR_HAND_POINTS);

   Layer *window_layer = window_get_root_layer(window);
   GRect bounds = layer_get_bounds(window_layer);

   // init layers

   // init background layer -> a plain bitmap layer with the Brains face
   back_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
   back_layer = bitmap_layer_create(dummy_frame);
   bitmap_layer_set_bitmap(back_layer, back_image);
   layer_add_child(window_layer, bitmap_layer_get_layer(back_layer));
   set_bitmap_image(&back_image, back_layer, RESOURCE_ID_IMAGE_BACKGROUND, GPoint (0, 0));

   // init battery layer -> a plain parent layer to create a battery update proc
   battery_layer = layer_create(bounds);
   layer_set_update_proc(battery_layer, battery_update_proc);
   layer_add_child(window_layer, battery_layer);

   // init battery
   battery_label = text_layer_create(GRect(50, 40, 60, 30));
   text_layer_set_text(battery_label, battery_buffer);
   text_layer_set_background_color(battery_label, GColorWhite);
   text_layer_set_text_color(battery_label, GColorBlack);
   text_layer_set_font(battery_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
   layer_add_child(battery_layer, text_layer_get_layer(battery_label));

   // init date layer -> a plain parent layer to create a date update proc
   date_layer = layer_create(bounds);
   layer_set_update_proc(date_layer, date_update_proc);
   layer_add_child(window_layer, date_layer);

   // init day
   day_label = text_layer_create(GRect(40, 90, 36, 30));
   text_layer_set_text(day_label, day_buffer);
   text_layer_set_background_color(day_label, GColorWhite);
   text_layer_set_text_color(day_label, GColorBlack);
   text_layer_set_font(day_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
   layer_add_child(date_layer, text_layer_get_layer(day_label));

   // init num
   num_label = text_layer_create(GRect(80, 90, 24, 30));

   text_layer_set_text(num_label, num_buffer);
   text_layer_set_background_color(num_label, GColorWhite);
   text_layer_set_text_color(num_label, GColorBlack);
   text_layer_set_font(num_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
   layer_add_child(date_layer, text_layer_get_layer(num_label));

   // init hands
   hands_layer = layer_create(bounds);
   layer_set_update_proc(hands_layer, hands_update_proc);
   layer_add_child(window_layer, hands_layer);

   // Push the window onto the stack - animated
   window_stack_push(window, true);

   timer_handle = app_timer_register(TIMER_MSEC, handle_tick, NULL);
}  // init()


static void set_bitmap_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint this_origin)
{
   gbitmap_destroy(*bmp_image);

   *bmp_image = gbitmap_create_with_resource(resource_id);
   GRect frame = (GRect)
   {
      .origin = this_origin,
      .size = (*bmp_image)->bounds.size
   };
   bitmap_layer_set_compositing_mode(bmp_layer, GCompOpAssign);
   layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
   bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
}  // set_bitmap_image()


int main(void)
{
   init();
   app_event_loop();
   deinit();
}

