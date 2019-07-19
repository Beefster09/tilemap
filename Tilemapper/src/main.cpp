
#include <glad/glad.h>
#include <glfw3.h>

#include <cstdio>

#include "renderer.h"
#include "text.h"
#include "console.h"

int virtual_width = 512;
int virtual_height = 288;
int screen_width = virtual_width * 3;
int screen_height = virtual_height * 3;

Tile simple_tilemap[] = {
	2,       0, 2|HFLIP,       0, 2|DFLIP,       0, 2|HFLIP|DFLIP, 0,
	2|VFLIP, 0, 2|HFLIP|VFLIP, 0, 2|DFLIP|VFLIP, 0, 2|HFLIP|DFLIP|VFLIP, 0,
	2, 0, rotateCW(2), 0, rotateCW(rotateCW(2)), filter_tile(0, 1.f, 0.5f, 0.f), rotateCCW(2), 0,
	2|VFLIP, 0, rotateCW(2|VFLIP), 0, rotateCW(rotateCW(2|VFLIP)), 0, rotateCCW(2|VFLIP), 0,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;
	glViewport(0, 0, width, height);
}

bool console_active = true;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (key) {
	case GLFW_KEY_RIGHT_SHIFT:
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_CONTROL:
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_ALT:
	case GLFW_KEY_LEFT_ALT:
	case GLFW_KEY_RIGHT_SUPER:
	case GLFW_KEY_LEFT_SUPER:
		return;
	}
	if (console_active && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		console_type_key(key | (mods << GLFW_TO_CONSOLE_SHIFT));
	}
}

constexpr float FPS_SMOOTHING = 0.9f;
constexpr float CURSOR_BLINK_PERIOD = 1.f;
constexpr float CURSOR_BLINK_DUTY_CYCLE = 0.5f * CURSOR_BLINK_PERIOD;

extern float scaling_sharpness;

int main(int argc, char* argv[]) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __MACOS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Tilemapper", NULL, NULL);
	if (window == NULL) {
		printf("Failed to create GLFW window.\n");
		glfwTerminate();
		getchar();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		glfwTerminate();
		getchar();
		return -1;
	}

	glViewport(0, 0, screen_width, screen_height);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	init_console();
	glfwSetKeyCallback(window, key_callback);

	{
		init_simple_font();
		Renderer renderer(window, virtual_width, virtual_height);

		auto tileset = load_tileset("assets/tileset24bit.png", 16);
		TileChunk test_chunk(tileset, simple_tilemap, 4, 4);
		renderer.add_chunk(&test_chunk, 8, 8, 0);
		renderer.add_chunk(&test_chunk, 64, 24, -2);
		renderer.add_chunk(&test_chunk, 60, 20, -2);
		renderer.add_chunk(&test_chunk, 96, 16, -3);
		renderer.add_chunk(&test_chunk, 125, 35, 0);
		auto blah = renderer.add_chunk(&test_chunk, 150, 80, 2);
		auto blah_base_x = blah->x;
		auto blah_base_y = blah->y;

		auto spritesheet = load_spritesheet("assets/tileset24bit.png");
		renderer.add_sprite(spritesheet, 120.f, 74.f, 1, 0, 0, 16, 16, 0);
		renderer.add_sprite(spritesheet, 10.f, 11.f, 0, 17, 2, 8, 8, 0);
		auto meh = renderer.add_sprite(spritesheet, 127.f, 90.f, 2, 47, 93, 15, 21, 0);
		auto meh_base_x = meh->attrs.x;
		auto meh_base_y = meh->attrs.y;

		logOpenGLErrors();

		bool pressed = false;
		float last_frame_time = glfwGetTime();
		float frame_period = 0.016667f;

		int r = 0xf, g = 0, b = 0;
		bool show_fps = true;
		bool f2_last_frame = false;

		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			float time = glfwGetTime();
			float diff = time - last_frame_time;
			if (diff < frame_period * 5.f) { // ignore outliers
				frame_period = (frame_period * FPS_SMOOTHING) + (diff * (1.f - FPS_SMOOTHING));
			}
			last_frame_time = time;
			float fps = 1.f / frame_period;

			if (r == 0xf) {
				if (b > 0) b--;
				else if (g == 0xf) r--;
				else g++;
			}
			else if (g == 0xf) {
				if (r > 0) r--;
				else if (b == 0xf) g--;
				else b++;
			}
			else if (b == 0xf) {
				if (g > 0) g--;
				else if (r == 0xf) b--;
				else r++;
			}

			blah->x = 96 * sinf(time * TAU * 0.8) + blah_base_x;
			blah->y = 52 * cosf(time * TAU * 1.2) + blah_base_y;

			meh->attrs.x = 120 * sinf(time * TAU * 0.3) + meh_base_x;
			meh->attrs.y = 12 * cosf(time * TAU * 0.5) + meh_base_y;

			int f2_state = glfwGetKey(window, GLFW_KEY_F2);
			if (f2_state == GLFW_RELEASE) {
				if (f2_last_frame == GLFW_PRESS) {
					show_fps = !show_fps;
				}
			}
			f2_last_frame = f2_state;

			renderer.print_text(200, 1, "#c[7f1]Scaling sharpness: %.3f\n", scaling_sharpness);
			renderer.print_text(88, 74, "The quick brown fox\n#c[%x%x%x]jumps#0 over the lazy dog.", r, g, b);
			renderer.print_text(88, 100, "HOW\tVEXINGLY\tQUICK\nDAFT\tZEBRAS\tJUMP!\nLycanthrope: Werewolf.\nLVA\niji\nf_J,T.V,P.");
			renderer.print_text(300, 20, "01234,56789_ABC;DEF.##$");
			renderer.print_text(10, virtual_height - 20, "%s", get_console_line(fmod(time, CURSOR_BLINK_PERIOD) < CURSOR_BLINK_DUTY_CYCLE) );
			renderer.draw_frame(fps, show_fps);

			logOpenGLErrors();

			if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, true);
			}

			temp_storage_clear();
		}
	}

	glfwTerminate();

	return 0;
}
