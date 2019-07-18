#include <string>

#include <glfw3.h>

#include "text.h"
#include "console.h"

constexpr int MOD_SHIFT = GLFW_MOD_SHIFT   << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_ALT   = GLFW_MOD_ALT     << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_CTRL  = GLFW_MOD_CONTROL << GLFW_TO_CONSOLE_SHIFT;
constexpr int MOD_SUPER = GLFW_MOD_SUPER   << GLFW_TO_CONSOLE_SHIFT;

static char key_to_char(int key) {
	if (key & (MOD_ALT | MOD_CTRL | MOD_SUPER)) return 0;
	int base_char = key & 0x7f; // strip to ascii range
	if (key & MOD_SHIFT) {
		switch (base_char) {
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
		default: return base_char;
		}
	}
	else {
		if (isalpha(base_char)) return tolower(base_char);
		else return base_char;
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

// GLOBALS
static STB_TexteditState console_state;
static std::string console_text;
static char console_line[1024];

//    void stb_textedit_initialize_state(STB_TexteditState *state, int is_single_line)
//
//    void stb_textedit_click(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    void stb_textedit_drag(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
//    int  stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
//    int  stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE *text, int len)
//    void stb_textedit_key(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXEDIT_KEYTYPE key)

#include "generated/console_commands.h"

static void console_submit_command() {
	console_text.clear(); // TEMP
	stb_textedit_initialize_state(&console_state, true);
}

static void console_history_scroll(int offset) {
}

void init_console() {
	stb_textedit_initialize_state(&console_state, true);
}

int console_type_key(int keycode) {
	if (keycode == GLFW_KEY_UP) {
		console_history_scroll(-1);
	}
	else if (keycode == GLFW_KEY_DOWN) {
		console_history_scroll(+1);
	}
	else if (keycode == GLFW_KEY_TAB) {
		// TODO: autocomplete
	}
	else {
		stb_textedit_key(&console_text, &console_state, keycode);
	}
	return 0;
}

const char* get_console_line(bool show_cursor = true) {
	if (show_cursor) {
		sprintf(console_line, "\x1%d\x2%s", console_state.cursor, console_text.c_str());
		return console_line;
	}
	else {
		return console_text.c_str();
	}
}