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
float PI = 3.14159265359;

class Wall {
public:
	float startX, startY, endX, endY;

	Wall(float startX, float startY, float endX, float endY)
		: startX(startX), startY(startY), endX(endX), endY(endY) {}

	void DrawWall() {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 start = ImVec2(startX, 720 - startY);
		ImVec2 end = ImVec2(endX, 720 - endY);
		draw_list->AddLine(start, end, IM_COL32(255, 255, 255, 255), 1.0f);
	}

};

std::vector<Wall> walls;

float getDistance(float x1, float y1, float x2, float y2) {
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

// Function to calculate the distance from a point to a line segment
float pointLineDistance(float px, float py, float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float t = ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy);

	float closestX = x1 + t * dx;
	float closestY = y1 + t * dy;

	return sqrt((closestX - px) * (closestX - px) + (closestY - py) * (closestY - py));
}

float reflectAngle(Wall wall, float angle) {
	// Wall angle
	float wallAngle = atan2(wall.endY - wall.startY, wall.endX - wall.startX) * 180.0 / PI;

	// Reflect the particle's angle based on the wall's angle
	float reflectedAngle = 2 * wallAngle - angle;

	reflectedAngle = fmod(reflectedAngle, 360.0f);

	return reflectedAngle;
}

class Particle {
public:
	float x, y;
	float angle; // Angle in degrees
	float velocity; // Velocity in pixels per second

	Particle(float x, float y, float angle, float velocity)
		: x(x), y(y), angle(angle), velocity(velocity) {}

	void UpdatePosition(float deltaTime) {
		// Convert angle to radians
		float radians = angle * PI / 180.0;

		// Calculate the change in position
		float dx = cos(radians) * velocity * deltaTime;
		float dy = sin(radians) * velocity * deltaTime;

		// Update position
		float newX = x + dx;
		float newY = y + dy;

		// Check if the new position collides with any wall
		bool collisionDetected = false;
		Wall* collidedWall = nullptr;
		for (auto& wall : walls) {
			float distance = pointLineDistance(newX, newY, wall.startX, wall.startY, wall.endX, wall.endY);
			float threshold = 3.0f; // Adjust this value based on the size of your particles
			if (distance < threshold) {
				// Ensure the particle is actually touching the wall before reflecting
				float dx = newX - wall.startX;
				float dy = newY - wall.startY;
				float wallLength = getDistance(wall.startX, wall.startY, wall.endX, wall.endY);
				float t = (dx * (wall.endX - wall.startX) + dy * (wall.endY - wall.startY)) / (wallLength * wallLength);
				if (t >= 0 && t <= 1) {
					// The particle is on the line segment, so it's a valid collision
					collisionDetected = true;
					collidedWall = &wall;
					break;
				}

			}
		}

		// If a collision is detected, reflect the particle and update its position
		if (collisionDetected) {
			angle = reflectAngle(*collidedWall, angle);
			// Recalculate the change in position based on the new angle
			radians = angle * PI / 180.0;
			dx = cos(radians) * velocity * deltaTime;
			dy = sin(radians) * velocity * deltaTime;
			newX = x + dx;
			newY = y + dy;
		}

		// If no collision with a wall, update the position normally
		x = newX;
		y = newY;

		// Reflect off window boundaries if necessary (existing code)
		if (x < 0) {
			x = 0;
			angle = 180 - angle;
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
	}
};

std::vector<Particle> particles;

void SpawnRandomParticle() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> disX(0, 1280);
	std::uniform_real_distribution<> disY(0, 720);
	std::uniform_real_distribution<> disAngle(0, 360);
	std::uniform_real_distribution<> disVelocity(70, 500);

	float x = disX(gen);
	float y = disY(gen);
	float angle = disAngle(gen);
	float velocity = disVelocity(gen);

	particles.emplace_back(x, y, angle, velocity);
}

static void GLFWErrorCallback(int error, const char* description) {
	std::cout << "GLFW Error " <<  description << " code: " << error << std::endl;
}

void DrawElements() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	for (const auto& particle : particles) {
		// Convert particle position to screen coordinates
		ImVec2 pos = ImVec2(particle.x, 720 - particle.y);

		// Draw a filled circle at the particle's position
		draw_list->AddCircleFilled(pos, 1.5f, IM_COL32(255, 255, 255, 255)); // White color
	}

	for (auto& wall : walls) {
		wall.DrawWall();
	}
}

int main(int argc, char *argv) {

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		std::cin.get();
	}

	glfwSetErrorCallback(GLFWErrorCallback);

	window = glfwCreateWindow(1280, 720, "Particle Sim", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Maximize the window
	glfwMaximizeWindow(window);

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

	// Batch particle variables
	char numParticlesStr[16] = "";
	int numParticles = 0;
	int particleVariationType = 0; //  0: Varying X and Y,  1: Varying Angle,  2: Varying Velocity
	float startX = 0.0f, endX = 0.0f;
	float startY = 0.0f, endY = 0.0f;
	float startAngle = 0.0f, endAngle = 0.0f;
	float startVelocity = 0.0f, endVelocity = 0.0f;

	// Wall variables
	float wallStartX = 0.0f, wallStartY = 0.0f;
	float wallEndX = 0.0f, wallEndY = 0.0f;

	double frameTime = 0.0; // Time since the last frame
	double targetFrameTime = 1.0 / 60.0; // Target time per frame (60 FPS)
	double updateInterval = 0.5; // Interval for updating particles (0.5 seconds)
	double lastUpdateTime = 0.0; // Last time particles were updated
	double lastFPSUpdateTime = 0.0; // Last time the framerate was updated

	const double targetFPS = 60.0;
	const std::chrono::duration<double> targetFrameDuration = std::chrono::duration<double>(1.0 / targetFPS);

	std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();

	const double timeStep = 1.0 / targetFPS; // Time step for updates
	double accumulator = 0.0; // Accumulates elapsed time

	double currentFramerate = 0.0;
	double lastUIUpdateTime = 0.0;

	while (!glfwWindowShouldClose(window)) {
		// Measure the time at the start of the loop
		double currentTime = glfwGetTime();
		frameTime = currentTime - lastUpdateTime;

		std::chrono::steady_clock::time_point currentTimeForDelta = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsedTime = currentTimeForDelta - prevTime;
		prevTime = currentTimeForDelta;

		accumulator += frameTime;

		if (elapsedTime > std::chrono::seconds(1)) {
			elapsedTime = std::chrono::duration<double>(1.0 / targetFPS);
		}

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

		DrawElements();

		// End the window
		ImGui::End();

		// Pop the style color to restore the default settings
		ImGui::PopStyleColor();

		// Create a new window for the button and input fields
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove padding
		ImGui::SetNextWindowSizeConstraints(ImVec2(640, 720), ImVec2(640, 720)); // Set size constraints
		ImGui::SetNextWindowPos(ImVec2(1280, 0), ImGuiCond_Always); // Positioned to the right of the black panel
		ImGui::Begin("Button Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		// Check if the Spawn Particle button was clicked
		if (ImGui::Button("Spawn Random Particle")) {
			SpawnRandomParticle();
		}

		ImGui::SameLine(); // Place the next item on the same line
		if (ImGui::Button("Reset Particles")) {
			particles.clear(); // Clear the particles vector
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear Walls")) {
			walls.clear();
		}

		// Text fields for inputting the new particle's properties
		ImGui::InputText("X Coordinate", newParticleXStr, sizeof(newParticleXStr));
		ImGui::InputText("Y Coordinate", newParticleYStr, sizeof(newParticleYStr));
		ImGui::InputText("Angle (degrees)", newParticleAngleStr, sizeof(newParticleAngleStr));
		ImGui::InputText("Velocity (pixels/sec)", newParticleVelocityStr, sizeof(newParticleVelocityStr));

		// Button to add the new particle to the canvas
		if (ImGui::Button("Add Particle")) {
			float newParticleX = atof(newParticleXStr);
			float newParticleY = atof(newParticleYStr);
			float newParticleAngle = atof(newParticleAngleStr);
			float newParticleVelocity = atof(newParticleVelocityStr);

			//std::cout << "New particle velocity: " << newParticleVelocity << std::endl; // Debug output

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
		
		// Display the current framerate in the UI
		ImGui::Text("Current FPS: %.f", currentFramerate); // Render the framerate value

		// Display the current number of particles
		ImGui::Text("Number of Particles: %d", particles.size());

		// Batch Particle UI elements
		
		// Text field for the number of particles
		ImGui::InputText("Number of Particles", numParticlesStr, sizeof(numParticlesStr));
		numParticles = atoi(numParticlesStr);

		// Dropdown menu for particle variation type
		const char* particleVariationTypes[] = { "Varying X and Y", "Varying Angle", "Varying Velocity" };
		ImGui::Combo("Particle Variation Type", &particleVariationType, particleVariationTypes, IM_ARRAYSIZE(particleVariationTypes));
		// Rows of text input fields for start and end values
		ImGui::InputFloat("Start X", &startX);
		ImGui::InputFloat("Start Y", &startY);
		ImGui::InputFloat("End X", &endX);
		ImGui::InputFloat("End Y", &endY);
		ImGui::InputFloat("Start Angle", &startAngle);
		ImGui::InputFloat("End Angle", &endAngle);
		ImGui::InputFloat("Start Velocity", &startVelocity);
		ImGui::InputFloat("End Velocity", &endVelocity);

		// Button to add the batch of particles
		if (ImGui::Button("Add Batch Particles")) {
			// Calculate the increments for each property
			float dX = (endX - startX) / (numParticles - 1);
			float dY = (endY - startY) / (numParticles - 1);
			float dAngle = (endAngle - startAngle) / (numParticles - 1);
			float dVelocity = (endVelocity - startVelocity) / (numParticles - 1);

			// Generate the batch of particles based on the input values
			for (int i = 0; i < numParticles; ++i) {
				float x = startX + i * dX;
				float y = startY + i * dY;
				float angle = startAngle + i * dAngle;
				float velocity = startVelocity + i * dVelocity;

				// Adjust the values based on the selected variation type
				switch (particleVariationType) {
					case 0: // Varying X and Y
						particles.emplace_back(x, 720 - y, startAngle, startVelocity);
						break;
					case 1: // Varying Angle
						angle = fmod(angle, 360.0f);
						particles.emplace_back(startX, 720 - startY, angle, startVelocity);
						break;
					case 2: // Varying Velocity
						// No additional adjustment needed for velocity
						particles.emplace_back(startX, 720 - startY, startAngle, velocity);
						break;
				}
				std::cout << "Particle position: (" << x << ", " << y << ")" << std::endl;
				
			}
		}

		// Text fields for wall start and end coordinates
		ImGui::InputFloat("Wall Start X", &wallStartX);
		ImGui::InputFloat("Wall Start Y", &wallStartY);
		ImGui::InputFloat("Wall End X", &wallEndX);
		ImGui::InputFloat("Wall End Y", &wallEndY);

		// Button to add the wall
		if (ImGui::Button("Add Wall")) {
			// Create wall objects and add them to the walls vector
			walls.emplace_back(wallStartX, wallStartY, wallEndX, wallEndY);
		}

		// End the button window
		ImGui::End();

		// Pop the style var to restore the default settings
		ImGui::PopStyleVar();

		// Only update particles if enough time has passed
		while (accumulator >= timeStep) {
			// std::cout << "Updating particles..." << std::endl; // Debug output
			for (auto& particle : particles) {
				particle.UpdatePosition(timeStep);
			}
			accumulator -= timeStep;
		}

		if (frameTime >= targetFrameTime) {
			lastUpdateTime = currentTime;
		}

		// Output the current framerate to the console
		if (currentTime - lastFPSUpdateTime >= updateInterval) {
			currentFramerate = 1.0 / frameTime;
			std::cout << "Framerate: " << currentFramerate << " FPS" << std::endl;
			lastFPSUpdateTime = currentTime;
		}

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwSwapInterval(0); // Disable VSync
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (window) {
		glfwDestroyWindow(window);
	}
	glfwTerminate();

	return 0;
}
