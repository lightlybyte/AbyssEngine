// ---------------------------------------------------------------------------
// ui.cpp — the whole DATS menu system in one file.
//
// Cold clinical greyscale. Split layout: fixed 240px sidebar + flexible
// content pane. One 180ms crossfade between screens. No rounding. No hue.
// ---------------------------------------------------------------------------

#include <ui.h>
#include <engine.h>

#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdio>
#include <cmath>

// Nuklear first — one place, one set of defines.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

namespace Ui {

// ---------------------------------------------------------------------------
// Palette — eight neutral steps, no hue
// ---------------------------------------------------------------------------
namespace G {
    constexpr nk_color bg        {  8,   8,   8, 255 };
    constexpr nk_color panel     { 16,  16,  16, 255 };
    constexpr nk_color row       { 24,  24,  24, 255 };
    constexpr nk_color row_hover { 36,  36,  36, 255 };
    constexpr nk_color row_active{ 48,  48,  48, 255 };
    constexpr nk_color rule      { 42,  42,  42, 255 };
    constexpr nk_color border    { 74,  74,  74, 255 };
    constexpr nk_color dim       {122, 122, 122, 255 };
    constexpr nk_color text      {176, 176, 176, 255 };
    constexpr nk_color bright    {232, 232, 232, 255 };
    constexpr nk_color white     {248, 248, 248, 255 };
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------
namespace D {
    constexpr float frame_inset   = 12.0f;
    constexpr float frame_border  = 1.0f;
    constexpr float bracket_len   = 14.0f;
    constexpr float bracket_thick = 2.0f;

    constexpr float sidebar_w     = 240.0f;
    constexpr float sidebar_pad_x = 24.0f;
    constexpr float sidebar_pad_y = 32.0f;

    constexpr float content_pad_x = 48.0f;
    constexpr float content_pad_y = 40.0f;

    constexpr float text_small    = 1.0f;
    constexpr float text_head     = 2.0f;
    constexpr float text_hero     = 3.0f;

    constexpr float nav_row_h     = 44.0f;
    constexpr float action_row_h  = 56.0f;
    constexpr float row_gap       = 4.0f;

    constexpr float transition_s  = 0.18f;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static Screen g_screen     = Screen::Main;
static float  g_transition = 1.0f;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static const char* ScreenLabel(Screen s) {
    switch (s) {
        case Screen::Main:       return "MAIN";
        case Screen::Operations: return "OPERATIONS";
        case Screen::Settings:   return "SETTINGS";
        case Screen::Console:    return "CONSOLE";
        case Screen::Archive:    return "ARCHIVE";
        case Screen::Credits:    return "CREDITS";
        default:                 return "—";
    }
}

static const char* ScreenSubtitle(Screen s) {
    switch (s) {
        case Screen::Main:       return "TACTICAL OPERATIONS COMMAND";
        case Screen::Operations: return "MISSION SELECTION";
        case Screen::Settings:   return "SYSTEM CONFIGURATION";
        case Screen::Console:    return "EMBEDDED PYTHON RUNTIME";
        case Screen::Archive:    return "MISSION LOG / RECORDS";
        case Screen::Credits:    return "BUILD INFORMATION";
        default:                 return "";
    }
}

static void Fill(nk_context* ctx, nk_rect r, nk_color c) {
    nk_fill_rect(nk_window_get_canvas(ctx), r, 0.0f, c);
}

static void Rule(nk_context* ctx, nk_color c) {
    nk_layout_row_dynamic(ctx, 1.0f, 1);
    nk_rect b = nk_widget_bounds(ctx);
    nk_fill_rect(nk_window_get_canvas(ctx), b, 0.0f, c);
}

static void Label(nk_context* ctx, const char* text, float scale, nk_color c) {
    nk_layout_row_dynamic(ctx, 13.0f * scale, 1);
    nk_rect b = nk_widget_bounds(ctx);
    nk_draw_text(nk_window_get_canvas(ctx), b, text,
                 (int)std::strlen(text), ctx->style.font,
                 nk_rgba(0,0,0,0), c);
}

static void Space(nk_context* ctx, float h) {
    nk_layout_row_dynamic(ctx, h, 1);
    nk_spacing(ctx, 1);
}

static bool NavRow(nk_context* ctx, const char* label, const char* hint,
                   bool selected, float height) {
    nk_layout_row_dynamic(ctx, height, 1);
    nk_rect bounds = nk_widget_bounds(ctx);

    nk_color bg = selected
        ? G::row_active
        : (nk_input_is_mouse_hovering_rect(&ctx->input, bounds)
            ? G::row_hover : G::panel);
    Fill(ctx, bounds, bg);

    if (selected) {
        nk_rect bar = bounds;
        bar.w = 2.0f;
        Fill(ctx, bar, G::white);
    }

    const nk_user_font* font = ctx->style.font;

    nk_rect label_r = bounds;
    label_r.x += 16.0f;
    label_r.w -= 16.0f;
    nk_draw_text(nk_window_get_canvas(ctx), label_r, label,
                 (int)std::strlen(label), font, nk_rgba(0,0,0,0),
                 selected ? G::white : G::text);

    if (hint && hint[0]) {
        float hintw = font->width(font->userdata, font->height, hint,
                                  (int)std::strlen(hint));
        nk_rect hint_r = bounds;
        hint_r.x = bounds.x + bounds.w - hintw - 16.0f;
        hint_r.w = hintw;
        nk_draw_text(nk_window_get_canvas(ctx), hint_r, hint,
                     (int)std::strlen(hint), font, nk_rgba(0,0,0,0),
                     selected ? G::bright : G::dim);
    }

    return nk_input_is_mouse_released(&ctx->input, NK_BUTTON_LEFT)
        && nk_input_is_mouse_hovering_rect(&ctx->input, bounds);
}

static bool ActionRow(nk_context* ctx, const char* label, const char* hint,
                      float height) {
    nk_layout_row_dynamic(ctx, height, 1);
    nk_rect bounds = nk_widget_bounds(ctx);
    bool hover = nk_input_is_mouse_hovering_rect(&ctx->input, bounds);

    Fill(ctx, bounds, hover ? G::row_hover : G::row);

    nk_rect top = bounds;
    top.h = 1.0f;
    Fill(ctx, top, hover ? G::bright : G::rule);

    const nk_user_font* font = ctx->style.font;

    nk_rect label_r = bounds;
    label_r.x += 16.0f;
    label_r.w -= 16.0f;
    nk_draw_text(nk_window_get_canvas(ctx), label_r, label,
                 (int)std::strlen(label), font, nk_rgba(0,0,0,0),
                 hover ? G::white : G::text);

    if (hint && hint[0]) {
        float hintw = font->width(font->userdata, font->height, hint,
                                  (int)std::strlen(hint));
        nk_rect hint_r = bounds;
        hint_r.x = bounds.x + bounds.w - hintw - 16.0f;
        hint_r.w = hintw;
        nk_draw_text(nk_window_get_canvas(ctx), hint_r, hint,
                     (int)std::strlen(hint), font, nk_rgba(0,0,0,0), G::dim);
    }

    return nk_input_is_mouse_released(&ctx->input, NK_BUTTON_LEFT)
        && nk_input_is_mouse_hovering_rect(&ctx->input, bounds);
}

static void TransitionOverlay(nk_context* ctx, float t) {
    if (t >= 1.0f) return;
    if (t < 0.0f) t = 0.0f;
    float s = t * t * (3.0f - 2.0f * t);
    int alpha = (int)((1.0f - s) * 255.0f);
    if (alpha <= 0) return;
    nk_rect content = nk_window_get_content_region(ctx);
    nk_fill_rect(nk_window_get_canvas(ctx), content, 0.0f,
                 nk_rgba(8, 8, 8, (nk_byte)alpha));
}

static void DrawBrackets(nk_context* ctx, nk_rect frame, nk_color c) {
    struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
    const float L = D::bracket_len;
    const float T = D::bracket_thick;

    nk_stroke_line(canvas, frame.x, frame.y + L, frame.x, frame.y, T, c);
    nk_stroke_line(canvas, frame.x, frame.y, frame.x + L, frame.y, T, c);

    nk_stroke_line(canvas, frame.x + frame.w - L, frame.y,
                   frame.x + frame.w, frame.y, T, c);
    nk_stroke_line(canvas, frame.x + frame.w, frame.y,
                   frame.x + frame.w, frame.y + L, T, c);

    nk_stroke_line(canvas, frame.x, frame.y + frame.h - L,
                   frame.x, frame.y + frame.h, T, c);
    nk_stroke_line(canvas, frame.x, frame.y + frame.h,
                   frame.x + L, frame.y + frame.h, T, c);

    nk_stroke_line(canvas, frame.x + frame.w - L, frame.y + frame.h,
                   frame.x + frame.w, frame.y + frame.h, T, c);
    nk_stroke_line(canvas, frame.x + frame.w, frame.y + frame.h - L,
                   frame.x + frame.w, frame.y + frame.h, T, c);
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------
static bool DrawSidebar(nk_context* ctx, Screen current, Screen* out_next) {
    bool changed = false;

    Space(ctx, 8.0f);
    Label(ctx, "DATS", D::text_hero, G::white);
    Label(ctx, "DYNAMIC ARMS TRAINING SIMULATOR", D::text_small, G::dim);
    Space(ctx, 20.0f);
    Rule(ctx, G::rule);
    Space(ctx, 20.0f);
    Label(ctx, "NAVIGATION", D::text_small, G::dim);
    Space(ctx, 8.0f);

    struct NavEntry { const char* label; const char* hint; Screen id; };
    const NavEntry entries[] = {
        { "Main",       "01", Screen::Main       },
        { "Operations", "02", Screen::Operations },
        { "Settings",   "03", Screen::Settings   },
        { "Console",    "04", Screen::Console    },
        { "Archive",    "05", Screen::Archive    },
        { "Credits",    "06", Screen::Credits    },
    };
    for (const auto& e : entries) {
        if (NavRow(ctx, e.label, e.hint, current == e.id, D::nav_row_h)) {
            *out_next = e.id;
            changed = true;
        }
        Space(ctx, D::row_gap);
    }

    // Push version block down. If content is short we pad with space; if
    // it's long this is a no-op because the layout cursor is already low.
    nk_rect content = nk_window_get_content_region(ctx);
    if (content.h > 500.0f) Space(ctx, 60.0f);

    Rule(ctx, G::rule);
    Space(ctx, 12.0f);
    Label(ctx, "BUILD 2027.Q1", D::text_small, G::dim);
    Label(ctx, "ABYSS ENGINE", D::text_small, G::dim);
    Space(ctx, 6.0f);
    Label(ctx,
          Engine::IsPythonInitialized() ? "PYTHON: ACTIVE" : "PYTHON: FAILED",
          D::text_small,
          Engine::IsPythonInitialized() ? G::text : G::dim);

    return changed;
}

// ---------------------------------------------------------------------------
// Content header
// ---------------------------------------------------------------------------
static void DrawContentHeader(nk_context* ctx, Screen screen) {
    nk_layout_row_dynamic(ctx, 16.0f, 1);
    nk_rect b = nk_widget_bounds(ctx);
    const nk_user_font* font = ctx->style.font;

    const char* label = ScreenLabel(screen);
    nk_draw_text(nk_window_get_canvas(ctx), b, label,
                 (int)std::strlen(label), font, nk_rgba(0,0,0,0), G::bright);

    const char* status = "ONLINE";
    float sw = font->width(font->userdata, font->height, status,
                           (int)std::strlen(status));
    nk_rect sr = b;
    sr.x = b.x + b.w - sw;
    sr.w = sw;
    nk_draw_text(nk_window_get_canvas(ctx), sr, status,
                 (int)std::strlen(status), font, nk_rgba(0,0,0,0), G::dim);

    Space(ctx, 6.0f);
    Rule(ctx, G::rule);
    Space(ctx, 4.0f);
    Label(ctx, ScreenSubtitle(screen), D::text_small, G::dim);
    Space(ctx, 20.0f);
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
static Screen Screen_Main(nk_context* ctx) {
    Label(ctx, "OPERATIONS", D::text_head, G::white);
    Space(ctx, 8.0f);
    Label(ctx, "Select a deployment to begin.", D::text_small, G::dim);
    Space(ctx, 28.0f);

    Screen next = Screen::Main;

    if (ActionRow(ctx, "DEPLOY", "ENTER", D::action_row_h))
        Engine::SetState(GameState::PLAYING);
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "QUICK MATCH", "Q", D::action_row_h))
        Engine::SetState(GameState::PLAYING);
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "TRAINING", "T", D::action_row_h))
        Engine::SetState(GameState::PLAYING);
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "SETTINGS", "S", D::action_row_h))
        next = Screen::Settings;
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "QUIT", "ESC", D::action_row_h))
        Engine::RequestQuit();

    Space(ctx, 32.0f);
    Rule(ctx, G::rule);
    Space(ctx, 16.0f);
    Label(ctx, "No flags. No bad guys. Just humans.", D::text_small, G::dim);

    return next;
}

static Screen Screen_Operations(nk_context* ctx) {
    Label(ctx, "MISSION SELECTION", D::text_head, G::white);
    Space(ctx, 8.0f);
    Label(ctx, "Choose an operation.", D::text_small, G::dim);
    Space(ctx, 28.0f);

    struct Op { const char* name; const char* status; };
    const Op ops[] = {
        { "Monomo",       "AVAILABLE" },
        { "Regeneration", "LOCKED"    },
        { "Alternate",    "LOCKED"    },
        { "Annihilation", "LOCKED"    },
    };
    for (const auto& op : ops) {
        if (ActionRow(ctx, op.name, op.status, D::action_row_h)) {
            if (std::strcmp(op.status, "AVAILABLE") == 0)
                Engine::SetState(GameState::PLAYING);
        }
        Space(ctx, D::row_gap);
    }
    return Screen::Operations;
}

static Screen Screen_Settings(nk_context* ctx, VkRenderer& renderer) {
    Label(ctx, "SYSTEM CONFIGURATION", D::text_head, G::white);
    Space(ctx, 8.0f);
    Label(ctx, "Renderer and runtime options.", D::text_small, G::dim);
    Space(ctx, 28.0f);

    char vs[64];
    std::snprintf(vs, sizeof(vs), "VSync: %s",
                  Engine::GetSettings().vsync ? "ON" : "OFF");
    if (ActionRow(ctx, vs, "TOGGLE", D::action_row_h)) {
        Engine::GetSettings().vsync = !Engine::GetSettings().vsync;
        Engine::ApplyVSync(renderer);
        Engine::SaveSettings("settings.json");
    }
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "Fullscreen", "F11", D::action_row_h)) { /* stub */ }
    Space(ctx, D::row_gap);

    if (ActionRow(ctx, "Reset to defaults", "-", D::action_row_h)) {
        Engine::GetSettings().vsync = true;
        Engine::ApplyVSync(renderer);
        Engine::SaveSettings("settings.json");
    }

    Space(ctx, 32.0f);
    Rule(ctx, G::rule);
    Space(ctx, 16.0f);
    Label(ctx, "Vulkan renderer: ACTIVE", D::text_small, G::dim);
    Label(ctx, "Nuklear UI: ACTIVE", D::text_small, G::dim);
    Label(ctx, "Python runtime: LOADED", D::text_small, G::dim);

    return Screen::Settings;
}

static char s_pyInput[512] = {0};
static Screen Screen_Console(nk_context* ctx) {
    Label(ctx, "EMBEDDED PYTHON RUNTIME", D::text_head, G::white);
    Space(ctx, 8.0f);

    char status[128];
    std::snprintf(status, sizeof(status), "Status: %s",
                  Engine::IsPythonInitialized() ? "INTERPRETER ACTIVE"
                                                : "INTERPRETER FAILED");
    Label(ctx, status, D::text_small,
          Engine::IsPythonInitialized() ? G::text : G::dim);
    Space(ctx, 20.0f);

    nk_layout_row_dynamic(ctx, 200.0f, 1);
    nk_rect out = nk_widget_bounds(ctx);
    Fill(ctx, out, G::panel);
    nk_stroke_rect(nk_window_get_canvas(ctx), out, 0.0f, 1.0f, G::rule);

    const nk_user_font* font = ctx->style.font;
    nk_rect line = out;
    line.x += 12.0f;
    line.y += 12.0f;
    line.w -= 24.0f;
    line.h = 13.0f;
    auto pl = [&](const char* s, nk_color c) {
        nk_draw_text(nk_window_get_canvas(ctx), line, s,
                     (int)std::strlen(s), font, nk_rgba(0,0,0,0), c);
        line.y += 16.0f;
    };
    pl("DATS Python bridge online.", G::text);
    pl("> import sys", G::dim);
    pl("> print('DATS v1.0')", G::dim);
    pl("DATS v1.0", G::text);
    pl("> ", G::dim);

    Space(ctx, 16.0f);
    nk_layout_row_dynamic(ctx, 36.0f, 1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, s_pyInput,
                                   sizeof(s_pyInput), nk_filter_default);
    Space(ctx, 12.0f);

    if (ActionRow(ctx, "EXECUTE", "ENTER", D::action_row_h)) {
        if (Engine::IsPythonInitialized() && s_pyInput[0] != '\0')
            Engine::ExecutePythonCommand(s_pyInput);
        s_pyInput[0] = '\0';
    }
    return Screen::Console;
}

static Screen Screen_Archive(nk_context* ctx) {
    Label(ctx, "MISSION LOG", D::text_head, G::white);
    Space(ctx, 8.0f);
    Label(ctx, "Recorded operations.", D::text_small, G::dim);
    Space(ctx, 28.0f);

    struct Row { const char* code; const char* name; const char* status; };
    const Row rows[] = {
        { "001", "Monomo",       "IN PROGRESS" },
        { "002", "Regeneration", "UNLOCKED"    },
        { "003", "Alternate",    "UNLOCKED"    },
        { "004", "Annihilation", "UNLOCKED"    },
    };
    for (const auto& r : rows) {
        nk_layout_row_dynamic(ctx, 40.0f, 1);
        nk_rect b = nk_widget_bounds(ctx);
        const nk_user_font* font = ctx->style.font;
        nk_color c = std::strcmp(r.status, "IN PROGRESS") == 0
                     ? G::bright : G::text;

        nk_draw_text(nk_window_get_canvas(ctx), b, r.code,
                     (int)std::strlen(r.code), font, nk_rgba(0,0,0,0), G::dim);

        nk_rect nr = b;
        nr.x += 60.0f;
        nk_draw_text(nk_window_get_canvas(ctx), nr, r.name,
                     (int)std::strlen(r.name), font, nk_rgba(0,0,0,0), c);

        float sw = font->width(font->userdata, font->height, r.status,
                               (int)std::strlen(r.status));
        nk_rect sr = b;
        sr.x = b.x + b.w - sw;
        sr.w = sw;
        nk_draw_text(nk_window_get_canvas(ctx), sr, r.status,
                     (int)std::strlen(r.status), font, nk_rgba(0,0,0,0), G::dim);

        nk_rect rr = b;
        rr.y = b.y + b.h - 1.0f;
        rr.h = 1.0f;
        Fill(ctx, rr, G::rule);
    }
    return Screen::Archive;
}

static Screen Screen_Credits(nk_context* ctx) {
    Label(ctx, "CREDITS", D::text_head, G::white);
    Space(ctx, 8.0f);
    Label(ctx, "Built by Divine.", D::text_small, G::dim);
    Space(ctx, 32.0f);

    auto row = [&](const char* k, const char* v) {
        nk_layout_row_dynamic(ctx, 22.0f, 1);
        nk_rect b = nk_widget_bounds(ctx);
        const nk_user_font* font = ctx->style.font;
        nk_draw_text(nk_window_get_canvas(ctx), b, k,
                     (int)std::strlen(k), font, nk_rgba(0,0,0,0), G::dim);
        nk_rect vr = b;
        vr.x += 200.0f;
        nk_draw_text(nk_window_get_canvas(ctx), vr, v,
                     (int)std::strlen(v), font, nk_rgba(0,0,0,0), G::text);
    };
    row("Project",   "DATS - Dynamic Arms Training Simulator");
    row("Engine",    "Abyss Engine vQ1.2026.1");
    row("Rendering", "Vulkan 1.3");
    row("UI",        "Nuklear");
    row("Scripting", "Embedded Python");
    row("Budget",    "$0.79 (unspent)");
    row("Studio",    "None");

    Space(ctx, 40.0f);
    Rule(ctx, G::rule);
    Space(ctx, 16.0f);
    Label(ctx, "No flags. No bad guys. Just humans.", D::text_small, G::dim);

    return Screen::Credits;
}

// ---------------------------------------------------------------------------
// Style setup
// ---------------------------------------------------------------------------
void ApplyStyle(nk_context* ctx) {
    struct nk_color table[NK_COLOR_COUNT];
    for (int i = 0; i < NK_COLOR_COUNT; ++i) table[i] = G::text;

    table[NK_COLOR_TEXT]                    = G::text;
    table[NK_COLOR_WINDOW]                  = G::panel;
    table[NK_COLOR_HEADER]                  = G::panel;
    table[NK_COLOR_BORDER]                  = G::border;
    table[NK_COLOR_BUTTON]                  = G::row;
    table[NK_COLOR_BUTTON_HOVER]            = G::row_hover;
    table[NK_COLOR_BUTTON_ACTIVE]           = G::row_active;
    table[NK_COLOR_TOGGLE]                  = G::row;
    table[NK_COLOR_TOGGLE_HOVER]            = G::row_hover;
    table[NK_COLOR_TOGGLE_CURSOR]           = G::bright;
    table[NK_COLOR_SELECT]                  = G::row_active;
    table[NK_COLOR_SELECT_ACTIVE]           = G::row_hover;
    table[NK_COLOR_SLIDER]                  = G::row;
    table[NK_COLOR_SLIDER_CURSOR]           = G::bright;
    table[NK_COLOR_SLIDER_CURSOR_HOVER]     = G::white;
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE]    = G::white;
    table[NK_COLOR_PROPERTY]                = G::row;
    table[NK_COLOR_EDIT]                    = G::row;
    table[NK_COLOR_EDIT_CURSOR]             = G::white;
    table[NK_COLOR_COMBO]                   = G::row;
    table[NK_COLOR_CHART]                   = G::panel;
    table[NK_COLOR_CHART_COLOR]             = G::bright;
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT]   = G::white;
    table[NK_COLOR_SCROLLBAR]               = G::panel;
    table[NK_COLOR_SCROLLBAR_CURSOR]        = G::border;
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = G::dim;
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = G::text;
    table[NK_COLOR_TAB_HEADER]              = G::panel;

    nk_style_from_table(ctx, table);

    ctx->style.window.padding        = nk_vec2(0, 0);
    ctx->style.window.spacing        = nk_vec2(0, 0);
    ctx->style.window.border         = 0.0f;
    ctx->style.window.rounding       = 0.0f;
    ctx->style.window.scrollbar_size = nk_vec2(0, 0);

    ctx->style.button.padding        = nk_vec2(0, 0);
    ctx->style.button.rounding       = 0.0f;
    ctx->style.button.border         = 0.0f;
    ctx->style.button.text_normal    = G::text;
    ctx->style.button.text_hover     = G::bright;
    ctx->style.button.text_active    = G::white;

    ctx->style.edit.padding          = nk_vec2(8, 6);
    ctx->style.edit.border           = 1.0f;
    ctx->style.edit.rounding         = 0.0f;

    ctx->style.text.padding          = nk_vec2(0, 0);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void Update(float dt) {
    if (g_transition < 1.0f) {
        g_transition += dt / D::transition_s;
        if (g_transition > 1.0f) g_transition = 1.0f;
    }
}

bool SidebarContains(float x, float y, int w, int h) {
    (void)w; (void)h;
    const float inset = D::frame_inset + D::frame_border;
    return x >= inset && x <= inset + D::sidebar_w && y >= inset;
}

void Render(nk_context* ctx, VkRenderer& renderer, Screen screen,
            int w, int h, GLFWwindow* window) {
    if (!ctx) return;

    nk_rect full = nk_rect(0, 0, (float)w, (float)h);
    if (!nk_begin(ctx, "dats_root", full,
                  NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
        nk_end(ctx);
        return;
    }

    // Background
    nk_rect content = nk_window_get_content_region(ctx);
    Fill(ctx, content, G::bg);

    // Frame
    const float inset = D::frame_inset;
    nk_rect frame = nk_rect(inset, inset,
                            (float)w - inset * 2.0f,
                            (float)h - inset * 2.0f);
    nk_stroke_rect(nk_window_get_canvas(ctx), frame, 0.0f,
                   D::frame_border, G::border);
    DrawBrackets(ctx, frame, G::bright);

    // Sidebar fill
    nk_rect sidebar = frame;
    sidebar.w = D::sidebar_w;
    Fill(ctx, sidebar, G::panel);

    // Content pane fill
    nk_rect pane = frame;
    pane.x += D::sidebar_w;
    pane.w -= D::sidebar_w;
    Fill(ctx, pane, G::panel);

    // Divider
    nk_rect divider = frame;
    divider.x += D::sidebar_w;
    divider.w = 1.0f;
    Fill(ctx, divider, G::rule);

    // Interior rects
    nk_rect sideInner = sidebar;
    sideInner.x += D::sidebar_pad_x;
    sideInner.y += D::sidebar_pad_y;
    sideInner.w -= D::sidebar_pad_x * 2.0f;
    sideInner.h -= D::sidebar_pad_y * 2.0f;

    nk_rect contInner = pane;
    contInner.x += D::content_pad_x;
    contInner.y += D::content_pad_y;
    contInner.w -= D::content_pad_x * 2.0f;
    contInner.h -= D::content_pad_y * 2.0f;

    Screen next = screen;

    // Content child window
    nk_style_push_color(ctx, &ctx->style.window.background, G::panel);
    if (nk_begin(ctx, "dats_content", contInner, NK_WINDOW_NO_SCROLLBAR)) {
        DrawContentHeader(ctx, screen);

        Screen req = screen;
        switch (screen) {
            case Screen::Main:       req = Screen_Main(ctx); break;
            case Screen::Operations: req = Screen_Operations(ctx); break;
            case Screen::Settings:   req = Screen_Settings(ctx, renderer); break;
            case Screen::Console:    req = Screen_Console(ctx); break;
            case Screen::Archive:    req = Screen_Archive(ctx); break;
            case Screen::Credits:    req = Screen_Credits(ctx); break;
            default: break;
        }
        if (req != screen) next = req;

        // Footer
        if (contInner.h > 400.0f) Space(ctx, 60.0f);
        Rule(ctx, G::rule);
        Space(ctx, 10.0f);
        nk_layout_row_dynamic(ctx, 16.0f, 1);
        nk_rect fb = nk_widget_bounds(ctx);
        const nk_user_font* font = ctx->style.font;
        nk_draw_text(nk_window_get_canvas(ctx), fb,
                     "DATS / ABYSS ENGINE",
                     (int)std::strlen("DATS / ABYSS ENGINE"),
                     font, nk_rgba(0,0,0,0), G::dim);
        const char* right = "BUILD 2027.Q1";
        float rw = font->width(font->userdata, font->height, right,
                               (int)std::strlen(right));
        nk_rect rr = fb;
        rr.x = fb.x + fb.w - rw;
        rr.w = rw;
        nk_draw_text(nk_window_get_canvas(ctx), rr, right,
                     (int)std::strlen(right), font, nk_rgba(0,0,0,0), G::dim);
    }
    nk_end(ctx);
    nk_style_pop_color(ctx);

    // Sidebar child window
    nk_style_push_color(ctx, &ctx->style.window.background, G::panel);
    if (nk_begin(ctx, "dats_sidebar", sideInner, NK_WINDOW_NO_SCROLLBAR)) {
        Screen requested = screen;
        if (DrawSidebar(ctx, screen, &requested)) next = requested;
    }
    nk_end(ctx);
    nk_style_pop_color(ctx);

    // Crossfade
    TransitionOverlay(ctx, g_transition);

    // Route screen change
    if (next != screen) {
        g_screen     = next;
        g_transition = 0.0f;
    }

    // Quit request handling
    if (Engine::IsQuitRequested() && window) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    nk_end(ctx);
}

} // namespace Ui