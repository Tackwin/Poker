#include <stdio.h>

#include "poker.hpp"
#include "Profiler/Timer.hpp"

#include "IA/Population.hpp"
#include "IA/Genome.hpp"
#include "IA/Network.hpp"

#include <functional>

#include "GLFW/glfw3.h"

#include "imgui.h"
#include "Extensions/imgui_ext.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "Experiments.hpp"

#include "macros.hpp"

double game() {
	Game g;
	g.players[0].name = "Alice";
	g.players[1].name = " Bob ";
	g.players[2].name = "Criss";

	//game.verbose = true;

	auto t1 = seconds();
	g.play_game();
	auto t2 = seconds();

	return t2 - t1;
}


void test_genome() {
}

//int main() {
//	constexpr auto N = 1000;
//
//	test_xor();
//	system("pause");
//
//	return 0;
//}

struct ImGui_State {
	bool show_demo = false;
	bool show_xor = false;
	bool show_f = false;

	bool exit = false;
};

static ImGui_State imgui_state;

void experiments(ImGui_State& state) {
	ImGui::Begin("Experiments", &state.exit);
	defer { ImGui::End(); };

	state.show_demo |= ImGui::Button("Demo");
	ImGui::Text("Experiments");
	state.show_xor |= ImGui::Button("Xor");
	state.show_f |= ImGui::Button("F");
}

void xor_window(ImGui_State& state) {
	static Xor_Exp exp;
	exp.render(state);
}

void f_window(ImGui_State& state) {
	static F_Exp exp;
	exp.render(state);
}

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**) {
	ImGui_State state;
	
	GLFWwindow* window = nullptr;
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) return 1;
	window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL2 example", NULL, NULL);
	if (window == NULL) return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	ImGui::CreateContext();
	imnodes::Initialize();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		experiments(state);
		if (state.show_demo) ImGui::ShowDemoWindow(&state.show_demo);

		if (state.show_xor) xor_window(state);
		if (state.show_f) f_window(state);

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	imnodes::Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
