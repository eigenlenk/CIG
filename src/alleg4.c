#include "imgui.h"
#include "demo.h"
#include <allegro.h>

// Locked 50 FPS
#define TICKRATE_MS 20

static volatile int pending_main_loop_update = 0;

void main_loop_ticker() {
  pending_main_loop_update = 1;
}
END_OF_FUNCTION(main_loop_ticker);


static IM_API_DRAW_IMAGE(alleg4_im_draw_image) {
  /*buffer_draw_image(
    buffer,
    image,
    frame.x + (center ? ((frame.w - image->size.x) * 0.5) : 0),
    frame.y + (center ? ((frame.h - image->size.y) * 0.5) : 0),
    flip_horizontally,
    flip_vertically
  );*/
}

static IM_API_DRAW_IMAGE_REGION(alleg4_im_draw_image_region) {
  /*buffer_blit_image(
    buffer,
    image,
    offset.x,
    offset.y,
    frame.x,
    frame.y,
    frame.w,
    frame.h,
    true,
    NULL
  );*/
}

static IM_API_DRAW_LINE(alleg4_im_draw_line) {
  // buffer_draw_line_2(buffer, x1, y1, x2, y2, color);
}

static IM_API_DRAW_RECT(alleg4_im_draw_rect) {
	if (fill >= 0) {
		rectfill(buffer, x, y, x+w-1, y+h-1, fill);
	}
	
	if (stroke >= 0) {
		rect(buffer, x, y, x+w-1, y+h-1, stroke);
	}
}

static IM_API_DRAW_TEXT(alleg4_im_draw_text) {
  // buffer_draw_text(buffer, x, y, font, alignment, color, COLOR_NONE, text);
}

static IM_API_GET_TEXT_SIZE(alleg4_im_get_text_size) {
  return text_length((FONT *)font, string);
}

static IM_API_GET_FONT_INFO(alleg4_im_get_font_info) {
  return (im_font_info_t) {
    0, 0, 0
    /*font->params.line_height,
    font->params.line_spacing,
    font->params.baseline_offset*/
  };
}

static IM_API_GET_NEXT_INPUT_CHARACTER(alleg4_im_get_next_input_character) {
  return 0; // input_character_from_buffer();
}

static IM_API_ALLOCATE_BUFFER(alleg4_im_allocate_buffer) {
  // im_buffer_ref buffer = malloc(sizeof(buffer_t));
  // buffer_configure(buffer, w, h);
  // return buffer;
  return NULL;
}

static IM_API_CLEAR_BUFFER(alleg4_im_clear_buffer) {
  clear(buffer);
}

static IM_API_BLIT_BUFFER(alleg4_im_blit_buffer) {
  // masked_blit(src->underlying, dst->underlying, 0, 0, frame.x, frame.y, frame.w, frame.h);
}

static IM_API_RELEASE_BUFFER(alleg4_im_release_buffer) {
  /*if (!buffer) { return; }
  buffer_invalidate(buffer);
  free(buffer);*/
}

static IM_API_SET_CLIP(alleg4_im_set_clip) {
  set_clip_rect(buffer, frame.x, frame.y, frame.x + frame.w - 1, frame.y + frame.h - 1);
}

static IM_API_RESET_CLIP(alleg4_im_reset_clip) {
  set_clip_rect(buffer, 0, 0, ((BITMAP *)buffer)->w - 1, ((BITMAP *)buffer)->h - 1);
}






int main(int argc, char *argv[]) {
  if (allegro_init() != 0) {
    return 1;
  }

  install_keyboard();
  install_mouse();
  install_timer();
  set_color_depth(8);

  int driver = GFX_AUTODETECT;

#ifdef _WIN32
  driver = GFX_AUTODETECT_WINDOWED;
#endif

  set_window_title("EGUI");

  if (set_gfx_mode(driver, 640, 480, 0, 0) != 0) {
    if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 2;
    }
  }

  if (install_int(main_loop_ticker, TICKRATE_MS) < 0) {
    allegro_message("Error installing interrupt\n%s\n", allegro_error);
    return 3;
  }

  LOCK_VARIABLE(pending_main_loop_update);
  LOCK_FUNCTION(main_loop_ticker);

  show_os_cursor(MOUSE_CURSOR_ARROW);
  enable_hardware_cursor();

  BITMAP *buffer = create_bitmap(SCREEN_W, SCREEN_H);

  im_configure((im_backend_configuration_t) {
    .draw_image = &alleg4_im_draw_image,
    .draw_image_region = &alleg4_im_draw_image_region,
    .draw_line = &alleg4_im_draw_line,
    .draw_rect = &alleg4_im_draw_rect,
    .draw_text = &alleg4_im_draw_text,
    .get_text_size = &alleg4_im_get_text_size,
    .get_font_info = &alleg4_im_get_font_info,
    .get_next_input_character = &alleg4_im_get_next_input_character,
    .allocate_buffer = alleg4_im_allocate_buffer,
    .clear_buffer = alleg4_im_clear_buffer,
    .blit_buffer = alleg4_im_blit_buffer,
    .release_buffer = alleg4_im_release_buffer,
    .set_clip = alleg4_im_set_clip,
    .reset_clip = alleg4_im_reset_clip
  });

  imgui_reset_internal_state();

  while (!keypressed()) {
    if (!pending_main_loop_update) {
      continue;
    }

    pending_main_loop_update = false;

    if (mouse_needs_poll()) {
      poll_mouse();
    }

    demo_ui(buffer);

    if (gfx_capabilities & GFX_HW_CURSOR) {
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    } else {
      show_mouse(NULL);
      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      show_mouse(screen);
    }
  }

  return 0;
}