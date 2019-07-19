#include <string>
#include <cassert>

#include <glfw3.h>

#include "text.h"
#include "console.h"

constexpr int MOD_SHIFT = GLFW_MOD_SHIFT   << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_ALT   = GLFW_MOD_ALT     << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_CTRL  = GLFW_MOD_CONTROL << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_SUPER = GLFW_MOD_SUPER   << GLFW_TO_CONSOLE_SHIFT;

static char key_to_char(int key) {
	if (key & (MOD_ALT | MOD_CTRL | MOD_SUPER)) return 0;
	int base_key = key & 0x0fffff; // strip modifiers
	if (base_key >= GLFW_KEY_KP_0 && base_key <= GLFW_KEY_KP_9) {
		return base_key - GLFW_KEY_KP_0 + '0';
	}
	if (base_key > 127) return 0;
	if (key & MOD_SHIFT) {
		switch (base_key) {
		case '`': return '~';
		case '0': return ')';
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return ')';
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case '\\': return '|';
		case ';': return ':';
		case '\'': return '"';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
		default: return base_key; // correct for CAPITAL LETTERS
		}
	}
	else {
		if (isalpha(base_key)) return tolower(base_key);
		else return base_key;
	}
	return 0;
}

const char WORD_CHARS[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
constexpr int N_WORD_CHARS = sizeof(WORD_CHARS) - 1;

static int word_before(std::string* str, int index) {
	int word_start = str->find_last_not_of(WORD_CHARS, index, N_WORD_CHARS) + 1;
	if (word_start == std::string::npos) return 0;
	if (word_start < index) return word_start; // Not already at start of current word
	int prev_word_end = str->find_last_of(WORD_CHARS, word_start - 1, N_WORD_CHARS) - 1; // last char of previous word
	if (prev_word_end == std::string::npos) return 0;
	int prev_word = str->find_last_not_of(WORD_CHARS, prev_word_end, N_WORD_CHARS) + 1;
	if (prev_word == std::string::npos) return 0;
	else return prev_word;
}

static int word_after(std::string* str, int index) {
	int word_end = str->find_first_not_of(WORD_CHARS, index, N_WORD_CHARS); // end of current word
	if (word_end == std::string::npos) return str->size();
	int next_word = str->find_first_of(WORD_CHARS, word_end, N_WORD_CHARS);
	if (next_word == std::string::npos) return str->size();
	else return next_word;
}

// Reference: https://github.com/nothings/stb/blob/master/stb_textedit.h
#define STB_TEXTEDIT_STRING               std::string
#define STB_TEXTEDIT_STRINGLEN(S)         (S->size())
#define STB_TEXTEDIT_LAYOUTROW(R,obj,n)   (*R = {})   /*stub*/
#define STB_TEXTEDIT_GETWIDTH(obj,n,i)    (0)        /*stub*/
#define STB_TEXTEDIT_KEYTOTEXT(KEY)       key_to_char(KEY)
#define STB_TEXTEDIT_GETCHAR(S,I)         (*S)[I]
#define STB_TEXTEDIT_NEWLINE              ('\n')
#define STB_TEXTEDIT_DELETECHARS(S,I,N)   (S->erase(I,N), true)
#define STB_TEXTEDIT_INSERTCHARS(S,I,C,N) (S->insert(I,C,N), true)
#define STB_TEXTEDIT_MOVEWORDLEFT(S,I)    word_before(S, I)
#define STB_TEXTEDIT_MOVEWORDRIGHT(S,I)   word_after(S, I) 
//
#define STB_TEXTEDIT_K_SHIFT       MOD_SHIFT
//
#define STB_TEXTEDIT_K_LEFT        GLFW_KEY_LEFT
#define STB_TEXTEDIT_K_RIGHT       GLFW_KEY_RIGHT
#define STB_TEXTEDIT_K_UP          GLFW_KEY_UP   // Not normally let through
#define STB_TEXTEDIT_K_DOWN        GLFW_KEY_DOWN // Not normally let through
#define STB_TEXTEDIT_K_LINESTART   GLFW_KEY_HOME
#define STB_TEXTEDIT_K_LINEEND     GLFW_KEY_END
#define STB_TEXTEDIT_K_TEXTSTART   (MOD_CTRL + GLFW_KEY_HOME)
#define STB_TEXTEDIT_K_TEXTEND     (MOD_CTRL + GLFW_KEY_END)
#define STB_TEXTEDIT_K_DELETE      GLFW_KEY_DELETE
#define STB_TEXTEDIT_K_BACKSPACE   GLFW_KEY_BACKSPACE
#define STB_TEXTEDIT_K_UNDO        (MOD_CTRL + GLFW_KEY_Z)
#define STB_TEXTEDIT_K_REDO        (MOD_CTRL + MOD_SHIFT + GLFW_KEY_Z)

#define STB_TEXTEDIT_K_WORDLEFT    (MOD_CTRL + GLFW_KEY_LEFT)
#define STB_TEXTEDIT_K_WORDRIGHT   (MOD_CTRL + GLFW_KEY_RIGHT)

#define STB_TEXTEDIT_CHARTYPE char
#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

enum ConsoleDataType {
	T_STRING,
	T_INT,
	T_FLOAT,
	T_HEX_COLOR,
};

enum CommandStatus {
	CMD_OK = 0,
	CMD_FAILURE,
	CMD_FORMAT_ERROR,
	CMD_TYPE_MISMATCH,
	CMD_NOT_ENOUGH_ARGS,
	CMD_DUPLICATE_ARG,
	CMD_NOT_FOUND = -1,
};

struct ConsoleVar {
	void* ptr;
	const char* name;
	ConsoleDataType type;
};

union Any{ // needed for putting args into an array when dispatching
	char*    string_default;
	int      int_default;
	float    float_default;
	HexColor color_default;
};

struct ConsoleParam {
	ConsoleDataType type;
};

typedef CommandStatus (*ConsoleFPtr)(int n_args, Any* args, void* ret_ptr);

struct ConsoleFunc {
	ConsoleFPtr ptr;
	const char* name;
	int n_args;
	ConsoleParam* params;
	ConsoleDataType ret_type;
};

constexpr int MAX_ARGS = 16;

#include "generated/console_commands.h"

// GLOBALS
static STB_TexteditState console_state;
static std::string console_line;
static std::string console_line_saved;
// a deque might be better, but console command performance doesn't matter much and I don't care how much command history I allocate.
static std::vector<std::string> console_history;
static int history_pos;

//    void stb_textedit_initialize_state(STB_TexteditState *state, int is_single_line)
//
//    void stb_textedit_click(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    void stb_textedit_drag(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    int  stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
//    int  stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE *text, int len)
//    void stb_textedit_key(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXEDIT_KEYTYPE key)

class ConsoleLexer {
	const char* line;
	int cur_char = 0;
	char* tok_buffer;

public:
	ConsoleLexer(const std::string& line) {
		this->line = line.c_str();
		tok_buffer = temp_alloc(char, line.size() + 1 + MAX_ARGS);
		assert(tok_buffer);
	}

	char* emit_token() {
		char* token = tok_buffer;
		
		// look for something that isn't whitespace/control chars
		while (line[cur_char] <= ' ') {
			if (line[cur_char] == 0) return nullptr; // Already at end of string. Nothing to return.
			cur_char++;
		}
		char c;
		while (c = line[cur_char]) {
			switch (c) {
			case '\'': while (c = line[++cur_char]) {
				if (c == '\'') break;
				*tok_buffer++ = c;
				cur_char++;
			} break;
			case '"': while (c = line[++cur_char]) {
				if (c == '"') break;
				*tok_buffer++ = c;
				cur_char++;
			} break;
			case '`': while (c = line[++cur_char]) {
				if (c == '`') break;
				*tok_buffer++ = c;
				cur_char++;
			} break;
			case ' ': goto parse_end;
			default: *tok_buffer++ = c;
			}
			cur_char++;
		}
		parse_end:
		*tok_buffer++ = 0;
		return token;
	}
};

static ConsoleFunc& lookup_command(const char* cmd) {

}

static CommandStatus set_variable(const char* var_name, const char* str_value) {
	if (!var_name || !str_value) return CMD_NOT_ENOUGH_ARGS;
	for (int i = 0; i < n_console_vars; i++) { // TEMP: is linear search
		const ConsoleVar& var = console_vars[i];
		if (strcmp(var_name, var.name) == 0) {
			switch (var.type) {
			case T_STRING:
				return CMD_FAILURE;
			case T_HEX_COLOR: {
				HexColor color;
				switch (parse_hex_color(str_value, nullptr, &color)) {
				case HEX_COLOR_OK:
					*((HexColor*)var.ptr) = color;
					return CMD_OK;
				case HEX_COLOR_INVALID_CHARS: return CMD_TYPE_MISMATCH;
				case HEX_COLOR_INVALID_LEN: return CMD_FORMAT_ERROR;
				default: return CMD_TYPE_MISMATCH;
				}
			}
			case T_FLOAT: {
				float value = strtof(str_value, nullptr);
				if (!isnan(value) && !isinf(value)) {
					*((float*)var.ptr) = value;
					return CMD_OK;
				}
				else return CMD_TYPE_MISMATCH;
			}
			case T_INT: {
				float value = strtol(str_value, nullptr, 0);
				if (!isnan(value) && !isinf(value)) {
					*((float*)var.ptr) = value;
					return CMD_OK;
				}
				else return CMD_TYPE_MISMATCH;
			}
			}
		}
	}
	return CMD_NOT_FOUND;
}

static CommandStatus console_submit_command() {
	ConsoleLexer lexer(console_line);
	char* cmd = lexer.emit_token();
	char* equals = strchr(cmd, '=');
	if (equals) {
		*equals = 0;
		return set_variable(cmd, equals + 1);
	}
	else if (strcmp(cmd, "set") == 0) {
		auto var = lexer.emit_token();
		auto val = lexer.emit_token();
		return set_variable(var, val);
	}
}

static void console_history_scroll(int offset) {
	auto len = console_history.size();
	if (len == 0) return;
	if (history_pos == 0) {
		if (offset > 0) return;
		console_line_saved = console_line;
	}
	history_pos -= offset;
	if (history_pos > 0) {
		if (history_pos > len) {
			history_pos = len;
		}
		console_line = console_history[len - history_pos];
	}
	else {
		history_pos = 0;
		console_line = console_line_saved;
	}
}

void init_console() {
	stb_textedit_initialize_state(&console_state, true);
}

int console_type_key(int keycode) {
	if (keycode == GLFW_KEY_ENTER) {
		console_submit_command();
		console_history.push_back(console_line);
		history_pos = 0;
		console_line.clear();
		stb_textedit_initialize_state(&console_state, true);
	}
	else if (keycode == GLFW_KEY_UP) {
		console_history_scroll(-1);
	}
	else if (keycode == GLFW_KEY_DOWN) {
		console_history_scroll(+1);
	}
	else if (keycode == GLFW_KEY_TAB) {
		// TODO: autocomplete
	}
	else {
		stb_textedit_key(&console_line, &console_state, keycode);
	}
	return 0;
}

static char console_line_buffer[1024];
const char* get_console_line(bool show_cursor = true) {
	if (show_cursor) {
		snprintf(console_line_buffer, sizeof(console_line_buffer), "\x1%d\x2%s", console_state.cursor, console_line.c_str());
		return console_line_buffer;
	}
	else {
		return console_line.c_str();
	}
}