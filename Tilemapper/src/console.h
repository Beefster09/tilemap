#pragma once

#define GLFW_TO_CONSOLE_SHIFT 24

void init_console();
int console_type_key(int keycode);

/// Gets a string that can be rendered by the renderer
const char* get_console_line(bool show_cursor);
const char* get_console_scrollback_line(int line_offset);