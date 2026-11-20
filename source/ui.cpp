// ui.cpp — Dear ImGui version (minimal)
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ui.h>
#include <engine.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <shellapi.h>
#include <cstring>
#include <cstdio>

namespace Ui {
static Screen g_screen = Screen::Main;
static float g_transition = 1.0f;
static const char* ScreenLabel(Screen s) {
    switch(s) {
        case Screen::Main: return "MAIN";
        case Screen::Operations: return "OPERATIONS";
        case Screen::Settings: return "SETTINGS";
        case Screen::Console: return "CONSOLE";
        case Screen::Archive: return "ARCHIVE";
        case Screen::Credits: return "CREDITS";
        case Screen::Login: return "LOGIN";
        case Screen::Profile: return "PROFILE";
        default: return "---";
    }
}
static const char* ScreenSubtitle(Screen s) {
    switch(s) {
        case Screen::Main: return "TACTICAL OPERATIONS COMMAND";
        case Screen::Operations: return "MISSION SELECTION";
        case Screen::Settings: return "SYSTEM CONFIGURATION";
        case Screen::Console: return "EMBEDDED PYTHON RUNTIME";
        case Screen::Archive: return "MISSION LOG / RECORDS";
        case Screen::Credits: return "BUILD INFORMATION";
        default: return "";
    }
}
void Update(float dt) {
    if (g_transition < 1.0f) { g_transition += dt / 0.18f; if (g_transition > 1.0f) g_transition = 1.0f; }
}
bool SidebarContains(float x, float y, int w, int h) { (void)w;(void)h; return x >= 13.0f && x <= 253.0f && y >= 13.0f; }

static void DrawContentArea(Screen screen);

void Render(VkRenderer& renderer, Screen screen, int w, int h, GLFWwindow* window) {
    (void)renderer;
    if (!window) return;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    static bool once = false;
    if (!once) { ImGui::StyleColorsDark(); once = true; }
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(8/255.0f,8/255.0f,8/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(74/255.0f,74/255.0f,74/255.0f,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12,12));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(24/255.0f,24/255.0f,24/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(36/255.0f,36/255.0f,36/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(48/255.0f,48/255.0f,48/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(176/255.0f,176/255.0f,176/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(16/255.0f,16/255.0f,16/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(24/255.0f,24/255.0f,24/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(16/255.0f,16/255.0f,16/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(16/255.0f,16/255.0f,16/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(42/255.0f,42/255.0f,42/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(24/255.0f,24/255.0f,24/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(36/255.0f,36/255.0f,36/255.0f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(48/255.0f,48/255.0f,48/255.0f,1));
    ImGui::Begin("dats_root", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowPos(ImVec2(0,0));
    ImGui::SetWindowSize(ImVec2((float)w,(float)h));
    ImGui::BeginChild("sidebar", ImVec2(240,0), true);
    {
        ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1), "Dynami 20.02");
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), "DYNAMIC ARMS TRAINING SIMULATOR");
        ImGui::Spacing(); ImGui::Separator();
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), "NAVIGATION");
        ImGui::Spacing();
        struct E{const char*l;const char*h;Screen s;} entries[]={{"Main","01",Screen::Main},{"Operations","02",Screen::Operations},{"Settings","03",Screen::Settings},{"Console","04",Screen::Console},{"Archive","05",Screen::Archive},{"Credits","06",Screen::Credits}};
        for(auto&e:entries){
            ImGui::PushID(e.l);
            if(ImGui::Button(e.l,ImVec2(0,0))) g_screen=e.s;
            if(g_screen==e.s){ImGui::SameLine();ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),e.h);}
            ImGui::PopID();
        }
        ImGui::Spacing(); ImGui::Separator();
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), "BUILD 2027.Q1");
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), "ABYSS ENGINE");
        ImGui::TextColored(Engine::IsPythonInitialized()?ImVec4(176/255.0f,176/255.0f,176/255.0f,1):ImVec4(122/255.0f,122/255.0f,122/255.0f,1), Engine::IsPythonInitialized()?"PYTHON: ACTIVE":"PYTHON: FAILED");
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("content", ImVec2(0,0), true);
    {
        ImGui::TextColored(ImVec4(232/255.0f,232/255.0f,232/255.0f,1), ScreenLabel(screen));
        ImGui::SameLine(); ImGui::.SetCursorPosX(400);
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), "ONLINE");
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1), ScreenSubtitle(screen));
        ImGui::Spacing();
        DrawContentArea(screen);
    }
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleColor(14);
    ImGui::PopStyleVar(3);
    if(Engine::IsQuitRequested() && window) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void DrawContentArea(Screen screen) {
    switch(screen) {
        case Screen::Main:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"OPERATIONS");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Select a deployment to begin.");
            break;
        case Screen::Operations:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"DEPLOYMENTS");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Available operations.");
            break;
        case Screen::Settings:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"SETTINGS");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"System configuration.");
            break;
        case Screen::Console:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"PYTHON CONSOLE");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Embedded Python runtime.");
            ImGui::Spacing();
            static char inp[256]={};
            ImGui::InputText("##inp",inp,sizeof(inp),ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Spacing();
            if(ImGui::Button("EXECUTE")){if(Engine::IsPythonInitialized()&&inp[0])Engine::ExecutePythonCommand(inp);inp[0]=0;}
            break;
        case Screen::Archive:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"MISSION LOG");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Recorded operations.");
            ImGui::Spacing();
            struct R{const char*c;const char*n;const char*s;};
            const R rows[]={{"001","Monomo","IN PROGRESS"},{"002","Regeneration","UNLOCKED"},{"003","Alternate","UNLOCKED"},{"004","Annihilation","UNLOCKED"}};
            for(auto&r:rows){ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),r.c);ImGui::SameLine();ImGui::TextColored(ImVec4(176/255.0f,176/255.0f,176/255.0f,1),r.n);ImGui::SameLine();ImGui::SetCursorPosX(400);ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),r.s);ImGui::Separator();}
            break;
        }
        case Screen::Credits: {
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"CREDITS");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Built by Divine.");
            ImGui::Spacing();
            auto row=[](const char*k,const char*v){ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),k);ImGui::SameLine();ImGui::TextColored(ImVec4(176/255.0f,176/255.0f,176/255.0f,1),v);};
            row("Project","Dynami 20.02 - Dynamic Arms Training Simulator");row("Engine","Abyss Engine vQ1.2026.1");row("Rendering","Vulkan 1.0");row("UI","Dear ImGui");row("Scripting","Embedded Python");row("Budget","$0.79 (unspent)");row("Studio","None");
            ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"No flags. No bad guys. Just humans.");
            break;
        }
        case Screen::Login: {
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"LOGIN");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Enter your credentials.");
            ImGui::Spacing();
            static char user[128]={},pass[128]={};
            ImGui::InputText("Username",user,sizeof(user));ImGui::Spacing();
            ImGui::InputText("Password",pass,sizeof(pass));ImGui::Spacing();
            if(ImGui::Button("SIGN IN")){std::string cmd="engine_set_loggedin(\""+std::string(user)+"\",\"user\")";Engine::ExecutePythonCommand(cmd);}
            ImGui::SameLine();
            if(ImGui::Button("REGISTER"))ShellExecuteA(nullptr,"open","http://127.0.0.1/register",nullptr,nullptr,SW_SHOWNORMAL);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Owner account: lightlybyte / DivineAsiegbu123");
            break;
        }
        case Screen::Profile: {
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"PROFILE");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(122/255.0f,122/255.0f,122/255.0f,1),"Your account.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(176/255.0f,176/255.0f,176/255.0f,1),("Username: "+Engine::GetLoggedInUser()).c_str());
            ImGui::TextColored(ImVec4(176/255.0f,176/255.0f,176/255.0f,1),("Role: "+Engine::GetLoggedInRole()).c_str());
            ImGui::Spacing();
            if(ImGui::Button("OPEN PROFILE ON WEBSITE"))ShellExecuteA(nullptr,"open","http://127.0.0.1/profile",nullptr,nullptr,SW_SHOWNORMAL);
            ImGui::SameLine();
            if(ImGui::Button("LOGOUT"))Engine::ExecutePythonCommand("engine_clear_loggedin()");
            break;
        }
        default: break;
    }
}
} // namespace Ui  
