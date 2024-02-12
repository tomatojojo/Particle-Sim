#define _USE_MATH_DEFINES

#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <ctime>
#include <cmath>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

using namespace std;

static GLFWwindow* window = nullptr;
class Particle {
public:
	int x, y;
	double angle; // Angle in degrees
	double velocity; // Velocity in pixels per second

	Particle(int x, int y, double angle, double velocity)
		: x(x), y(y), angle(angle), velocity(velocity) {}

	void UpdatePosition(double deltaTime) {
		// Convert angle to radians
		double radians = -angle * M_PI / 180.0;

		// Calculate the change in position
		double dx = cos(radians) * velocity * deltaTime;
		double dy = sin(radians) * velocity * deltaTime;

		// Update position
		x += static_cast<int>(dx);
		y += static_cast<int>(dy);

		y = 720 - y; // Invert y-axis

		// Bounce off the walls (ADJUST HERE)
		if (x < 0) {
			x = 0;
			angle = 180 - angle; // Reflect angle
		}
		else if (x > 1280) {
			x = 1280;
			angle = 180 - angle;
		}

		if (y < 0) {
			y = 0;
			angle = -angle;
		}
		else if (y > 720) {
			y = 720;
			angle = -angle;
		}
		y = 720 - y; // Invert y-axis
	}
};

static void GLFWErrorCallback(int error, const char* description)
{
	std::cout << "GLFW Error " <<  description << " code: " << error << std::endl;
}

int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to initialize GLFW" << std::endl;
		std::cin.get();
	}

	glfwSetErrorCallback(GLFWErrorCallback);

	window = glfwCreateWindow(1280, 720, "Particle Sim", NULL, NULL);

	glfwMakeContextCurrent(window);

	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Hello, world!");
		ImGui::Text("Text");
		ImGui::End();

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (window)
	{
		glfwDestroyWindow(window);
	}
	glfwTerminate();
}
