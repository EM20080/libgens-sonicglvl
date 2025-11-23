#include "IconDataBase.h"

namespace LibGens {
IconDataBase::IconDataBase(string filename) {
TiXmlDocument doc(filename);
if (!doc.LoadFile()) {
Error::addMessage(Error::FILE_NOT_FOUND, LIBGENS_ICONDATABASE_ERROR_FILE + filename);
return;
}
TiXmlHandle hDoc(&doc);
TiXmlElement* pElem;
TiXmlHandle hRoot(0);
pElem=hDoc.FirstChildElement().Element();
if (!pElem) {
Error::addMessage(Error::EXCEPTION, LIBGENS_ICONDATABASE_ERROR_FILE_ROOT);
return;
}
pElem=pElem->FirstChildElement();
for(pElem; pElem; pElem=pElem->NextSiblingElement()) {
string object_name = pElem->ValueStr();
TiXmlElement* iconElem = pElem->FirstChildElement(LIBGENS_ICONDATABASE_ICON);
if (iconElem) {
const char* icon_text = iconElem->GetText();
if (icon_text) {
icon_mappings[object_name] = ToString(icon_text);
}
}
}
}
string IconDataBase::getIcon(string object_name) {
map<string, string>::iterator it = icon_mappings.find(object_name);
if (it != icon_mappings.end()) {
return it->second;
}
return "";
}
bool IconDataBase::hasIcon(string object_name) {
return icon_mappings.find(object_name) != icon_mappings.end();
}
}
