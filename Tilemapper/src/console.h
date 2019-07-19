#pragma once

#define GLFW_TO_CONSOLE_SHIFT 24

void init_console();
int console_type_key(int keycode);
const char* get_console_line(bool show_cursor);

void register_commands();