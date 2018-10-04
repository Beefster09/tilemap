
#include <glad/glad.h> 
#include <glfw3.h>

#include <cstdio>

#include "renderer.h"

//int screen_width = 1280;
int screen_width = 512 * 2;
//int screen_height = 720;
int screen_height = 288 * 2;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;
	glViewport(0, 0, width, height);
}

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

		float base_sharp = renderer.get_sharpness();
		bool pressed = false;

		while(!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

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
			}

			renderer.draw_frame();

			if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, true);
			}
		}
	}

	glfwTerminate();

	return 0;
}