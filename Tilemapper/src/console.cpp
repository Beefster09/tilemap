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
	T_VOID = 0,
	T_INT = 1,
	T_FLOAT,
	T_BOOL,
	T_STRING,
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
	CMD_ARG_NAME_NOT_FOUND = -2,
};

static const char* status_string(CommandStatus status) {
	switch (status) {
	case CMD_OK: return "Done";
	case CMD_FAILURE: return "Operation failed";
	case CMD_FORMAT_ERROR: return "Invalid format";
	case CMD_TYPE_MISMATCH: return "Type mismatch";
	case CMD_NOT_ENOUGH_ARGS: return "Not enough arguments";
	case CMD_DUPLICATE_ARG: return "Argument specified more than once";
	case CMD_NOT_FOUND: return "Command/variable not found";
	case CMD_ARG_NAME_NOT_FOUND: return "Invalid argument name";
	}
}

static HexColor status_color(CommandStatus status) {
	switch (status) {
	case CMD_OK: return 0x00aa00;
	default: return 0xaa0000;
	}
}

struct ConsoleVar {
	void* ptr;
	const char* name;
	ConsoleDataType type;
};

union Any { // needed for putting args into an array when dispatching
	char*    v_string;
	int      v_int;
	float    v_float;
	bool     v_bool;
	HexColor v_hex_color;
};

struct ConsoleParam {
	ConsoleDataType type;
	const char* name;
	void* default_value;
};

typedef CommandStatus (*ConsoleFPtr)(int n_args, Any* args, void* ret_ptr);

constexpr int MAX_ARGS = 16;
struct ConsoleFunc {
	ConsoleFPtr ptr;
	const char* name;
	int n_params;
	ConsoleParam params[MAX_ARGS];
	ConsoleDataType ret_type;
};

struct ConsoleScrollback {
	std::string contents;
	enum { COMMAND, RESPONSE, LOG } type = COMMAND;
	CommandStatus resp_status = CMD_OK;
};

#include "generated/console_commands.h"

// GLOBALS
static STB_TexteditState console_state;
static std::string console_line;
static std::string console_line_saved;
// a deque might be better, but console command performance doesn't matter much and I don't care how much command history I allocate.
static std::vector<std::string> console_history;
static std::vector<ConsoleScrollback> console_scrollback;
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
		while (c = line[cur_char++]) {
			switch (c) {
			case '\'':
				while (c = line[cur_char++]) {
					if (c == '\'') break;
					if (c == 0) goto emit_token_end;
					*tok_buffer++ = c;
				} break;
			case '"':
				while (c = line[cur_char++]) {
					if (c == '"') break;
					if (c == 0) goto emit_token_end;
					*tok_buffer++ = c;
				} break;
			case '`': 
				while (c = line[cur_char++]) {
					if (c == '`') break;
					if (c == 0) goto emit_token_end;
					*tok_buffer++ = c;
				} break;
			case ' ': goto emit_token_end;
			default: *tok_buffer++ = c;
			}
		}
	emit_token_end:
		*tok_buffer++ = 0;
		return token;
	}
};

static const ConsoleFunc* lookup_command(const char* cmd) {
	for (int i = 0; i < n_console_funcs; i++) { // TEMP: is linear search
		const ConsoleFunc& func = console_funcs[i];
		if (strcmp(cmd, func.name) == 0) {
			return &func;
		}
	}
	return nullptr;
}

static const ConsoleVar* lookup_variable(const char* var_name) {
	for (int i = 0; i < n_console_vars; i++) { // TEMP: is linear search
		const ConsoleVar& var = console_vars[i];
		if (strcmp(var_name, var.name) == 0) {
			return &var;
		}
	}
	return nullptr;
}

static CommandStatus set_variable(const char* var_name, const char* str_value) {
	if (!var_name || !str_value) return CMD_NOT_ENOUGH_ARGS;
	auto var = lookup_variable(var_name);
	if (var == nullptr) {
		return CMD_NOT_FOUND;
	}
	switch (var->type) {
	case T_STRING:
		return CMD_FAILURE;
	case T_HEX_COLOR: {
		HexColor color;
		switch (parse_hex_color(str_value, nullptr, &color)) {
		case OK:
			*((HexColor*)var->ptr) = color;
			return CMD_OK;
		case INVALID_CHARS: return CMD_TYPE_MISMATCH;
		case INVALID_LEN: return CMD_FORMAT_ERROR;
		default: return CMD_TYPE_MISMATCH;
		}
	}
	case T_FLOAT: {
		float value = strtof(str_value, nullptr);
		if (!isnan(value) && !isinf(value)) {
			*((float*)var->ptr) = value;
			return CMD_OK;
		}
		else return CMD_TYPE_MISMATCH;
	}
	case T_INT: {
		errno = 0;
		int value = strtol(str_value, nullptr, 0);
		if (errno = ERANGE) {
			*((int*)var->ptr) = value;
			return CMD_OK;
		}
		else return CMD_TYPE_MISMATCH;
	}
	case T_BOOL: {
		bool value;
		switch(parse_bool(str_value, nullptr, &value)) {
		case OK:
			*((bool*)var->ptr) = value;
			return CMD_OK;
		default: return CMD_TYPE_MISMATCH;
		}
	}
	default: return CMD_FAILURE;
	}
}

static CommandStatus get_variable(const char* var_name) {
	ERR_LOG("Not Implemented.\n");
	return CMD_FAILURE;
}

static CommandStatus get_func_args(const ConsoleFunc& func, ConsoleLexer &lexer, char * &equals, Any * args, bool &retflag) {
	retflag = true;
	bool* set_vars = temp_alloc0(bool, func.n_params);
	int cur_pos_arg = 0;
	// Populate the arguments with each token
	while (cur_pos_arg < func.n_params) {
		int actual_arg = cur_pos_arg;
		auto arg = lexer.emit_token();
		if (arg == nullptr) break; // no more tokens
		char* val = arg;
		equals = strchr(arg, '=');
		if (equals) {
			val = equals + 1;
			if (equals > arg) { // the equals was not the first character
				*equals = 0;
				actual_arg = -1;
				// find the arg that can be keyworded
				for (int i = 0; i < func.n_params; i++) {
					auto& param = func.params[i];
					if (strcmp(arg, param.name) == 0) {
						actual_arg = i;
						break;
					}
				}
				if (actual_arg < 0) return CMD_ARG_NAME_NOT_FOUND;
			}
		}
		if (set_vars[actual_arg]) return CMD_DUPLICATE_ARG;
		// Actually interpret the token according to the target argument type
		switch (func.params[actual_arg].type) {
		case T_STRING:
			return CMD_FAILURE;
		case T_HEX_COLOR: {
			HexColor color;
			switch (parse_hex_color(val, nullptr, &color)) {
			case OK:
				args[actual_arg].v_hex_color = color;
				break;
			case INVALID_CHARS: return CMD_TYPE_MISMATCH;
			case INVALID_LEN: return CMD_FORMAT_ERROR;
			default: return CMD_TYPE_MISMATCH;
			}
		}
		case T_FLOAT: {
			float value = strtof(val, nullptr);
			if (!isnan(value) && !isinf(value)) {
				args[actual_arg].v_float = value;
				break;
			}
			else return CMD_TYPE_MISMATCH;
		}
		case T_INT: {
			errno = 0;
			int value = strtol(val, nullptr, 0);
			if (errno = ERANGE) {
				args[actual_arg].v_int = value;
				break;
			}
			else return CMD_TYPE_MISMATCH;
		}
		default: return CMD_FAILURE;
		}
		set_vars[actual_arg] = true;
		// find the next argument that can be read
		if (actual_arg == cur_pos_arg) {
			while (cur_pos_arg < func.n_params && set_vars[cur_pos_arg]) cur_pos_arg++;
		}
	}
	// Set remaining args to defaults
	while (cur_pos_arg < func.n_params) {
		if (!set_vars[cur_pos_arg] && func.params[cur_pos_arg].default_value) {
			switch (func.params[cur_pos_arg].type)
			{
			case T_INT:
				args[cur_pos_arg].v_int = *((int*)func.params[cur_pos_arg].default_value);
				break;
			case T_FLOAT:
				args[cur_pos_arg].v_float = *((float*)func.params[cur_pos_arg].default_value);
				break;
			case T_HEX_COLOR:
				args[cur_pos_arg].v_hex_color = *((HexColor*)func.params[cur_pos_arg].default_value);
				break;
			default: return CMD_FAILURE;
			}
		}
		else return CMD_NOT_ENOUGH_ARGS;
		cur_pos_arg++;
	}
	retflag = false;
	return {};
}

static CommandStatus console_submit_command(std::string& response) {
	ConsoleLexer lexer(console_line);
	char* cmd = lexer.emit_token();
	char* equals = strchr(cmd, '=');
	if (equals) {
		*equals = 0;
		auto status = set_variable(cmd, equals + 1);
		response = status_string(status);
		return status;
	}
	else if (strcmp(cmd, "set") == 0) {
		auto var = lexer.emit_token();
		auto val = lexer.emit_token();
		auto status = set_variable(var, val);
		response = status_string(status);
		return status;
	}
	else if (strcmp(cmd, "get") == 0) {
		auto var = lexer.emit_token();
		return get_variable(var);
	}
	else {
		auto func = lookup_command(cmd);
		if (func == nullptr) {
			response = "Command \"";
			response += cmd;
			response += "\" does not exist";
			return CMD_NOT_FOUND;
		}
		Any* args = temp_alloc(Any, func->n_params);
		{
			bool retflag;
			CommandStatus retval = get_func_args(*func, lexer, equals, args, retflag);
			if (retflag) {
				response = status_string(retval);
				return retval;
			}
		}
		Any result;
		auto status = (*func->ptr)(func->n_params, args, &result);
		if (status == CMD_OK) {
			switch (func->ret_type) {
			case T_VOID: {
				response = "Done";
			} break;
			case T_INT: {
				response = std::to_string(result.v_int);
			} break;
			case T_FLOAT: {
				response = std::to_string(result.v_float);
			} break;
			case T_HEX_COLOR: {
				char hex_color_str[9];
				snprintf(hex_color_str, 9, "%08X", result.v_hex_color);
				response = hex_color_str;
			} break;
			case T_BOOL: {
				response = result.v_bool ? "true" : "false";
			} break;
			}
		}
		return status;
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
	stb_textedit_initialize_state(&console_state, true);
	console_state.cursor = console_line.size();
}

void init_console() {
	stb_textedit_initialize_state(&console_state, true);
}

int console_type_key(int keycode) {
	if (keycode == GLFW_KEY_ENTER) {
		std::string response;
		auto status = console_submit_command(response);
		// TODO: print some sort of response in a command history
		console_scrollback.push_back({console_line, ConsoleScrollback::COMMAND});
		console_scrollback.push_back({response, ConsoleScrollback::RESPONSE, status});
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

static size_t escape_hash(char* const dest, const std::string& src, size_t buf_size) {
	int i = 0;
	auto end = src.cend();
	for (auto it = src.cbegin(); it != end; it++) {
		if (i + 1 >= buf_size) break;
		dest[i++] = *it;
		if (*it == '#') {
			if (i + 1 >= buf_size) break;
			dest[i++] = '#';
		}
	}
	dest[i] = 0;
	return i;
}

const char* get_console_line(bool show_cursor = true) {
	size_t capacity = console_line.size() * 2 + 16;
	char* buf = temp_alloc(char, capacity);
	int i = 0;
	if (show_cursor) {
		i = snprintf(buf, capacity, "\x1%d\x2", console_state.cursor + 2);
	}
	buf[i++] = '>';
	buf[i++] = ' ';
	escape_hash(buf + i, console_line, capacity - i);
	return buf;
}

const char* get_console_scrollback_line(int line_offset) {
	if (line_offset <= 0) return nullptr;
	int sb_index = console_scrollback.size() - line_offset;
	if (sb_index < 0) return nullptr;
	const auto& sb_line = console_scrollback[sb_index];
	size_t capacity = sb_line.contents.size() * 2 + 32;
	char* buf = temp_alloc(char, capacity);
	switch (sb_line.type) {
	case ConsoleScrollback::COMMAND: {
		int i = snprintf(buf, capacity, "#c[888]> ");
		escape_hash(buf + i, sb_line.contents.c_str(), capacity - i);
		return buf;
	}
	case ConsoleScrollback::RESPONSE: {
		int i = snprintf(buf, capacity, "#c[888]---> #c[%06x]", status_color(sb_line.resp_status));
		escape_hash(buf + i, sb_line.contents.c_str(), capacity - i);
		return buf;
	}
	}
}