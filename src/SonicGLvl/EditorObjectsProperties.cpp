#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <array>

class SonicGLvlUI {
public:
    bool show_main_menu = true;
    bool show_properties_panel = true;
    bool show_palette_panel = true;
    bool show_bottom_panel = true;
    bool show_stage_info_panel = true;
    bool show_help_panel = true;

    bool show_edit_bool_dialog = false;
    bool show_edit_float_dialog = false;
    bool show_edit_string_dialog = false;
    bool show_edit_vector_dialog = false;
    bool show_edit_vector_list_dialog = false;
    bool show_edit_id_dialog = false;
    bool show_edit_id_list_dialog = false;
    bool show_multiset_param_dialog = false;
    bool show_material_editor = false;
    bool show_material_preview = false;
    bool show_physics_editor = false;
    bool show_find_dialog = false;
    bool show_look_at_point_dialog = false;
    bool show_terrain_info_dialog = false;

    bool show_objects = true;
    bool show_terrain = true;
    bool show_terrain_autodraw = true;
    bool show_mti = true;
    bool show_collision = true;
    bool show_paths = true;
    bool show_ghost = true;

    bool game_shaders = true;
    bool framebuffer_enabled = true;
    bool uv_animations = true;
    bool skybox_enabled = true;

    bool world_transform = false;
    bool local_rotation = false;
    bool placement_snap = false;
    bool rotation_snap = false;

    struct Selection {
        float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f;
        float rot_x = 0.0f, rot_y = 0.0f, rot_z = 0.0f;
    } current_selection;

    int current_object_set = 0;
    bool object_set_visible = true;
    std::vector<std::string> object_sets = { "Default", "Set A", "Set B" };

    float ghost_seek = 0.0f;
    bool ghost_playing = false;

    std::vector<std::pair<std::string, std::string>> properties;
    int selected_property = -1;

    int selected_palette_category = 0;
    std::vector<std::string> palette_categories = { "Objects", "Terrain", "Collision" };
    std::vector<std::string> palette_items;

    struct Material {
        std::string name;
        std::string shader;
        int mesh_slot = 0;
        bool unknown_flag = false;
        std::vector<std::string> texture_units;
    };
    std::vector<Material> materials;
    int selected_material = -1;

    std::string stage_name = "No stage loaded";
    std::string model_open = "N/A";
    std::string skeleton_open = "N/A";
    std::string animation_open = "N/A";

    std::string help_description = "Description lines of current selection. Object's Description will be used if selected on stage or palette. Property description will be used if a property is selected.";

    void Initialize();
    void Render();
    void Shutdown();

private:
    void RenderMainMenuBar();
    void RenderPropertiesPanel();
    void RenderPalettePanel();
    void RenderBottomPanel();
    void RenderStageInfoPanel();
    void RenderHelpPanel();

    void RenderEditBoolDialog();
    void RenderEditFloatDialog();
    void RenderEditStringDialog();
    void RenderEditVectorDialog();
    void RenderEditVectorListDialog();
    void RenderEditIdDialog();
    void RenderEditIdListDialog();
    void RenderMultiSetParamDialog();
    void RenderMaterialEditor();
    void RenderMaterialPreview();
    void RenderPhysicsEditor();
    void RenderFindDialog();
    void RenderLookAtPointDialog();
    void RenderTerrainInfoDialog();
};

static SonicGLvlUI g_UI;

void SonicGLvlUI::Initialize() {
    properties.push_back({ "Position", "0, 0, 0" });
    properties.push_back({ "Rotation", "0, 0, 0" });
    properties.push_back({ "Scale", "1, 1, 1" });

    palette_items.push_back("Ring");
    palette_items.push_back("Spring");
    palette_items.push_back("DashPanel");
}

void SonicGLvlUI::Shutdown() {
}

void SonicGLvlUI::Render() {
    if (show_main_menu) {
        RenderMainMenuBar();
    }

    if (show_properties_panel) {
        RenderPropertiesPanel();
    }

    if (show_palette_panel) {
        RenderPalettePanel();
    }

    if (show_bottom_panel) {
        RenderBottomPanel();
    }

    if (show_stage_info_panel) {
        RenderStageInfoPanel();
    }

    if (show_help_panel) {
        RenderHelpPanel();
    }

    if (show_edit_bool_dialog) RenderEditBoolDialog();
    if (show_edit_float_dialog) RenderEditFloatDialog();
    if (show_edit_string_dialog) RenderEditStringDialog();
    if (show_edit_vector_dialog) RenderEditVectorDialog();
    if (show_edit_vector_list_dialog) RenderEditVectorListDialog();
    if (show_edit_id_dialog) RenderEditIdDialog();
    if (show_edit_id_list_dialog) RenderEditIdListDialog();
    if (show_multiset_param_dialog) RenderMultiSetParamDialog();
    if (show_material_editor) RenderMaterialEditor();
    if (show_material_preview) RenderMaterialPreview();
    if (show_physics_editor) RenderPhysicsEditor();
    if (show_find_dialog) RenderFindDialog();
    if (show_look_at_point_dialog) RenderLookAtPointDialog();
    if (show_terrain_info_dialog) RenderTerrainInfoDialog();
}

void SonicGLvlUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Stage...")) {  }
            if (ImGui::MenuItem("Save Stage Data...")) {  }
            if (ImGui::MenuItem("Save Stage Resources...")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Convert/Fix All Materials (Unleashed)")) {  }
            if (ImGui::MenuItem("Convert/Fix All Materials (Unleashed Shaders)")) {  }
            if (ImGui::MenuItem("Convert/Fix All Materials (Generations)")) {  }
            if (ImGui::MenuItem("Convert/Fix All Materials (Lost World)")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Close")) {  }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {  }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {  }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {  }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {  }
            if (ImGui::MenuItem("Delete", "Del")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Selection", "Ctrl+D")) {  }
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {  }
            ImGui::Separator();

            if (ImGui::BeginMenu("Look at each other (two objects)")) {
                if (ImGui::MenuItem("Use X-Axis As Direction")) {  }
                if (ImGui::MenuItem("Use Y-Axis As Direction")) {  }
                if (ImGui::MenuItem("Use Z-Axis As Direction")) {  }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Look at...")) { show_look_at_point_dialog = true; }
            ImGui::Separator();
            if (ImGui::MenuItem("Snap objects to closest path")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Find", "Ctrl+F")) { show_find_dialog = true; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Show")) {
                ImGui::MenuItem("Objects", "Ctrl+1", &show_objects);
                ImGui::MenuItem("Terrain", "Ctrl+2", &show_terrain);
                ImGui::MenuItem("Terrain Autodraw", "Ctrl+3", &show_terrain_autodraw);
                ImGui::MenuItem("Collision", "Ctrl+4", &show_collision);
                ImGui::MenuItem("Paths", "Ctrl+5", &show_paths);
                ImGui::MenuItem("Ghost", "Ctrl+6", &show_ghost);
                ImGui::MenuItem("MTI Instances", "Ctrl+7", &show_mti);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Graphics")) {
                ImGui::MenuItem("Game Shaders", "F5", &game_shaders);
                ImGui::Separator();
                ImGui::MenuItem("Framebuffer & Depth Buffer (Requires Shaders)", "F6", &framebuffer_enabled);
                ImGui::MenuItem("UV Animations (Requires Shaders)", "F7", &uv_animations);
                ImGui::MenuItem("Skybox", "F8", &skybox_enabled);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::MenuItem("Use World Transform", "Ctrl+E", &world_transform);
            ImGui::MenuItem("Use Local Rotation", nullptr, &local_rotation);
            ImGui::MenuItem("Use Placement Snap", nullptr, &placement_snap);
            ImGui::MenuItem("Use Rotation Snap", nullptr, &rotation_snap);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Terrain")) {
            if (ImGui::MenuItem("Load All Terrain...")) {  }
            if (ImGui::MenuItem("Export Scene as FBX...")) {  }
            if (ImGui::MenuItem("Terrain Info...")) { show_terrain_info_dialog = true; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Objects")) {
            if (ImGui::MenuItem("Reload Object Templates Database...")) {  }
            if (ImGui::MenuItem("Save Object Templates Database...")) {  }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Materials")) {
            if (ImGui::MenuItem("Open Material Editor...")) { show_material_editor = true; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Editor")) {
            if (ImGui::MenuItem("Save Configuration...")) {  }
            if (ImGui::MenuItem("Reload Configuration...")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Launch Game")) {  }
            ImGui::Separator();
            if (ImGui::MenuItem("Connect To Game")) {  }
            ImGui::Separator();

            if (ImGui::BeginMenu("Ghost")) {
                if (ImGui::MenuItem("Start Recording")) {  }
                if (ImGui::MenuItem("Stop Recording")) {  }
                if (ImGui::MenuItem("Load Recording From Game")) {  }
                ImGui::Separator();
                if (ImGui::MenuItem("Load From File")) {  }
                if (ImGui::MenuItem("Save Recording", nullptr, false, false)) {  }
                if (ImGui::MenuItem("Save Recording (FBX)", nullptr, false, false)) {  }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Quick Overview")) {  }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void SonicGLvlUI::RenderPropertiesPanel() {
    ImGui::Begin("Object Properties", &show_properties_panel);

    if (ImGui::BeginTable("Properties", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        for (int i = 0; i < properties.size(); i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", properties[i].first.c_str());
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(properties[i].second.c_str(), selected_property == i, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_property = i;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                show_edit_string_dialog = true;
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void SonicGLvlUI::RenderPalettePanel() {
    ImGui::Begin("Object Palette", &show_palette_panel);

    ImGui::Text("Category:");
    if (ImGui::BeginCombo("##PaletteCategory", palette_categories[selected_palette_category].c_str())) {
        for (int i = 0; i < palette_categories.size(); i++) {
            bool is_selected = (selected_palette_category == i);
            if (ImGui::Selectable(palette_categories[i].c_str(), is_selected)) {
                selected_palette_category = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    if (ImGui::BeginListBox("##PaletteItems", ImVec2(-FLT_MIN, -FLT_MIN))) {
        for (int i = 0; i < palette_items.size(); i++) {
            if (ImGui::Selectable(palette_items[i].c_str())) {
            }
        }
        ImGui::EndListBox();
    }

    ImGui::End();
}

void SonicGLvlUI::RenderBottomPanel() {
    ImGui::Begin("Bottom Panel", &show_bottom_panel, ImGuiWindowFlags_NoTitleBar);

    ImGui::BeginGroup();
    ImGui::Text("Current Object Set");
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("##ObjectSet", object_sets[current_object_set].c_str())) {
        for (int i = 0; i < object_sets.size(); i++) {
            bool is_selected = (current_object_set == i);
            if (ImGui::Selectable(object_sets[i].c_str(), is_selected)) {
                current_object_set = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Visible", &object_set_visible);
    ImGui::EndGroup();

    ImGui::SameLine(200.0f);

    ImGui::BeginGroup();
    ImGui::Text("Current Selection's Transform");
    ImGui::Text("Position:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##PosX", &current_selection.pos_x, 0.1f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##PosY", &current_selection.pos_y, 0.1f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##PosZ", &current_selection.pos_z, 0.1f);

    ImGui::Text("Rotation:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##RotX", &current_selection.rot_x, 0.1f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##RotY", &current_selection.rot_y, 0.1f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##RotZ", &current_selection.rot_z, 0.1f);
    ImGui::EndGroup();

    ImGui::SameLine(600.0f);

    ImGui::BeginGroup();
    ImGui::Text("Ghost Playback Controls");
    ImGui::SliderFloat("##GhostSeek", &ghost_seek, 0.0f, 1.0f, "%.2f");
    if (ImGui::Button("<<")) {  }
    ImGui::SameLine();
    if (ImGui::Button("| |")) { ghost_playing = false; }
    ImGui::SameLine();
    if (ImGui::Button("|>")) { ghost_playing = true; }
    ImGui::SameLine();
    if (ImGui::Button(">>")) {  }
    ImGui::EndGroup();

    ImGui::End();
}

void SonicGLvlUI::RenderStageInfoPanel() {
    ImGui::Begin("Stage Information", &show_stage_info_panel);

    ImGui::Text("Stage: %s", stage_name.c_str());
    ImGui::Text("Current Model Open: %s", model_open.c_str());
    ImGui::Text("Current Skeleton Open: %s", skeleton_open.c_str());
    ImGui::Text("Current Animation Open: %s", animation_open.c_str());

    ImGui::End();
}

void SonicGLvlUI::RenderHelpPanel() {
    ImGui::Begin("Help", &show_help_panel);

    ImGui::TextWrapped("%s", help_description.c_str());

    ImGui::End();
}

void SonicGLvlUI::RenderEditBoolDialog() {
    ImGui::OpenPopup("Change Property Value##Bool");

    if (ImGui::BeginPopupModal("Change Property Value##Bool", &show_edit_bool_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static int bool_value = 0;
        const char* items[] = { "false", "true" };

        ImGui::Text("New Value (true/false)");
        ImGui::Combo("##BoolValue", &bool_value, items, 2);

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_bool_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_bool_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditFloatDialog() {
    ImGui::OpenPopup("Change Property Value##Float");

    if (ImGui::BeginPopupModal("Change Property Value##Float", &show_edit_float_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char float_value[64] = "0.0";

        ImGui::Text("New Value (float)");
        ImGui::InputText("##FloatValue", float_value, 64);

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_float_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_float_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditStringDialog() {
    ImGui::OpenPopup("Change Property Value##String");

    if (ImGui::BeginPopupModal("Change Property Value##String", &show_edit_string_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char string_value[256] = "";

        ImGui::Text("New Value (string)");
        ImGui::InputText("##StringValue", string_value, 256);

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_string_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_string_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditVectorDialog() {
    ImGui::OpenPopup("Change Property Value##Vector");

    if (ImGui::BeginPopupModal("Change Property Value##Vector", &show_edit_vector_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float vec[3] = { 0.0f, 0.0f, 0.0f };
        static bool enable_viewport_editing = false;

        ImGui::Text("New Value (vector / point)");
        ImGui::InputFloat3("##Vector", vec);

        ImGui::Checkbox("Enable editing in viewport", &enable_viewport_editing);
        if (ImGui::Button("Focus on point")) {
        }

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_vector_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_vector_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditVectorListDialog() {
    ImGui::OpenPopup("Change Property Value##VectorList");

    if (ImGui::BeginPopupModal("Change Property Value##VectorList", &show_edit_vector_list_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static std::vector<std::array<float, 3>> vector_list;
        static int selected_vec = -1;
        static float edit_vec[3] = { 0.0f, 0.0f, 0.0f };

        ImGui::Text("Vector List");

        if (ImGui::BeginListBox("##VectorList", ImVec2(300, 150))) {
            for (int i = 0; i < vector_list.size(); i++) {
                char label[64];
                snprintf(label, 64, "%.2f, %.2f, %.2f", vector_list[i][0], vector_list[i][1], vector_list[i][2]);
                if (ImGui::Selectable(label, selected_vec == i)) {
                    selected_vec = i;
                    edit_vec[0] = vector_list[i][0];
                    edit_vec[1] = vector_list[i][1];
                    edit_vec[2] = vector_list[i][2];
                }
            }
            ImGui::EndListBox();
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::Button("Create")) {
            std::array<float, 3> new_vec = { 0.0f, 0.0f, 0.0f };
            vector_list.push_back(new_vec);
        }
        if (ImGui::Button("Delete") && selected_vec >= 0) {
            vector_list.erase(vector_list.begin() + selected_vec);
            selected_vec = -1;
        }
        if (ImGui::Button("Move Up") && selected_vec > 0) {
            std::swap(vector_list[selected_vec], vector_list[selected_vec - 1]);
            selected_vec--;
        }
        if (ImGui::Button("Move Down") && selected_vec >= 0 && selected_vec < vector_list.size() - 1) {
            std::swap(vector_list[selected_vec], vector_list[selected_vec + 1]);
            selected_vec++;
        }
        ImGui::EndGroup();

        ImGui::InputFloat3("##EditVec", edit_vec);
        if (selected_vec >= 0) {
            vector_list[selected_vec][0] = edit_vec[0];
            vector_list[selected_vec][1] = edit_vec[1];
            vector_list[selected_vec][2] = edit_vec[2];
        }

        static bool enable_viewport_editing = false;
        ImGui::Checkbox("Enable editing in viewport", &enable_viewport_editing);
        if (ImGui::Button("Focus on point")) {
        }

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_vector_list_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_vector_list_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditIdDialog() {
    ImGui::OpenPopup("Change Property Value##ID");

    if (ImGui::BeginPopupModal("Change Property Value##ID", &show_edit_id_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char id_value[128] = "";
        static bool select_from_viewport = false;

        ImGui::Text("New Value (Object ID)");
        ImGui::InputText("##IDValue", id_value, 128);
        ImGui::Text("Points to: (none)");

        ImGui::Checkbox("Select From Viewport", &select_from_viewport);
        if (ImGui::Button("Go to Target")) {
        }

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_id_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_id_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderEditIdListDialog() {
    ImGui::OpenPopup("Change Property Value##IDList");

    if (ImGui::BeginPopupModal("Change Property Value##IDList", &show_edit_id_list_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static std::vector<std::string> id_list;
        static int selected_id = -1;
        static char edit_id[128] = "";

        ImGui::Text("Object ID List");

        if (ImGui::BeginListBox("##IDList", ImVec2(300, 150))) {
            for (int i = 0; i < id_list.size(); i++) {
                if (ImGui::Selectable(id_list[i].c_str(), selected_id == i)) {
                    selected_id = i;
                    strncpy(edit_id, id_list[i].c_str(), 128);
                }
            }
            ImGui::EndListBox();
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::Button("Create")) {
            id_list.push_back("NewID");
        }
        if (ImGui::Button("Delete") && selected_id >= 0) {
            id_list.erase(id_list.begin() + selected_id);
            selected_id = -1;
        }
        if (ImGui::Button("Move Up") && selected_id > 0) {
            std::swap(id_list[selected_id], id_list[selected_id - 1]);
            selected_id--;
        }
        if (ImGui::Button("Move Down") && selected_id >= 0 && selected_id < id_list.size() - 1) {
            std::swap(id_list[selected_id], id_list[selected_id + 1]);
            selected_id++;
        }
        ImGui::EndGroup();

        ImGui::InputText("##EditID", edit_id, 128);
        if (selected_id >= 0) {
            id_list[selected_id] = edit_id;
        }
        ImGui::Text("Points to: (none)");

        static bool select_from_viewport = false;
        ImGui::Checkbox("Select from viewport", &select_from_viewport);
        if (ImGui::Button("Go to Target")) {
        }

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_edit_id_list_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_edit_id_list_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderMultiSetParamDialog() {
    ImGui::OpenPopup("Cloning / Instancing Options");

    if (ImGui::BeginPopupModal("Cloning / Instancing Options", &show_multiset_param_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static int method = 0; 
        static float separation[3] = { 0.0f, 0.0f, 0.0f };
        static int count = 1;
        static float spacing = 1.0f;
        static bool snap_path_edges = false;
        static bool snap_path_centers = false;

        ImGui::BeginGroup();
        ImGui::Text("Method");
        ImGui::RadioButton("Cloning", &method, 0);
        ImGui::RadioButton("Instancing", &method, 1);
        ImGui::BeginDisabled();
        ImGui::RadioButton("Instancing (additive)", &method, 2);
        ImGui::EndDisabled();

        ImGui::Text("Options");
        ImGui::BeginDisabled();
        ImGui::Checkbox("Snap to path edges", &snap_path_edges);
        ImGui::Checkbox("Snap to path centers", &snap_path_centers);
        ImGui::EndDisabled();
        ImGui::EndGroup();

        ImGui::SameLine(150.0f);

        ImGui::BeginGroup();
        ImGui::Text("Parameters");
        ImGui::Text("Separation Vector");
        ImGui::Text("X:"); ImGui::SameLine();
        ImGui::InputFloat("##SepX", &separation[0]);
        ImGui::Text("Y:"); ImGui::SameLine();
        ImGui::InputFloat("##SepY", &separation[1]);
        ImGui::Text("Z:"); ImGui::SameLine();
        ImGui::InputFloat("##SepZ", &separation[2]);
        if (ImGui::Button("Get from object")) {
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Count");
        ImGui::InputInt("##Count", &count);
        ImGui::Text("Spacing");
        ImGui::InputFloat("##Spacing", &spacing);
        ImGui::EndGroup();

        ImGui::Separator();
        ImGui::TextWrapped("Cloning will create new copies of the selected object with independent properties.\n\nInstancing will use the game's cloning method. The copies will share their properties with the source object.");

        ImGui::Separator();

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            show_multiset_param_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(120, 0))) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_multiset_param_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderMaterialEditor() {
    ImGui::Begin("Material Editor", &show_material_editor, ImGuiWindowFlags_AlwaysAutoResize);

    static int editing_mode = 0; 

    ImGui::Text("Editing Mode");
    ImGui::RadioButton("Model Mode", &editing_mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Material Mode", &editing_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Terrain Mode", &editing_mode, 2);

    ImGui::Text("Current Model Open: %s", model_open.c_str());
    ImGui::Text("Current Skeleton Open: %s", skeleton_open.c_str());
    ImGui::Text("Current Animation Open: %s", animation_open.c_str());

    if (ImGui::Button("Load Model...")) {  }
    ImGui::SameLine();
    if (ImGui::Button("Save Model...")) {  }
    ImGui::SameLine();
    if (ImGui::Button("Load Skeleton...")) {  }
    ImGui::SameLine();
    if (ImGui::Button("Load Animation...")) {  }

    ImGui::Separator();

    ImGui::BeginGroup();
    ImGui::Text("Material List");
    if (ImGui::BeginListBox("##MaterialList", ImVec2(200, 300))) {
        for (int i = 0; i < materials.size(); i++) {
            if (ImGui::Selectable(materials[i].name.c_str(), selected_material == i)) {
                selected_material = i;
            }
        }
        ImGui::EndListBox();
    }
    if (ImGui::Button("Save")) {  }
    ImGui::SameLine();
    if (ImGui::Button("Save All")) {  }
    ImGui::EndGroup();

    ImGui::SameLine();

    if (selected_material >= 0 && selected_material < materials.size()) {
        ImGui::BeginGroup();
        ImGui::Text("Material Parameters");

        Material& mat = materials[selected_material];
        char name_buf[128];
        strncpy(name_buf, mat.name.c_str(), 128);
        ImGui::InputText("Name", name_buf, 128);
        mat.name = name_buf;

        char shader_buf[128];
        strncpy(shader_buf, mat.shader.c_str(), 128);
        ImGui::InputText("Shader", shader_buf, 128);
        mat.shader = shader_buf;

        ImGui::InputInt("Mesh Slot", &mat.mesh_slot);
        ImGui::Checkbox("Unknown Flag", &mat.unknown_flag);

        ImGui::Separator();
        ImGui::Text("Texture Units");

        ImGui::Separator();

        static bool load_defaults_on_change = true;
        ImGui::Checkbox("Load new defaults when Shader is changed", &load_defaults_on_change);
        if (ImGui::Button("Load Default Parameters for Shader")) {
        }

        ImGui::EndGroup();
    }

    if (ImGui::Button("Close")) {
        show_material_editor = false;
    }

    ImGui::End();
}

void SonicGLvlUI::RenderMaterialPreview() {
    ImGui::Begin("Preview", &show_material_preview);

    ImGui::Text("Material Preview Viewport");

    ImGui::End();
}

void SonicGLvlUI::RenderPhysicsEditor() {
    ImGui::OpenPopup("Physics Editor");

    if (ImGui::BeginPopupModal("Physics Editor", &show_physics_editor, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Collision Files:");

        if (ImGui::BeginListBox("##CollisionList", ImVec2(300, 200))) {
            ImGui::EndListBox();
        }

        if (ImGui::Button("Import...")) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            show_physics_editor = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderFindDialog() {
    ImGui::OpenPopup("Find");

    if (ImGui::BeginPopupModal("Find", &show_find_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char object_name[256] = "";
        static bool match_exactly = false;
        static bool find_all = false;
        static bool with_property = false;
        static char property_name[128] = "";
        static char property_value[128] = "";

        ImGui::Text("Basic Options");
        ImGui::Text("Object Name");
        ImGui::InputText("##ObjectName", object_name, 256);
        ImGui::Checkbox("Find And Select All", &find_all);
        ImGui::Checkbox("Match Exactly", &match_exactly);

        ImGui::Separator();
        ImGui::Text("Filter Options");
        ImGui::Checkbox("With Property And Value", &with_property);
        if (with_property) {
            ImGui::Text("Property");
            ImGui::InputText("##PropertyName", property_name, 128);
            ImGui::Text("Value");
            ImGui::InputText("##PropertyValue", property_value, 128);
        }

        ImGui::Separator();

        if (ImGui::Button("Find Next", ImVec2(120, 0))) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_find_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderLookAtPointDialog() {
    ImGui::OpenPopup("Look At Point");

    if (ImGui::BeginPopupModal("Look At Point", &show_look_at_point_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        static float point[3] = { 0.0f, 0.0f, 0.0f };
        static int axis = 0; 
        static bool enable_viewport = false;
        static bool get_from_object = false;

        ImGui::Text("New point value");
        ImGui::InputFloat3("##Point", point);

        ImGui::Separator();

        ImGui::BeginGroup();
        ImGui::Text("Modify Point");
        ImGui::Checkbox("Enable editing in viewport", &enable_viewport);
        ImGui::Checkbox("Get from object", &get_from_object);
        if (ImGui::Button("Focus on point")) {
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Direction Axis");
        ImGui::RadioButton("X-Axis", &axis, 0);
        ImGui::RadioButton("Y-Axis", &axis, 1);
        ImGui::RadioButton("Z-Axis", &axis, 2);
        ImGui::EndGroup();

        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            show_look_at_point_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_look_at_point_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void SonicGLvlUI::RenderTerrainInfoDialog() {
    ImGui::OpenPopup("Terrain Info");

    if (ImGui::BeginPopupModal("Terrain Info", &show_terrain_info_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Instance Name: N/A");
        ImGui::Text("Group Name: N/A");
        ImGui::Text("Model Name: N/A");
        ImGui::Text("Subset ID: N/A");

        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_terrain_info_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

extern "C" {
    void SonicGLvl_InitUI() {
        g_UI.Initialize();
    }

    void SonicGLvl_RenderUI() {
        g_UI.Render();
    }

    void SonicGLvl_ShutdownUI() {
        g_UI.Shutdown();
    }
}
