#include "StdAfx.h"
#include "EditorApplication.h"

void EditorApplication::openMultiSetParamDlg()
{
	show_multiset_dialog = true;
	clearMultiSetParamDlg();
}

void EditorApplication::closeMultiSetParamDlg()
{
	show_multiset_dialog = false;
	deleteTemporaryNodes();
}

void EditorApplication::clearMultiSetParamDlg()
{
	multiset_vec_x = 0.0f;
	multiset_vec_y = 0.0f;
	multiset_vec_z = 0.0f;
	multiset_count = 0;
	multiset_spacing = 0.0f;
}

void EditorApplication::createMultiSetParamObjects()
{
	if (multiset_count < 1 || selected_nodes.size() < 1)
	{
		deleteTemporaryNodes();
		return;
	}

	LibGens::Vector3 pos_vector(multiset_vec_x, multiset_vec_y, multiset_vec_z);

	list<EditorNode*>::iterator it;

	for (it = selected_nodes.begin(); it != selected_nodes.end(); ++it)
	{
		if ((*it)->getType() == EDITOR_NODE_OBJECT)
		{
			ObjectNode* obj_node = static_cast<ObjectNode*>(*it);
			LibGens::Object* obj = obj_node->getObject();
			LibGens::Vector3 base_pos = obj->getPosition();
			LibGens::Quaternion base_rot = obj->getRotation();
			LibGens::Vector3 new_pos;

			if (cloning_mode == SONICGLVL_MULTISETPARAM_MODE_CLONE)
			{
				for (int i = 1; i <= multiset_count; ++i)
				{
					new_pos = base_pos + (pos_vector * (i * multiset_spacing));
					LibGens::Object* new_obj = new LibGens::Object(obj);
					new_obj->setPosition(new_pos);
					new_obj->setRotation(base_rot);

					if (current_level) {
						if (current_level->getLevel()) {
							new_obj->setID(current_level->getLevel()->newObjectID());
						}
					}

					if (current_set) {
						current_set->addObject(new_obj);

						if (!current_level) {
							new_obj->setID(current_set->newObjectID());
						}
					}

					ObjectNode* new_object_node = object_node_manager->createObjectNode(new_obj);
				}
			}
			else if (cloning_mode == SONICGLVL_MULTISETPARAM_MODE_MSP)
			{
				// remove old instances
				obj->getMultiSetParam()->removeAllNodes();

				for (int i = 1; i <= multiset_count; ++i)
				{
					new_pos = base_pos + (pos_vector * (i * multiset_spacing));
					LibGens::MultiSetNode* msp_node = new LibGens::MultiSetNode();

					msp_node->position = new_pos;
					msp_node->rotation = base_rot;

					obj->getMultiSetParam()->addNode(msp_node);
				}

				obj_node->createObjectMultiSetNodes(obj, scene_manager);
				obj_node->clearNames();
				object_node_manager->reloadObjectNode(obj);
			}
		}
	}

	deleteTemporaryNodes();
}

void EditorApplication::setVectorAndSpacing()
{
	list<EditorNode*>::iterator it = selected_nodes.begin();
	if ((*it)->getType() == EDITOR_NODE_OBJECT)
	{
		ObjectNode* obj_node = static_cast<ObjectNode*>(*it);
		LibGens::Vector3 tgt_pos = obj_node->getObject()->getPosition();

		it = cloning_nodes.begin();
		if ((*it)->getType() == EDITOR_NODE_OBJECT)
		{
			ObjectNode* origin_node = static_cast<ObjectNode*>(*it);
			LibGens::Vector3 originPos = origin_node->getObject()->getPosition();
			LibGens::Vector3 v = tgt_pos - originPos;

			multiset_spacing = v.normalise();
			multiset_vec_x = v.x;
			multiset_vec_y = v.y;
			multiset_vec_z = v.z;
			multiset_count = 1;
		}
	}
}

void EditorApplication::getVectorFromObject()
{
	if (!selected_nodes.size())
		return;

	list<EditorNode*>::iterator it = selected_nodes.begin();

	if ((*it)->getType() == EDITOR_NODE_OBJECT)
	{
		ObjectNode* obj_node = static_cast<ObjectNode*>(*it);

		Ogre::Quaternion obj_rotation = obj_node->getRotation();
		Ogre::Vector3 direction(0, 0, 1);
		direction = obj_rotation * direction;

		multiset_vec_x = direction.x;
		multiset_vec_y = direction.y;
		multiset_vec_z = direction.z;
	}
}

void EditorApplication::setCloningMode(size_t mode)
{
	cloning_mode = mode;
}

void EditorApplication::deleteTemporaryNodes()
{
	if (!temporary_nodes.size()) return;

	for (list<EditorNode*>::iterator it = temporary_nodes.begin(); it != temporary_nodes.end(); it++) {
		if ((*it)->getType() == EDITOR_NODE_OBJECT) {
			ObjectNode* object_node = static_cast<ObjectNode*>(*it);

			LibGens::Object* object = object_node->getObject();
			if (object) {
				LibGens::ObjectSet* object_set = object->getParentSet();
				if (object_set) {
					object_set->eraseObject(object);
				}

				(*it)->setSelect(false);
				object_node_manager->deleteObjectNode(object);
				delete object;
			}
		}
	}

	temporary_nodes.clear();
}

void EditorApplication::renderMultiSetDialog() {
	if (!show_multiset_dialog) return;

	if (ImGui::Begin("Multi Set / Clone", &show_multiset_dialog, ImGuiWindowFlags_NoResize)) {
		ImGui::Text("Cloning Mode:");
		if (ImGui::RadioButton("Clone", cloning_mode == SONICGLVL_MULTISETPARAM_MODE_CLONE)) {
			setCloningMode(SONICGLVL_MULTISETPARAM_MODE_CLONE);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Multi Set Param", cloning_mode == SONICGLVL_MULTISETPARAM_MODE_MSP)) {
			setCloningMode(SONICGLVL_MULTISETPARAM_MODE_MSP);
		}
		
		ImGui::Separator();
		
		ImGui::Text("Direction Vector:");
		ImGui::InputFloat("X##multiset", &multiset_vec_x);
		ImGui::InputFloat("Y##multiset", &multiset_vec_y);
		ImGui::InputFloat("Z##multiset", &multiset_vec_z);
		
		ImGui::Separator();
		
		ImGui::InputFloat("Spacing", &multiset_spacing);
		ImGui::InputInt("Count", &multiset_count);
		if (multiset_count < 0) multiset_count = 0;
		
		ImGui::Separator();
		
		if (ImGui::Button("Get Vector from Object")) {
			getVectorFromObject();
		}
		
		if (ImGui::Button("Clear")) {
			clearMultiSetParamDlg();
		}
		
		if (ImGui::Button("Create")) {
			createMultiSetParamObjects();
		}
	}
	ImGui::End();
}