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

namespace fs = std::filesystem;
std::string _current_track = "";

static ALCdevice* gDevice = nullptr;
static ALCcontext* gContext = nullptr;

static ALuint gBuffer = 0;
static ALuint gSource = 0;

float gAudioDuration = 0.0f;

bool init_openal() {
    gDevice = alcOpenDevice(NULL);
    if (!gDevice) {
        return false;
    }

    gContext = alcCreateContext(gDevice, NULL);
    if (!gContext) { 
        alcCloseDevice(gDevice); 
        return false; 
    }

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

void stop_audio() {
    if (gSource) {
        alSourceStop(gSource);
        alSourcei(gSource, AL_BUFFER, 0);
        _current_track = "";
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

float get_audio_duration(ALuint buffer) {
    ALint sizeInBytes, channels, bits, frequency;

    alGetBufferi(buffer, AL_SIZE, &sizeInBytes);
    alGetBufferi(buffer, AL_CHANNELS, &channels);
    alGetBufferi(buffer, AL_BITS, &bits);
    alGetBufferi(buffer, AL_FREQUENCY, &frequency);

    // Total samples = total bytes / (bits/8 * channels)
    // Duration = total samples / sample rate
    float duration = static_cast<float>(sizeInBytes) /
                     (channels * (bits / 8)) /
                     frequency;

    return duration;
}

float get_current_time(ALuint source) {
    float seconds;
    alGetSourcef(source, AL_SEC_OFFSET, &seconds);

    return seconds;
}

bool is_audio_file(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".wav"; 
}

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

    gAudioDuration = get_audio_duration(gBuffer);
    _current_track = file;
}

int main () {
    init_openal();

    window _window;
    _window.create("hexen", 800, 600);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

               ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "hexen");

               if (ImGui::Button("home")) { }
               if (ImGui::Button("favourites")) { }
               if (ImGui::Button("search")) { }

               ImGui::Unindent(10.0f);
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("home", ImVec2(550, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                ImGui::Dummy(ImVec2(0, 8));
                ImGui::Indent(10.0f);

                std::string username = get_username();
                ImGui::Text("get back where u left off, %s", username.c_str());

                ImGui::BeginChild("music browser", ImVec2(550, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                    float control_bar_height = 80.0f;      
                    float file_browser_height = ImGui::GetContentRegionAvail().y - control_bar_height;

                    ImGui::BeginChild("file browser", ImVec2(0, file_browser_height), true);
                        static std::string current_dir = "../music/";

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
                    ImGui::EndChild();
                    ImGui::BeginChild("audio controls", ImVec2(0, 0), false);
                        if (ImGui::Button("stop")) { stop_audio(); }

                        float current_time = get_current_time(gSource);
                        float progress = current_time / gAudioDuration;
                        progress = std::clamp(progress, 0.0f, 1.0f);

                        ImVec2 progress_bar_size = ImVec2(ImGui::GetContentRegionAvail().x, 20.0f);
                        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                        
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            cursor_pos,
                            ImVec2(cursor_pos.x + progress_bar_size.x, cursor_pos.y + progress_bar_size.y),
                            IM_COL32(60, 64, 72, 255) 
                        );
                        
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            cursor_pos,
                            ImVec2(cursor_pos.x + progress_bar_size.x * progress, cursor_pos.y + progress_bar_size.y),
                            IM_COL32(200, 200, 200, 255)     
                        );
                        
                        ImGui::InvisibleButton("##progress_bar", progress_bar_size);

                        if (ImGui::IsItemClicked()) {
                            ImVec2 mouse_pos = ImGui::GetMousePos();
                            
                            float click_x = mouse_pos.x - cursor_pos.x;
                            float new_progress = click_x / progress_bar_size.x;
    
                            new_progress = std::clamp(new_progress, 0.0f, 1.0f);
                            
                            float new_time = new_progress * gAudioDuration;
                            alSourcef(gSource, AL_SEC_OFFSET, new_time);
                        }
                    ImGui::EndChild();
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
