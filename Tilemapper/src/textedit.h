#pragma once

#define GLFW_TO_CONSOLE_SHIFT 20

void init_console();
int console_type_key(int keycode);
const char* get_console_line();

