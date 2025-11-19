#include "EditorApplication.h"

void EditorApplication::renderNewSetDialog() {
	if (!show_new_set_dialog) return;

	bool is_unleashed = (current_level && current_level->getGameMode() == LIBGENS_LEVEL_GAME_UNLEASHED);
	ImVec2 window_size = is_unleashed ? ImVec2(400, 220) : ImVec2(400, 150);
	
	ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
	if (ImGui::Begin("New Object Set", &show_new_set_dialog, ImGuiWindowFlags_NoResize)) {
		ImGui::Text("Enter a name for the new object set");
		ImGui::Spacing();
		
		bool enter_pressed = ImGui::InputText("Name", new_set_name, sizeof(new_set_name), ImGuiInputTextFlags_EnterReturnsTrue);
		
		if (is_unleashed) {
			ImGui::InputText("File Name", new_set_filename, sizeof(new_set_filename));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Will be saved as .set.xml in your stage archive (extension added automatically if not present)");
			}
			
			ImGui::Checkbox("IsGameActive", &new_set_is_game_active);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Whether this set is active during gameplay");
			}
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		bool can_create = strlen(new_set_name) > 0;
		
		if (!can_create) {
			ImGui::BeginDisabled();
		}
		
		if ((ImGui::Button("Create", ImVec2(120, 0)) || enter_pressed) && can_create) {
			const char* final_filename = (is_unleashed && strlen(new_set_filename) > 0) ? new_set_filename : new_set_name;
			createNewSet(new_set_name, final_filename, new_set_is_game_active);
			show_new_set_dialog = false;
		}
		
		if (!can_create) {
			ImGui::EndDisabled();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			show_new_set_dialog = false;
		}
	}
	ImGui::End();
}
