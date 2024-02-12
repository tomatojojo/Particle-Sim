#define _USE_MATH_DEFINES

#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <ctime>
#include <cmath>
#include <random>

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

std::vector<Particle> particles;

void SpawnRandomParticle() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> disX(0, 1280);
	std::uniform_real_distribution<> disY(0, 720);
	std::uniform_real_distribution<> disAngle(-180, 180);
	std::uniform_real_distribution<> disVelocity(10, 100);

	int x = static_cast<int>(disX(gen));
	int y = static_cast<int>(disY(gen));
	double angle = disAngle(gen);
	double velocity = disVelocity(gen);

	particles.emplace_back(x, y, angle, velocity);
}

static void GLFWErrorCallback(int error, const char* description)
{
	std::cout << "GLFW Error " <<  description << " code: " << error << std::endl;
}

void DrawParticles() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	for (const auto& particle : particles) {
		// Convert particle position to screen coordinates
		ImVec2 pos = ImVec2(particle.x, particle.y);
		// Draw a filled circle at the particle's position
		draw_list->AddCircleFilled(pos, 2.0f, IM_COL32(255, 255, 255, 255)); // White color
	}
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

		// Set the window background color to black
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		// Begin the black panel with no title bar and no resize option
		ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always); // Center the window
		ImGui::Begin("Black Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		// Get the draw list for the black panel
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		DrawParticles();

		// End the window
		ImGui::End();

		// Pop the style color to restore the default settings
		ImGui::PopStyleColor();

		// Create a new window for the button
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
		ImGui::SetNextWindowSizeConstraints(ImVec2(640, 300), ImVec2(640, 300)); // Set size constraints
		ImGui::SetNextWindowPos(ImVec2(1280, 0), ImGuiCond_Always); // Positioned to the right of the black panel
		ImGui::Begin("Button Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		// Check if the button was clicked
		if (ImGui::Button("Spawn Particle")) {
			SpawnRandomParticle();
		}

		// End the button window
		ImGui::End();

		// Pop the style var to restore the default settings
		ImGui::PopStyleVar();

		// Update and draw particles
		for (auto& particle : particles) {
			particle.UpdatePosition(0.1);
		}

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
	glfwDestroyWindow(window);
	glfwTerminate();
}
