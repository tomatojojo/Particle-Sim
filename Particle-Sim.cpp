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

float PI = 3.14159265358979323846;

class Particle {
public:
	int x, y;
	double angle; // Angle in degrees
	double velocity; // Velocity in pixels per second

	Particle(int x, int y, double angle, double velocity)
		: x(x), y(y), angle(angle), velocity(velocity) {}

	void UpdatePosition(double deltaTime) {
		// Convert angle to radians
		double radians = angle * PI / 180.0;

		// Calculate the change in position
		float dx = cos(radians) * velocity;
		float dy = sin(radians) * velocity;

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
		ImVec2 pos = ImVec2(particle.x, 720 - particle.y);
		// Draw a filled circle at the particle's position
		draw_list->AddCircleFilled(pos, 3.0f, IM_COL32(255, 255, 255, 255)); // White color
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

	// Placeholders for input values for new individual particles
	char newParticleXStr[16] = "";
	char newParticleYStr[16] = "";
	char newParticleAngleStr[16] = "";
	char newParticleVelocityStr[16] = "";

	bool showErrorPopup = false;

	const double targetFPS = 60.0;
	const std::chrono::duration<double> targetFrameDuration = std::chrono::duration<double>(1.0 / targetFPS);

	std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

	while (!glfwWindowShouldClose(window))
	{
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsedTime = currentTime - prevTime;
		prevTime = currentTime;

		// Cap the elapsed time to avoid large time steps
		if (elapsedTime > std::chrono::seconds(1)) {
			elapsedTime = std::chrono::duration<double>(1.0 / targetFPS);
		}

		// Convert elapsed time to seconds
		double deltaTime = elapsedTime.count();

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

		// Create a new window for the button and input fields
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
		ImGui::SetNextWindowSizeConstraints(ImVec2(640, 360), ImVec2(640, 360)); // Set size constraints
		ImGui::SetNextWindowPos(ImVec2(1280, 0), ImGuiCond_Always); // Positioned to the right of the black panel
		ImGui::Begin("Button Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		// Check if the Spawn Particle button was clicked
		if (ImGui::Button("Spawn Random Particle")) {
			SpawnRandomParticle();
		}

		// Check if the "Reset Particles" button was clicked
		ImGui::SameLine(); // Place the next item on the same line
		if (ImGui::Button("Reset Particles")) {
			particles.clear(); // Clear the particles vector
		}

		// Text fields for inputting the new particle's properties
		ImGui::InputText("X Coordinate", newParticleXStr, sizeof(newParticleXStr));
		ImGui::InputText("Y Coordinate", newParticleYStr, sizeof(newParticleYStr));
		ImGui::InputText("Angle (degrees)", newParticleAngleStr, sizeof(newParticleAngleStr));
		ImGui::InputText("Velocity (pixels/sec)", newParticleVelocityStr, sizeof(newParticleVelocityStr));

		// Button to add the new particle to the canvas
		if (ImGui::Button("Add Particle")) {
			int newParticleX = atoi(newParticleXStr);
			int newParticleY = atoi(newParticleYStr);
			double newParticleAngle = atof(newParticleAngleStr);
			double newParticleVelocity = atof(newParticleVelocityStr);

			if (newParticleX >= 0 && newParticleX <= 1280 &&
				newParticleY >= 0 && newParticleY <= 720 &&
				newParticleAngle >= 0.0 && newParticleAngle <= 360.0) {
				particles.emplace_back(newParticleX, newParticleY, newParticleAngle, newParticleVelocity);
			}
			else {
				showErrorPopup = true; // Show error popup if conditions are not met
			}
		}

		// Display error popup if necessary
		if (showErrorPopup) {
			ImGui::OpenPopup("Invalid Input");
			if (ImGui::BeginPopupModal("Invalid Input", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("The input values for X and Y coordinates must be within the dimensions of the black panel (0-1280,   0-720).\nThe angle must be between   0 and   360 degrees.");
				if (ImGui::Button("OK")) {
					ImGui::CloseCurrentPopup();
					showErrorPopup = false;
				}
				ImGui::EndPopup();
			}
		}

		// End the button window
		ImGui::End();

		// Pop the style var to restore the default settings
		ImGui::PopStyleVar();

		// Update and draw particles
		for (auto& particle : particles) {
			particle.UpdatePosition(deltaTime);
		}

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		std::chrono::duration<double> sleepTime = targetFrameDuration - (std::chrono::steady_clock::now() - currentTime);
		if (sleepTime > std::chrono::duration<double>(0)) {
			std::this_thread::sleep_for(sleepTime);
		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
