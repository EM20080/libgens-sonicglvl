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
#include "MessageTypes.h"

int global_cursor_state=0;

void EditorApplication::focus() {
	SDL_RaiseWindow(sdl_window);
	HWND hwnd = NULL;
	window->getCustomAttribute("WINDOW", &hwnd);
	if (hwnd && GetFocus() != hwnd) {
		SetFocus(hwnd);
	}
}

bool EditorApplication::inFocus() {
	return (SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void EditorApplication::updateCursor() {
	switch (global_cursor_state) {
		case 0:
			SDL_SetCursor(cursor_arrow);
			break;
		case 1:
			SDL_SetCursor(cursor_hand);
			break;
		case 2:
			SDL_SetCursor(cursor_sizeall);
			break;
		case 3:
			SDL_SetCursor(cursor_cross);
			break;
	}
}

void EditorApplication::handleSDLEvent(const SDL_Event& event) {
	ImGui_ImplSDL2_ProcessEvent(&event);
	
	if (event.type == SDL_QUIT) {
		shut_down = true;
		return;
	}
}

void EditorApplication::renderMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open Stage...", "Ctrl+O")) {
				openLevelGUI();
			}
			if (ImGui::MenuItem("Save Stage Data...", "Ctrl+S")) {
				saveLevelDataGUI();
			}
			if (ImGui::MenuItem("Save Stage Resources...")) {
				saveLevelResourcesGUI();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Convert/Fix All Materials (Unleashed)")) {
				convertMaterialsToUnleashed();
			}
			if (ImGui::MenuItem("Convert/Fix All Materials (Unleashed Shaders)")) {
				convertMaterialsToUnleashedShaders();
			}
			if (ImGui::MenuItem("Convert/Fix All Materials (Generations)")) {
				convertMaterialsToGenerations();
			}
			if (ImGui::MenuItem("Convert/Fix All Materials (Lost World)")) {
				convertMaterialsToLostWorld();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close")) {
				shut_down = true;
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
				undoHistory();
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
				redoHistory();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X")) {
				copySelection();
				deleteSelection();
			}
			if (ImGui::MenuItem("Copy", "Ctrl+C")) {
				copySelection();
			}
			if (ImGui::MenuItem("Paste", "Ctrl+V")) {
				pasteSelection();
			}
			if (ImGui::MenuItem("Delete", "Del")) {
				deleteSelection();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Clear Selection", "Ctrl+D")) {
				clearSelection();
			}
			if (ImGui::MenuItem("Select All", "Ctrl+A")) {
				selectAll();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Look at each other (two objects)")) {
				if (ImGui::MenuItem("Use X-Axis As Direction")) {
					lookAtEachOther(LIBGENS_MATH_AXIS_X);
				}
				if (ImGui::MenuItem("Use Y-Axis As Direction")) {
					lookAtEachOther(LIBGENS_MATH_AXIS_Y);
				}
				if (ImGui::MenuItem("Use Z-Axis As Direction")) {
					lookAtEachOther(LIBGENS_MATH_AXIS_Z);
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Look at...", "")) {
				openLookAtPointGUI();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Snap objects to closest path")) {
				snapToClosestPath();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Find", "Ctrl+F")) {
				openFindGUI();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("View")) {
			if (ImGui::BeginMenu("Show")) {
				if (ImGui::MenuItem("Objects", "Ctrl+1", (node_visibility_flags & EDITOR_NODE_OBJECT) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_OBJECT);
					toggleNodeVisibility(EDITOR_NODE_OBJECT_MSP);
				}
				if (ImGui::MenuItem("Terrain", "Ctrl+2", (node_visibility_flags & EDITOR_NODE_TERRAIN) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_TERRAIN);
				}
				if (ImGui::MenuItem("Terrain Autodraw", "Ctrl+3", (node_visibility_flags & EDITOR_NODE_TERRAIN_AUTODRAW) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_TERRAIN_AUTODRAW);
				}
				if (ImGui::MenuItem("Collision", "Ctrl+4", (node_visibility_flags & EDITOR_NODE_HAVOK) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_HAVOK);
				}
				if (ImGui::MenuItem("Paths", "Ctrl+5", (node_visibility_flags & EDITOR_NODE_PATH) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_PATH);
				}
				if (ImGui::MenuItem("Ghost", "Ctrl+6", (node_visibility_flags & EDITOR_NODE_GHOST) != 0)) {
					toggleNodeVisibility(EDITOR_NODE_GHOST);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Graphics")) {
				if (ImGui::MenuItem("Game Shaders", "F5", game_shaders_enabled)) {
					game_shaders_enabled = !game_shaders_enabled;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Framebuffer & Depth Buffer", "F6", framebuffer_enabled)) {
					framebuffer_enabled = !framebuffer_enabled;
				}
				if (ImGui::MenuItem("UV Animations", "F7", uv_animations_enabled)) {
					uv_animations_enabled = !uv_animations_enabled;
				}
				if (ImGui::MenuItem("Skybox", "F8", skybox_enabled)) {
					skybox_enabled = !skybox_enabled;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Use World Transform", "Ctrl+E", world_transform)) {
				toggleWorldTransform();
				updateSelection();
			}
			if (ImGui::MenuItem("Use Local Rotation", "", local_rotation)) {
				toggleLocalRotation();
			}
			if (ImGui::MenuItem("Use Placement Snap", "", placement_grid_snap > 0.0f)) {
				togglePlacementSnap();
			}
			if (ImGui::MenuItem("Use Rotation Snap", "", axis->isRotationSnap())) {
				toggleRotationSnap();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Bottom Panel", "", show_bottom_panel)) {
				show_bottom_panel = !show_bottom_panel;
			}
			if (current_layout == 2) {
				if (ImGui::MenuItem("Left Panel", "", show_left_panel)) {
					show_left_panel = !show_left_panel;
				}
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Terrain")) {
			if (ImGui::MenuItem("Load All Terrain...")) {
				loadAllTerrain();
			}
			if (ImGui::MenuItem("Export Scene as FBX...")) {
				exportSceneFBXGUI();
			}
			if (ImGui::MenuItem("Terrain Info...")) {
				openTerrainInfoDialog();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Objects")) {
			if (ImGui::MenuItem("Reload Object Templates Database...")) {
				reloadTemplatesDatabase();
			}
			if (ImGui::MenuItem("Save Object Templates Database...")) {
				saveTemplatesDatabase();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Materials")) {
			if (ImGui::MenuItem("Open Material Editor...")) {
				openMaterialEditorGUI();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Editor")) {
			if (ImGui::MenuItem("Save Configuration...")) {
				configuration->save(SONICGLVL_CONFIGURATION_FILE);
			}
			if (ImGui::MenuItem("Reload Configuration...")) {
				configuration->load(SONICGLVL_CONFIGURATION_FILE);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Launch Game")) {
				launchGame();
			}
			ImGui::Separator();
			
			bool connected = checkGameConnection();
			if (ImGui::MenuItem("Connect To Game", "", false, !connected)) {
				connectGame();
			}
			ImGui::Separator();
			
			if (ImGui::BeginMenu("Ghost")) {
				if (ImGui::MenuItem("Start Recording", "", false, connected && !isGhostRecording)) {
					sendMessageGame(MsgSetRecording(true), sizeof(MsgSetRecording));
				}
				if (ImGui::MenuItem("Stop Recording", "", false, connected && isGhostRecording)) {
					sendMessageGame(MsgSetRecording(false), sizeof(MsgSetRecording));
				}
				if (ImGui::MenuItem("Load Recording From Game", "", false, connected)) {
					setupGhost();
					sendMessageGame(MsgSaveRecording(), sizeof(MsgSaveRecording));
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Load From File")) {
					loadGhostRecording();
				}
				if (ImGui::MenuItem("Save Recording", "", false, ghost_data != nullptr)) {
					saveGhostRecording();
				}
				if (ImGui::MenuItem("Save Recording (FBX)", "", false, ghost_data != nullptr)) {
					saveGhostRecordingFbx();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Layout")) {
			if (ImGui::MenuItem("Layout 1 (Original)", "", current_layout == 1)) {
				current_layout = 1;
			}
			if (ImGui::MenuItem("Layout 2 (New)", "", current_layout == 2)) {
				current_layout = 2;
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Quick Overview")) {
				show_quick_overview = true;
			}
			ImGui::EndMenu();
		}
		
		ImGui::EndMainMenuBar();
	}
}

void EditorApplication::renderQuickOverviewDialog() {
	if (!show_quick_overview) return;
	
	ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Quick Overview", &show_quick_overview, ImGuiWindowFlags_NoResize)) {
		ImGui::TextWrapped("Camera Controls:");
		ImGui::Spacing();
		ImGui::BulletText("Middle Mouse Button:");
		ImGui::Indent();
		ImGui::Text("Drag: Move Camera");
		ImGui::Text("Ctrl + Drag: Zoom In/Out");
		ImGui::Text("Alt + Drag: Rotate View");
		ImGui::Text("Mouse Wheel: Zoom In/Out (Speed affected by Shift/Alt)");
		ImGui::Unindent();
		ImGui::Spacing();
		
		ImGui::BulletText("Right Mouse Button:");
		ImGui::Indent();
		ImGui::Text("Drag: Rotate View");
		ImGui::Text("WASD: Move");
		ImGui::Text("Space: Move Up");
		ImGui::Text("Ctrl: Move Down");
		ImGui::Text("Shift: Increase Move Speed");
		ImGui::Text("Alt: Decrease Move Speed");
		ImGui::Unindent();
		ImGui::Spacing();
		
		ImGui::TextWrapped("General Hotkeys:");
		ImGui::BulletText("Ctrl+O: Open Level");
		ImGui::BulletText("Ctrl+S: Save Stage Data");
		ImGui::BulletText("Ctrl+F: Find");
		ImGui::Spacing();
		
		ImGui::TextWrapped("Editing Hotkeys:");
		ImGui::BulletText("Ctrl+Z: Undo");
		ImGui::BulletText("Ctrl+Y: Redo");
		ImGui::BulletText("Ctrl+C: Copy");
		ImGui::BulletText("Ctrl+V: Paste");
		ImGui::BulletText("Ctrl+D: Clear Selection");
		ImGui::BulletText("Ctrl+A: Select All");
		ImGui::BulletText("Del: Delete Object");
		ImGui::BulletText("Ctrl+Drag: Clone Object");
		ImGui::BulletText("Shift+Drag: Clone or Instance Object");
		ImGui::Spacing();
		
		ImGui::TextWrapped("Transform Gizmos:");
		ImGui::BulletText("T: Translation Gizmo");
		ImGui::BulletText("R: Rotation Gizmo");
		ImGui::BulletText("Tap Right Mouse Button: Toggle Translation/Rotation Gizmo");
		ImGui::BulletText("Ctrl+E: Toggle World/Local Transform");
	}
	ImGui::End();
}

