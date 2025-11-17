#include "EditorApplication.h"

bool matchesQuery(string str1, string str2, bool exactly)
{
	// convert search query string and current object's name to lowercase

	for (size_t i = 0; i < str1.size(); ++i)
		str1[i] = tolower(str1[i]);

	for (size_t i = 0; i < str2.size(); ++i)
		str2[i] = tolower(str2[i]);

	if (exactly)
		return (!strcmp(str1.c_str(), str2.c_str()));
	else
		return str1.find(str2, 0) != string::npos;
}


bool valueMatches(LibGens::Object *object, string element_name, string value_string)
{
	LibGens::ObjectElement *element = NULL;
	element = object->getElement(element_name);
	if (!element) {
		string element_name_lower = element_name;
		for (size_t i = 0; i < element_name_lower.size(); ++i)
			element_name_lower[i] = tolower(element_name_lower[i]);
		
		list<LibGens::ObjectElement*> elements = object->getElements();
		for (list<LibGens::ObjectElement*>::iterator it = elements.begin(); it != elements.end(); ++it) {
			string current_name = (*it)->getName();
			string current_name_lower = current_name;
			for (size_t i = 0; i < current_name_lower.size(); ++i)
				current_name_lower[i] = tolower(current_name_lower[i]);
			
			if (element_name_lower == current_name_lower) {
				element = *it;
				break;
			}
		}
	}
	
	if (!element)
		return false;

	LibGens::ObjectElementType element_type = element->getType();
	string element_value = "";

	// cast the object element to the appropriate type

	switch (element_type)
	{
	case LibGens::OBJECT_ELEMENT_BOOL:
	{
		LibGens::ObjectElementBool* element_bool = static_cast<LibGens::ObjectElementBool*>(element);
		bool value = element_bool->value;
		element_value = value ? "true" : "false";
		break;
	}

	case LibGens::OBJECT_ELEMENT_FLOAT:
	{
		LibGens::ObjectElementFloat* element_float = static_cast<LibGens::ObjectElementFloat*>(element);
		float value = element_float->value;
		element_value = ToString<float>(value);
		break;
	}

	case LibGens::OBJECT_ELEMENT_ID:
	{
		LibGens::ObjectElementID* element_id = static_cast<LibGens::ObjectElementID*>(element);
		size_t value = element_id->value;
		element_value = ToString<size_t>(value);
		break;
	}

	case LibGens::OBJECT_ELEMENT_INTEGER:
	{
		LibGens::ObjectElementInteger* element_int = static_cast<LibGens::ObjectElementInteger*>(element);
		unsigned int value = element_int->value;
		element_value = ToString<unsigned int>(value);
		break;
	}

	case LibGens::OBJECT_ELEMENT_STRING:
	{
		LibGens::ObjectElementString* element_string = static_cast<LibGens::ObjectElementString*>(element);
		element_value = element_string->value;
		break;
	}

	case LibGens::OBJECT_ELEMENT_VECTOR:
	{
		LibGens::ObjectElementVector* element_vector = static_cast<LibGens::ObjectElementVector*>(element);
		LibGens::Vector3 v3 = element_vector->value;
		element_value = ToString<float>(v3.x) + ", " + ToString<float>(v3.y) + ", " + ToString<float>(v3.z);
		break;
	}

	case LibGens::OBJECT_ELEMENT_ID_LIST:
	{
		LibGens::ObjectElementIDList* element_id_list = static_cast<LibGens::ObjectElementIDList*>(element);
		vector<size_t> id_list = element_id_list->value;
		vector<string> id_list_str;
		for (size_t count = 0; count < id_list.size(); ++count)
		{
			id_list_str.push_back(ToString<size_t>(id_list[count]));
		}

		for (size_t count = 0; count < id_list_str.size(); ++count)
		{
			if (matchesQuery(id_list_str[count], value_string, true))
				return true;
		}

		return false;
	}

	case LibGens::OBJECT_ELEMENT_VECTOR_LIST:
	{
		LibGens::ObjectElementVectorList* element_vector_list = static_cast<LibGens::ObjectElementVectorList*>(element);
		vector<LibGens::Vector3> vector_list = element_vector_list->value;
		vector<string> vector_list_str;

		for (size_t count = 0; count < vector_list.size(); ++count)
		{
			LibGens::Vector3 v3 = vector_list[count];
			string v3_str = ToString<float>(v3.x) + ", " + ToString<float>(v3.y) + ", " + ToString<float>(v3.z);
			vector_list_str.push_back(v3_str);
		}

		for (size_t count = 0; count < vector_list_str.size(); ++count)
		{
			if (matchesQuery(vector_list_str[count], value_string, true))
				return true;
		}

		return false;
	}

	default:
		break;
	}

	return matchesQuery(element_value, value_string, true);
}

void EditorApplication::openFindGUI()
{
	if (!show_find_dialog)
	{
		find_position = object_node_manager->getObjectNodes().begin();
		find_object_name[0] = '\0';
		find_property_name[0] = '\0';
		find_property_value[0] = '\0';
		find_match_exactly = false;
		find_with_filter = false;
		find_select_all = false;
	}
	show_find_dialog = true;
}

void EditorApplication::closeFindGUI()
{
	show_find_dialog = false;
}

void EditorApplication::findNext(string obj_name, string param, string value)
{
	if (!obj_name.size()) return;

	bool found = false;
	bool exact = find_match_exactly;

	for (list<ObjectNode*>::iterator it = find_position; it != object_node_manager->getObjectNodes().end(); ++it)
	{
		ObjectNode* object_node = *it;
		LibGens::Object* object = object_node->getObject();

		if (object_node->isForceHidden() || !set_visibility[object->getParentSet()])
			continue;

		if (matchesQuery(object->getName(), obj_name, exact))
		{
			if (param != "" && value != "")
				if (!valueMatches(object, param, value))
					continue;

			clearSelection();
			selectNode(object_node);

			find_position = it;
			if (it != object_node_manager->getObjectNodes().end())
				find_position++;

			found = true;
		}

		if (found)
			break;
	}

	if (!found)
	{
		MessageBox(NULL, "No more matches found.", "SonicGlvl", MB_OK);
		find_position = object_node_manager->getObjectNodes().begin();
	}

	updateSelection();
}

void EditorApplication::findAll(string obj_name, string param, string value)
{
	if (!obj_name.size()) return;

	list<ObjectNode*> object_nodes = object_node_manager->getObjectNodes();
	clearSelection();

	bool exact = find_match_exactly;

	for (list<ObjectNode*>::iterator it = object_nodes.begin(); it != object_nodes.end(); ++it)
	{
		ObjectNode* object_node = *it;
		LibGens::Object* object = object_node->getObject();

		if (object_node->isForceHidden() || !set_visibility[object->getParentSet()])
			continue;

		if (matchesQuery(object->getName(), obj_name, exact))
		{
			if (param != "" && value != "")
				if (!valueMatches(object, param, value))
					continue;

			object_node->setSelect(true);
			selected_nodes.push_back(object_node);
		}
	}

	updateSelection();
}

void EditorApplication::renderFindDialog() {
	if (!show_find_dialog) return;

	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSizeConstraints(ImVec2(380, 0), ImVec2(380, FLT_MAX));
	
	if (ImGui::Begin("Find", &show_find_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Basic Options");
		
		ImGui::Text("Object Name");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##ObjectName", find_object_name, 256);
		
		ImGui::Spacing();
		ImGui::Checkbox("Find And Select All", &find_select_all);
		ImGui::Checkbox("Match Exactly", &find_match_exactly);
		
		ImGui::Spacing();
		ImGui::Text("Filter Options");
		ImGui::Checkbox("With Property And Value", &find_with_filter);
		
		if (find_with_filter) {
			ImGui::Text("Property");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##PropertyName", find_property_name, 256);
			
			ImGui::Text("Value");
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##PropertyValue", find_property_value, 256);
		}
		
		ImGui::Spacing();
		
		float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
		
		if (ImGui::Button("Find Next", ImVec2(button_width, 0))) {
			string obj_name = find_object_name;
			string property = find_with_filter ? find_property_name : "";
			string value = find_with_filter ? find_property_value : "";
			
			if (find_select_all) {
				findAll(obj_name, property, value);
			} else {
				findNext(obj_name, property, value);
			}
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Close", ImVec2(button_width, 0))) {
			closeFindGUI();
		}
	}
	ImGui::End();
}
