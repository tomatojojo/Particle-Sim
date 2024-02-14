#include <iostream>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <ctime>
#include <cmath>
#include <random>
#include <future>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

using namespace std;

static GLFWwindow* window = nullptr;
float PI = 3.14159265359;
ImVec4 wallColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
ImVec4 particleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

class Wall {
public:
	float startX, startY, endX, endY;

	Wall(float startX, float startY, float endX, float endY)
		: startX(startX), startY(startY), endX(endX), endY(endY) {}

	void DrawWall() {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 start = ImVec2(startX, 720 - startY);
		ImVec2 end = ImVec2(endX, 720 - endY);
		draw_list->AddLine(start, end, ImColor(wallColor), 1.0f);
	}

};

std::vector<Wall> walls;

static float getDistance(float x1, float y1, float x2, float y2) {
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

static float pointLineDistance(float px, float py, float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float t = ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy);
	float closestX = x1 + t * dx;
	float closestY = y1 + t * dy;

	return sqrt((closestX - px) * (closestX - px) + (closestY - py) * (closestY - py));
}

static float reflectAngle(Wall wall, float angle) {
	float wallAngle = atan2(wall.endY - wall.startY, wall.endX - wall.startX) * 180.0 / PI;
	float reflectedAngle = 2 * wallAngle - angle;

	reflectedAngle = fmod(reflectedAngle, 360.0f);

	return reflectedAngle;
}

class Particle {
public:
	float x, y;
	float angle; 
	float velocity;

	Particle(float x, float y, float angle, float velocity)
		: x(x), y(y), angle(angle), velocity(velocity) {}

	void UpdatePosition(float deltaTime) {
		float radians = angle * PI / 180.0;

		float dx = cos(radians) * velocity * deltaTime;
		float dy = sin(radians) * velocity * deltaTime;

		float newX = x + dx;
		float newY = y + dy;

		// Threshold for collision detection
		float threshold = velocity > 500 ? 10.0f : 3.0f;

		bool collisionDetected = false;
		Wall* collidedWall = nullptr;

		for (auto& wall : walls) {
			float lineDistance = pointLineDistance(newX, newY, wall.startX, wall.startY, wall.endX, wall.endY);
			float wallStartDistance = getDistance(newX, newY, wall.startX, wall.startY);
			float wallEndDistance = getDistance(newX, newY, wall.endX, wall.endY);

			if (lineDistance < threshold || wallStartDistance < threshold || wallEndDistance < threshold) {
				float dx = newX - wall.startX;
				float dy = newY - wall.startY;
				float wallLength = getDistance(wall.startX, wall.startY, wall.endX, wall.endY);
				float t = (dx * (wall.endX - wall.startX) + dy * (wall.endY - wall.startY)) / (wallLength * wallLength);

				if (t >= 0 && t <= 1) {
					collisionDetected = true;
					collidedWall = &wall;
					break;
				}
			}
		}

		if (collisionDetected) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> disOffset(-0.3f, 0.3f);
			float offsetX = disOffset(gen);
			float offsetY = disOffset(gen);

			x += offsetX;
			y += offsetY;

			if (getDistance(newX, newY, collidedWall->startX, collidedWall->startY) < threshold || 
				getDistance(newX, newY, collidedWall->endX, collidedWall->endY) < threshold) {
				//std::cout << "hIT THE TIP" << std::endl;
				angle = fmod(angle + 180, 360.0f);
			} else {
				angle = reflectAngle(*collidedWall, angle);
			}
			
			radians = angle * PI / 180.0;
			dx = cos(radians) * velocity * deltaTime;
			dy = sin(radians) * velocity * deltaTime;
			newX = x + dx;
			newY = y + dy;
		}

		x = newX;
		y = newY;

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

static void SpawnRandomParticle() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> disX(0, 1280);
	std::uniform_real_distribution<> disY(0, 720);
	std::uniform_real_distribution<> disAngle(0, 360);
	std::uniform_real_distribution<> disVelocity(10, 300);

	float x, y;
	bool validPosition = false;

	while (!validPosition) {
		x = disX(gen);
		y = disY(gen);

		validPosition = true;
		for (const auto& wall : walls) {
			if (x >= wall.startX && x <= wall.endX && y >= wall.startY && y <= wall.endY) {
				validPosition = false;
				break;
			}
		}
	}

	float angle = disAngle(gen);
	float velocity = disVelocity(gen);

	particles.emplace_back(x, y, angle, velocity);
}

static void SpawnRandomWall() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> disStartX(0, 1280);
	std::uniform_real_distribution<> disStartY(0, 720);
	std::uniform_real_distribution<> disEndX(0, 1280);
	std::uniform_real_distribution<> disEndY(0, 720);
	
	float startX = disStartX(gen);
	float startY = disStartY(gen);
	float endX = disEndX(gen);
	float endY = disEndY(gen);

	for (auto& particle : particles) {
		if (particle.x >= startX && particle.x <= endX && particle.y >= startY && particle.y <= endY) {
			float offsetX = (particle.x < (startX + endX) / 2) ? -10.0f : 10.0f;
			float offsetY = (particle.y < (startY + endY) / 2) ? -10.0f : 10.0f;

			bool insideAnotherWall = false;
			for (const auto& otherWall : walls) {
				if (particle.x + offsetX >= otherWall.startX && particle.x + offsetX <= otherWall.endX &&
					particle.y + offsetY >= otherWall.startY && particle.y + offsetY <= otherWall.endY) {
					insideAnotherWall = true;
					break;
				}
			}

			if (!insideAnotherWall && particle.x + offsetX >= 0 && particle.x + offsetX <= 1280 &&
				particle.y + offsetY >= 0 && particle.y + offsetY <= 720) {
				particle.x += offsetX;
				particle.y += offsetY;
			}
		}
	}

	walls.emplace_back(startX, startY, endX, endY);	
}

static void GLFWErrorCallback(int error, const char* description) {
	std::cout << "GLFW Error " <<  description << " code: " << error << std::endl;
}

static void DrawElements() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	for (const auto& particle : particles) {
		ImVec2 pos = ImVec2(particle.x, 720 - particle.y);

		draw_list->AddCircleFilled(pos, 1.5f, ImColor(particleColor));
	}

	for (auto& wall : walls) {
		wall.DrawWall();
	}
}

void UpdateParticlesRange(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end, float deltaTime) {
	for (auto it = begin; it != end; ++it) {
		it->UpdatePosition(deltaTime);
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

	glfwMaximizeWindow(window);
	glfwMakeContextCurrent(window);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	char newParticleXStr[16] = "";
	char newParticleYStr[16] = "";
	char newParticleAngleStr[16] = "";
	char newParticleVelocityStr[16] = "";

	bool showErrorPopup = false;

	char numParticlesStr[16] = "";
	int numParticles = 0;
	int particleVariationType = 0; //  0: Varying X and Y,  1: Varying Angle,  2: Varying Velocity
	float startX = 0.0f, endX = 0.0f;
	float startY = 0.0f, endY = 0.0f;
	float startAngle = 0.0f, endAngle = 0.0f;
	float startVelocity = 0.0f, endVelocity = 0.0f;

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

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::Begin("Black Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		DrawElements();

		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(1280, 100), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(0, 720), ImGuiCond_Once);

		ImGui::Begin("Color Pickers", nullptr, ImGuiWindowFlags_NoDecoration);

		ImGui::ColorEdit3("Wall Color", (float*)&wallColor);
		ImGui::ColorEdit3("Particle Color", (float*)&particleColor);

		ImGui::End();

		ImGui::PopStyleColor();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::SetNextWindowSizeConstraints(ImVec2(640, 1080), ImVec2(640, 1080));
		ImGui::SetNextWindowPos(ImVec2(1280, 0), ImGuiCond_Always);
		ImGui::Begin("Button Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		if (ImGui::Button("Spawn Random Particle")) {
			SpawnRandomParticle();
		}

		ImGui::SameLine(); 
		if (ImGui::Button("Spawn Random Wall")) {
			SpawnRandomWall();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset Particles")) {
			particles.clear();
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear Walls")) {
			walls.clear();
		}

		ImGui::InputText("X Coordinate", newParticleXStr, sizeof(newParticleXStr));
		ImGui::InputText("Y Coordinate", newParticleYStr, sizeof(newParticleYStr));
		ImGui::InputText("Angle (degrees)", newParticleAngleStr, sizeof(newParticleAngleStr));
		ImGui::InputText("Velocity (pixels/sec)", newParticleVelocityStr, sizeof(newParticleVelocityStr));

		if (ImGui::Button("Add Particle")) {
			float newParticleX = atof(newParticleXStr);
			float newParticleY = atof(newParticleYStr);
			float newParticleAngle = atof(newParticleAngleStr);
			float newParticleVelocity = atof(newParticleVelocityStr);

			bool insideWall = false;
			Wall* collidingWall = nullptr;

			for (auto& wall : walls) {
				if (newParticleX >= wall.startX && newParticleX <= wall.endX &&
					newParticleY >= wall.startY && newParticleY <= wall.endY) {
					insideWall = true;
					collidingWall = &wall;
					break;
				}
			}

			if (insideWall) {
				if (newParticleX < collidingWall->endX && !(collidingWall->endX >= 1280)) {
					newParticleX = collidingWall->endX + 1.0f;
				}
				else {
					newParticleX = collidingWall->startX - 1.0f;
				}
			}

			//std::cout << "New particle velocity: " << newParticleVelocity << std::endl; // Debug output

			if (newParticleX >= 0 && newParticleX <= 1280 &&
				newParticleY >= 0 && newParticleY <= 720 &&
				newParticleAngle >= 0.0 && newParticleAngle <= 360.0) {
				particles.emplace_back(newParticleX, newParticleY, newParticleAngle, newParticleVelocity);
			}
			else {
				showErrorPopup = true;
			}
		}

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
		
		ImGui::Text("Current FPS: %.f", currentFramerate);

		ImGui::Text("Number of Particles: %d", particles.size());

		ImGui::InputText("Number of Particles", numParticlesStr, sizeof(numParticlesStr));
		numParticles = atoi(numParticlesStr);

		const char* particleVariationTypes[] = { "Varying X and Y", "Varying Angle", "Varying Velocity" };
		ImGui::Combo("Particle Variation Type", &particleVariationType, particleVariationTypes, IM_ARRAYSIZE(particleVariationTypes));
		ImGui::InputFloat("Start X", &startX);
		ImGui::InputFloat("Start Y", &startY);
		ImGui::InputFloat("End X", &endX);
		ImGui::InputFloat("End Y", &endY);
		ImGui::InputFloat("Start Angle", &startAngle);
		ImGui::InputFloat("End Angle", &endAngle);
		ImGui::InputFloat("Start Velocity", &startVelocity);
		ImGui::InputFloat("End Velocity", &endVelocity);

		if (ImGui::Button("Add Batch Particles")) {
			float dX = (endX - startX) / (numParticles - 1);
			float dY = (endY - startY) / (numParticles - 1);
			float dAngle = (endAngle - startAngle) / (numParticles - 1);
			float dVelocity = (endVelocity - startVelocity) / (numParticles - 1);

			for (int i = 0; i < numParticles; ++i) {
				float x = startX + i * dX;
				float y = startY + i * dY;
				float angle = startAngle + i * dAngle;
				float velocity = startVelocity + i * dVelocity;

				bool insideWall = false;
				Wall* collidingWall = nullptr;

				for (auto& wall : walls) {
					if (x >= wall.startX && x <= wall.endX &&
						y >= wall.startY && y <= wall.endY) {
						insideWall = true;
						collidingWall = &wall;
						break;
					}
				}

				if (insideWall) {
					if (x < collidingWall->endX && !(collidingWall->endX >= 1280)) {
						x = collidingWall->endX + 1.0f;
					}
					else {
						x = collidingWall->startX - 1.0f;
					}
				}

				switch (particleVariationType) {
					case 0: // Varying X and Y
						particles.emplace_back(x, y, startAngle, startVelocity);
						break;
					case 1: // Varying Angle
						angle = fmod(angle, 360.0f);
						particles.emplace_back(startX, startY, angle, startVelocity);
						break;
					case 2: // Varying Velocity
						particles.emplace_back(startX, startY, startAngle, velocity);
						break;
				}
				//std::cout << "Particle position: (" << x << ", " << y << ")" << std::endl;
			}
		}

		ImGui::InputFloat("Wall Start X", &wallStartX);
		ImGui::InputFloat("Wall Start Y", &wallStartY);
		ImGui::InputFloat("Wall End X", &wallEndX);
		ImGui::InputFloat("Wall End Y", &wallEndY);

		if (ImGui::Button("Add Wall")) {
			walls.emplace_back(wallStartX, wallStartY, wallEndX, wallEndY);
		}

		ImGui::End();

		ImGui::PopStyleVar();

		while (accumulator >= timeStep) {
			std::vector<std::future<void>> futures;
			size_t numParticles = particles.size();
			size_t numThreads = std::thread::hardware_concurrency();
			size_t chunkSize = numParticles / numThreads;

			for (size_t i = 0; i < numThreads; ++i) {
				auto startIter = particles.begin() + i * chunkSize;
				auto endIter = (i == numThreads - 1) ? particles.end() : startIter + chunkSize;
				futures.push_back(std::async(std::launch::async, UpdateParticlesRange, startIter, endIter, timeStep));
			}

			for (auto& future : futures) {
				future.wait();
			}
			accumulator -= timeStep;
		}

		if (frameTime >= targetFrameTime) {
			lastUpdateTime = currentTime;
		}

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
		glfwPollEvents();
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
