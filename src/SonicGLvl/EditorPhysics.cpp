//=========================================================================
//	  Copyright (c) 2016 SonicGLvl
//
//    This file is part of SonicGLvl, a community-created free level editor 
//    for the PC version of Sonic Generations.
//
//    SonicGLvl is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    SonicGLvl is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    
//
//    Read AUTHORS.txt, LICENSE.txt and COPYRIGHT.txt for more details.
//=========================================================================

#include "EditorApplication.h"

void EditorApplication::openPhysicsEditorGUI() {
	if (!current_level) return;
	if (!current_level->getLevel()) return;
	show_physics_editor = true;
}

void EditorApplication::addPhysicsEditorEntryGUI(LibGens::LevelCollisionEntry *entry) {
	// No-op in ImGui path; rendering happens in renderPhysicsEditor()
}

void EditorApplication::importPhysicsEditorGUI() {
	char *filename = (char *) malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
    memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize     = sizeof(ofn);
	ofn.lpstrFilter     = "Havok Physics File(*.phy.hkx)\0*.phy.hkx\0";
	ofn.nFilterIndex    = 1;
	ofn.lpstrFile       = filename;
    ofn.nMaxFile        = 1024;
    ofn.lpstrTitle      = "Choose the Havok Physics File";
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                          OFN_LONGNAMES     | OFN_EXPLORER |
                          OFN_HIDEREADONLY  | OFN_ENABLESIZING;

    if(GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());
		
		// Check if Havok Entry is on level
		LibGens::Level *level = current_level->getLevel();
		string entry_name = LibGens::File::nameFromFilenameNoExtension(ToString(filename));
		LibGens::LevelCollisionEntry *entry = level->getCollisionEntry(entry_name);

		// If entry exists, overwrite the file and reload the collision
		if (entry) {
			// Copy the file to the current data cache folder
			string data_cache_folder = current_level->getDataCacheFolder();
			LibGens::File collision_file(ToString(filename), LIBGENS_FILE_READ_BINARY);

			if (collision_file.valid()) {
				collision_file.clone(data_cache_folder + "/" + entry_name + LIBGENS_HAVOK_PHYSICS_EXTENSION);

				// Erase current loaded file from Havok Cache
				havok_enviroment->deletePhysicsEntry(entry_name);

				// Load Physics Entry into Cache
				LibGens::HavokPhysicsCache *physics_cache = havok_enviroment->getPhysics(entry_name);
				detectAndTagHavokPhysics(physics_cache);

				// Reload collision
				current_level->cleanCollision(havok_nodes_list);
				current_level->loadCollision(havok_enviroment, scene_manager, havok_nodes_list);
			}
		}
		// If entry doesn't exist, add to list and load the new collision
		else {
		}
	}

	chdir(exe_path.c_str());
    free(filename);
}

void EditorApplication::detectAndTagHavokPhysics(LibGens::HavokPhysicsCache *physics_cache) {
	hkpPhysicsData *physics_data = physics_cache->getPhysics();

	if (physics_data) {
		const hkArray<hkpPhysicsSystem*> &systems = physics_data->getPhysicsSystems();
		for (int i = 0; i < systems.getSize(); i++) {
			const hkArray<hkpRigidBody*> &rigidbodies = systems[i]->getRigidBodies();
			for (int j = 0; j < rigidbodies.getSize(); j++) {
				havok_property_database->applyProperties(rigidbodies[j]);
			}
		}
	}

	physics_cache->save();
}

void EditorApplication::clearPhysicsEditorGUI() {
	show_physics_editor = false;
}
void EditorApplication::renderPhysicsEditor() {
	if (!show_physics_editor) return;
	if (!current_level || !current_level->getLevel()) return;

	if (ImGui::Begin("Physics Editor", &show_physics_editor, ImGuiWindowFlags_NoResize)) {
		if (ImGui::Button("Import Havok Physics")) {
			importPhysicsEditorGUI();
		}
		ImGui::Separator();

		if (ImGui::BeginTable("CollisionEntries", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Filename");
			ImGui::TableSetupColumn("Visible");
			ImGui::TableHeadersRow();

			auto entries = current_level->getLevel()->getCollisionEntries();
			for (auto* entry : entries) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(entry->name.c_str());
				ImGui::TableSetColumnIndex(1);
				bool render = entry->rendering;
				if (ImGui::Checkbox(("##vis_" + entry->name).c_str(), &render)) {
					entry->rendering = render;
				}
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}


