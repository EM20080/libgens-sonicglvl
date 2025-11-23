#pragma once

#define LIBGENS_ICONDATABASE_ERROR_FILE "Couldn't load icon database (normal text will be shown) "
#define LIBGENS_ICONDATABASE_ERROR_FILE_ROOT "Icon Database file doesn't have a valid root, please check XML formatting"
#define LIBGENS_ICONDATABASE_ROOT "IconDataBase"
#define LIBGENS_ICONDATABASE_ENTRY "Entry"
#define LIBGENS_ICONDATABASE_ICON "Icon"

namespace LibGens {
class IconDataBase {
protected:
map<string, string> icon_mappings;
public:
IconDataBase(string filename);
string getIcon(string object_name);
bool hasIcon(string object_name);
};
}
