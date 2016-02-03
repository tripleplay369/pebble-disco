#include <pebble.h>
#include "font.h"

#define HEIGHT 180
#define WIDTH 180
#define SQUARE_SIZE 4

#define HOUR_ROW_START 11
#define HOUR_COL_MIDDLE 22
#define HOUR_SPACING 1
#define HOUR_THICKNESS 3
#define MINUTE_RADIUS 75
#define MINUTE_SIZE 5

#define SQUARES_PER_HEIGHT (HEIGHT / SQUARE_SIZE)
#define SQUARES_PER_WIDTH (WIDTH / SQUARE_SIZE)

static uint8_t random_colors[SQUARES_PER_HEIGHT * SQUARES_PER_WIDTH]; 
static uint8_t random_colors2[SQUARES_PER_HEIGHT * SQUARES_PER_WIDTH];
static Window * main_window;
static Layer * main_layer;

static void update_proc(Layer * layer, GContext * ctx) {
  graphics_context_set_antialiased(ctx, false);
  
  time_t now = time(NULL);
  struct tm * now_tm = localtime(&now);
  
  // background
  for(unsigned int i = 0; i < sizeof(random_colors); ++i){
    random_colors[i] = 0b11000000 + (rand() & 0x3F);
    random_colors2[i] = 0b11000000 + (rand() & 0x3F);
  }
  
  for(int r = 0; r < HEIGHT; ++r){
    for(int c = 0; c < WIDTH; ++c){
      uint8_t r_color_index = r / SQUARE_SIZE;
      uint8_t c_color_index = c / SQUARE_SIZE;
      uint8_t color_index = r_color_index * SQUARES_PER_WIDTH + c_color_index;
      
      uint8_t current_color = random_colors[color_index];
      if((r + c) % 2 == 0){
        current_color = random_colors2[color_index];
      }
      
      graphics_context_set_stroke_color(ctx, (GColor)current_color);
      graphics_draw_pixel(ctx, GPoint(c, r));
    }
  }
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  // hour
  int hour = now_tm->tm_hour;
  if(!clock_is_24h_style()){
    if(hour >= 13){
      hour -= 12;
    }
    else{
      if(hour < 1){
        hour += 12;
      }
    }
  }
  
  int num_hour_digits = hour >= 10 ? 2 : 1;
  int total_number_width = NUMBER_WIDTHS[hour / 10] + NUMBER_WIDTHS[hour % 10];
  int hour_col_start = num_hour_digits == 2 ? HOUR_COL_MIDDLE - (total_number_width * HOUR_THICKNESS + HOUR_SPACING) / 2 : HOUR_COL_MIDDLE - NUMBER_WIDTHS[hour] * HOUR_THICKNESS / 2;
    
  for(int r = HOUR_ROW_START; r < HOUR_ROW_START + NUMBER_HEIGHT * HOUR_THICKNESS; r += HOUR_THICKNESS){
    int hour_col = hour_col_start;
    for(int i = 0; i < num_hour_digits; ++i){
      int digit = num_hour_digits > 1 ? (i == 0 ? hour / 10 : hour % 10) : hour;
      
      for(int c = hour_col; c < hour_col + NUMBER_WIDTH * HOUR_THICKNESS; c += HOUR_THICKNESS){
        int number_r = (r - HOUR_ROW_START) / HOUR_THICKNESS;
        int number_c = (c - hour_col) / HOUR_THICKNESS;
        if(NUMBERS[digit][number_r][number_c]){
          graphics_fill_rect(ctx, GRect(c * SQUARE_SIZE, r * SQUARE_SIZE, HOUR_THICKNESS * SQUARE_SIZE, HOUR_THICKNESS * SQUARE_SIZE), 0, GCornerNone);
        }
      }
      hour_col += (NUMBER_WIDTHS[digit] * HOUR_THICKNESS) + HOUR_SPACING;
    }
  }
  
  // minute
  int minute = now_tm->tm_min;
  
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60 - TRIG_MAX_ANGLE / 4;
  int minute_center_start_x = ((WIDTH / 2) + cos_lookup(minute_angle) * MINUTE_RADIUS / TRIG_MAX_RATIO) / SQUARE_SIZE * SQUARE_SIZE;
  int minute_center_start_y = ((HEIGHT / 2) + sin_lookup(minute_angle) * MINUTE_RADIUS / TRIG_MAX_RATIO) / SQUARE_SIZE * SQUARE_SIZE;
  
  graphics_fill_rect(ctx, GRect(minute_center_start_x - SQUARE_SIZE * (MINUTE_SIZE / 2), minute_center_start_y - SQUARE_SIZE * (MINUTE_SIZE / 2), MINUTE_SIZE * SQUARE_SIZE, MINUTE_SIZE * SQUARE_SIZE), 0, GCornerNone);
  
  // grid
  graphics_context_set_stroke_color(ctx, GColorBlack);
  for(int r = 0; r < SQUARES_PER_HEIGHT; ++r){
    graphics_draw_line(ctx, GPoint(0, r * SQUARE_SIZE), GPoint(WIDTH, r * SQUARE_SIZE));
  }
  for(int c = 0; c < SQUARES_PER_WIDTH; ++c){
    graphics_draw_line(ctx, GPoint(c * SQUARE_SIZE, 0), GPoint(c * SQUARE_SIZE, HEIGHT));
  }
}


static void tick_handler(struct tm * tick_time, TimeUnits units_changed) {
  layer_mark_dirty(main_layer);
}

static void main_window_load(Window * window) {
  Layer * window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  main_layer = layer_create(bounds);
  layer_set_update_proc(main_layer, update_proc);
  layer_add_child(window_layer, main_layer);
}

static void main_window_unload(Window * window) {
  layer_destroy(main_layer);
}

static void init() {
  srand(time(NULL));
  main_window = window_create();

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
      
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}