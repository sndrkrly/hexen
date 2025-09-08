#include <algorithm>
#include <iostream>
#include <filesystem>

#include <vector>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <AL/al.h>
#include <AL/alc.h>
#define DR_WAV_IMPLEMENTATION
#include "../vendor/dr_wav.h"

static ALCdevice* gDevice = nullptr;
static ALCcontext* gContext = nullptr;

static ALuint gBuffer = 0;
static ALuint gSource = 0;

bool init_openal() {
    gDevice = alcOpenDevice(NULL);
    if (!gDevice) return false;

    gContext = alcCreateContext(gDevice, NULL);
    if (!gContext) { alcCloseDevice(gDevice); return false; }

    alcMakeContextCurrent(gContext);
    return true;
}

void cleanup_openal() {
    alDeleteSources(1, &gSource);
    alDeleteBuffers(1, &gBuffer);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(gContext);

    alcCloseDevice(gDevice);
}

void stop_openal_audio() {
    if (gSource) {
        alSourceStop(gSource);
        alSourcei(gSource, AL_BUFFER, 0);
    }
}

#include "config.h"
#include "window.h"

#ifdef _WIN32 /* windows */
    #include <window.h>

    std::string get_username() {
        char username[256];
        DWORD size = sizeof(username);
        
        if (GetUserNameA(username, &size)) {
            return std::string(username);
        }

        return "unknown";
    }
#else /* linux */
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <cstdlib>

    std::string get_username() {
        if (const char *user_env = getenv("USER")) { 
            return std::string(user_env); 
        }

        struct passwd *pw = getpwuid(getuid());
        if (pw) { 
            return std::string(pw->pw_name); 
        }

        return "unknown";
    }
#endif

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

namespace fs = std::filesystem;
std::string _current_track = "";

void play_audio(const std::string& file) {
    unsigned int channels, sampleRate;
    drwav_uint64 totalPCMFrameCount;
    int16_t* pSampleData = drwav_open_file_and_read_pcm_frames_s16(
        file.c_str(), &channels, &sampleRate, &totalPCMFrameCount, NULL);

    if (!pSampleData) {
        std::cerr << "failed to load WAV: " << file << "\n";
        return;
    }

    if (gSource) alDeleteSources(1, &gSource);
    if (gBuffer) alDeleteBuffers(1, &gBuffer);

    alGenBuffers(1, &gBuffer);
    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(gBuffer, format, pSampleData,
                 (ALsizei)(totalPCMFrameCount * channels * sizeof(int16_t)),
                 sampleRate);
    free(pSampleData);

    alGenSources(1, &gSource);
    alSourcei(gSource, AL_BUFFER, gBuffer);
    alSourcePlay(gSource);
}

bool is_audio_file(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".wav"; 
}

int main () {
    init_openal();

    window _window;
    _window.create("hexen", 800, 600);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetKeyCallback(_window.get_window(), key_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("../res/fonts/inter.ttf", 36.0f);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = WINDOW_BG_COLOR;

    ImGui_ImplGlfw_InitForOpenGL(_window.get_window(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!_window.should_close()) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("hexen", nullptr, window_flags);

             ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 size = ImVec2(200, ImGui::GetContentRegionAvail().y);
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(13, 15, 18, 255));

            ImGui::BeginChild("sidebar", size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
               ImGui::SetWindowFontScale(0.8f);

               ImGui::Dummy(ImVec2(0, 8));
               ImGui::Indent(10.0f);

               if (ImGui::Button("home")) { }
               if (ImGui::Button("favourites")) { }
               if (ImGui::Button("search")) { }

               ImGui::Unindent(10.0f);
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("home", ImVec2(550, 0), false);
                ImGui::Dummy(ImVec2(0, 8));
                ImGui::Indent(10.0f);

                std::string username = get_username();
                ImGui::Text("welcome back, %s", username.c_str());

                ImGui::BeginChild("music browser", ImVec2(550, 0), false);
                    static std::string current_dir = "../music";
                    ImGui::Separator();

                    for (const auto& entry : fs::directory_iterator(current_dir)) {
                        const auto& path = entry.path();
                        std::string name = path.filename().string();

                        if (entry.is_directory()) {
                            if (ImGui::Selectable((name + "/").c_str(), false)) {
                                current_dir = path.string();
                            }
                        } else if (entry.is_regular_file() && is_audio_file(path)) {
                            if (ImGui::Selectable(name.c_str(), false)) {
                                play_audio(path.string());             
                            }

                            if (ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload("AUDIO_FILE", path.string().c_str(), path.string().size() + 1);
                                ImGui::Text("dragging %s", name.c_str());
                                ImGui::EndDragDropSource();
                            }
                        }
                    }
                    
                    ImGui::Dummy(ImVec2(0, 10));
                    if (ImGui::Button("stop")) {
                        stop_openal_audio();
                    }
                ImGui::EndChild();
                ImGui::Unindent(10.0f);
            ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        _window.swap_buffers();
        _window.poll_events();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    cleanup_openal();
    _window.cleanup();

    return 0;
}
