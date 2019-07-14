
#include <glad/glad.h> 
#include <glfw3.h>

#include <cstdio>

#include "renderer.h"

//int screen_width = 1280;
int screen_width = 512 * 2;
//int screen_height = 720;
int screen_height = 288 * 2;

Tile simple_tilemap[] = {
	2,       0, 2|HFLIP,       0, 2|DFLIP,       0, 2|HFLIP|DFLIP, 0,
	2|VFLIP, 0, 2|HFLIP|VFLIP, 0, 2|DFLIP|VFLIP, 0, 2|HFLIP|DFLIP|VFLIP, 0,
	2, 0, rotateCW(2), 0, rotateCW(rotateCW(2)), filter(0, 1.f, 0.5f, 0.f), rotateCCW(2), 0,
	2|VFLIP, 0, rotateCW(2|VFLIP), 0, rotateCW(rotateCW(2|VFLIP)), 0, rotateCCW(2|VFLIP), 0,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;
	glViewport(0, 0, width, height);
}
constexpr float FPS_SMOOTHING = 0.9f;

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

	{
		Renderer renderer(window, 512, 288);
		//Renderer renderer(window, 32, 32);

		auto tileset = load_tileset("assets/tileset24bit.png", 16);
		TileChunk test_chunk(tileset, simple_tilemap, 4, 4);
		renderer.add_chunk(&test_chunk, 8, 8, 0);
		renderer.add_chunk(&test_chunk, 64, 24, -2);
		renderer.add_chunk(&test_chunk, 60, 20, -2);
		renderer.add_chunk(&test_chunk, 96, 16, -3);
		renderer.add_chunk(&test_chunk, 125, 35, 2);
		auto blah = renderer.add_chunk(&test_chunk, 150, 80, 2);
		auto blah_base_x = blah->x;
		auto blah_base_y = blah->y;

		Texture spritesheet("assets/tileset24bit.png");
		//renderer.add_sprite();

		logOpenGLErrors();

		float base_sharp = renderer.get_sharpness();
		bool pressed = false;
		float last_frame_time = glfwGetTime();
		float frame_period = 0.f;

		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			float time = glfwGetTime();
			float diff = time - last_frame_time;
			last_frame_time = time;
			frame_period = (frame_period * FPS_SMOOTHING) + (diff * (1.f - FPS_SMOOTHING));
			float fps = 1.f / frame_period;

			blah->x = 96 * sinf(time * TAU * 0.8) + blah_base_x;
			blah->y = 52 * cosf(time * TAU * 1.2) + blah_base_y;

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
				double mouse_x, mouse_y;
				glfwGetCursorPos(window, &mouse_x, &mouse_y);
				if (!pressed) {
					base_sharp = renderer.get_sharpness() - mouse_x / screen_width;
					pressed = true;
				}
				else {
					renderer.set_sharpness(mouse_x / screen_width + base_sharp);
				}
				printf("Scaling sharpness: %.3f\n", renderer.get_sharpness());
			}
			else {
				pressed = false;
				for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; key++) {
					if(glfwGetKey(window, key)) {
						renderer.set_sharpness((float) (key - GLFW_KEY_0) * 0.25);
						printf("Scaling sharpness: %.3f\n", renderer.get_sharpness());
					}
				}
			}

			renderer.draw_frame(fps, true);

			logOpenGLErrors();

			if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, true);
			}
		}
	}

	glfwTerminate();

	return 0;
}