//============================================================================================================================================
// Copied/Modified from the Ogre3D Wiki: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Ogre+Wiki+Tutorial+Framework&structure=Development
//============================================================================================================================================

#include "BaseApplication.h"
#include "EmbeddedConfig.h"
#include <d3d9.h>
#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h>
#include "Resource.h"
#pragma comment(lib, "dwmapi.lib")
#endif

// Forward declare Ogre D3D9 classes to get the device
namespace Ogre {
	class D3D9RenderSystem {
	public:
		static IDirect3DDevice9* getActiveD3D9Device();
	};
}

BaseApplication::BaseApplication(void) : root(0), scene_manager(0), window(0), resources_config(Ogre::StringUtil::BLANK),
plugin_config(Ogre::StringUtil::BLANK), shut_down(false), input_manager(0), mouse(0), keyboard(0), sdl_window(0) {

}

BaseApplication::~BaseApplication(void) {
	Ogre::WindowEventUtilities::removeWindowEventListener(window, this);
	windowClosed(window);
	delete root;
}

bool BaseApplication::configure(void) {
	if (root->showConfigDialog()) {
		window = root->initialise(true, SONICGLVL_WINDOW_NAME);

		window->setFullscreen(false, window->getWidth(), window->getHeight());

		window->setAutoUpdated(false);

		return true;
	}
	else {
		return false;
	}
}

void BaseApplication::createFrameListener(void) {
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;
	window->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
	input_manager = OIS::InputManager::createInputSystem(pl);
	keyboard = static_cast<OIS::Keyboard*>(input_manager->createInputObject(OIS::OISKeyboard, true));
	mouse = static_cast<OIS::Mouse*>(input_manager->createInputObject(OIS::OISMouse, true));
	mouse->setEventCallback(this);
	keyboard->setEventCallback(this);
	windowResized(window);
	Ogre::WindowEventUtilities::addWindowEventListener(window, this);
	root->addFrameListener(this);
}

void BaseApplication::destroyScene(void) {
}

void BaseApplication::setupResources(void) {
	Ogre::ConfigFile cf;
	cf.load(resources_config);

	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements()) {
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap* settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i) {
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
		}
	}
}

void BaseApplication::createResourceListener(void) {
}

void BaseApplication::loadResources(void) {
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void BaseApplication::go(void) {
	EmbeddedConfig::initializeConfigs();

	resources_config = SONICGLVL_RESOURCES_NAME;
	plugin_config = SONICGLVL_PLUGINS_NAME;

	char cCurrentPath[1024];
	_getcwd(cCurrentPath, sizeof(cCurrentPath));
	exe_path = ToString(cCurrentPath);

	if (!setup()) return;

	LibGens::initialize();
	LibGens::Error::setLogging(true);
	window->setDeactivateOnFocusChange(false);

	initializeImGui();

	while (true) {
		Ogre::WindowEventUtilities::messagePump();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			handleSDLEvent(event);
		}

		if (window->isClosed() || shut_down) break;

		keyboard->capture();
		mouse->capture();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)screen_width, (float)screen_height);
		io.DeltaTime = 1.0f / 60.0f;

		HWND hwnd = NULL;
		window->getCustomAttribute("WINDOW", &hwnd);
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hwnd, &cursorPos);

		io.MousePos = ImVec2((float)cursorPos.x, (float)cursorPos.y);

		OIS::MouseState& ms = const_cast<OIS::MouseState&>(mouse->getMouseState());
		ms.X.abs = cursorPos.x;
		ms.Y.abs = cursorPos.y;
		io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

		io.MouseWheel = (float)ms.Z.rel / 120.0f;
		ms.Z.rel = 0;  // Reset after reading

		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		renderImGuiContent();

		if (io.WantCaptureMouse) {
			ms.X.rel = 0;
			ms.Y.rel = 0;
		}

		Ogre::FrameEvent evt;
		evt.timeSinceLastFrame = 1.0f / 60.0f;
		if (!root->_fireFrameStarted(evt)) break;
		if (!root->_fireFrameRenderingQueued(evt)) break;

		window->update(false);

		ImGui::Render();
		IDirect3DDevice9* d3d_device = Ogre::D3D9RenderSystem::getActiveD3D9Device();
		if (d3d_device) {
			d3d_device->BeginScene();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			d3d_device->EndScene();
		}

		window->swapBuffers();
		root->_fireFrameEnded(evt);
	}

	shutdownImGui();
	destroyScene();
}

OIS::KeyCode SDLScancodeToOIS(SDL_Scancode scancode) {
	switch (scancode) {
	case SDL_SCANCODE_ESCAPE: return OIS::KC_ESCAPE;
	case SDL_SCANCODE_1: return OIS::KC_1;
	case SDL_SCANCODE_2: return OIS::KC_2;
	case SDL_SCANCODE_3: return OIS::KC_3;
	case SDL_SCANCODE_4: return OIS::KC_4;
	case SDL_SCANCODE_5: return OIS::KC_5;
	case SDL_SCANCODE_6: return OIS::KC_6;
	case SDL_SCANCODE_7: return OIS::KC_7;
	case SDL_SCANCODE_8: return OIS::KC_8;
	case SDL_SCANCODE_9: return OIS::KC_9;
	case SDL_SCANCODE_0: return OIS::KC_0;
	case SDL_SCANCODE_MINUS: return OIS::KC_MINUS;
	case SDL_SCANCODE_EQUALS: return OIS::KC_EQUALS;
	case SDL_SCANCODE_BACKSPACE: return OIS::KC_BACK;
	case SDL_SCANCODE_TAB: return OIS::KC_TAB;
	case SDL_SCANCODE_Q: return OIS::KC_Q;
	case SDL_SCANCODE_W: return OIS::KC_W;
	case SDL_SCANCODE_E: return OIS::KC_E;
	case SDL_SCANCODE_R: return OIS::KC_R;
	case SDL_SCANCODE_T: return OIS::KC_T;
	case SDL_SCANCODE_Y: return OIS::KC_Y;
	case SDL_SCANCODE_U: return OIS::KC_U;
	case SDL_SCANCODE_I: return OIS::KC_I;
	case SDL_SCANCODE_O: return OIS::KC_O;
	case SDL_SCANCODE_P: return OIS::KC_P;
	case SDL_SCANCODE_LEFTBRACKET: return OIS::KC_LBRACKET;
	case SDL_SCANCODE_RIGHTBRACKET: return OIS::KC_RBRACKET;
	case SDL_SCANCODE_RETURN: return OIS::KC_RETURN;
	case SDL_SCANCODE_LCTRL: return OIS::KC_LCONTROL;
	case SDL_SCANCODE_A: return OIS::KC_A;
	case SDL_SCANCODE_S: return OIS::KC_S;
	case SDL_SCANCODE_D: return OIS::KC_D;
	case SDL_SCANCODE_F: return OIS::KC_F;
	case SDL_SCANCODE_G: return OIS::KC_G;
	case SDL_SCANCODE_H: return OIS::KC_H;
	case SDL_SCANCODE_J: return OIS::KC_J;
	case SDL_SCANCODE_K: return OIS::KC_K;
	case SDL_SCANCODE_L: return OIS::KC_L;
	case SDL_SCANCODE_SEMICOLON: return OIS::KC_SEMICOLON;
	case SDL_SCANCODE_APOSTROPHE: return OIS::KC_APOSTROPHE;
	case SDL_SCANCODE_GRAVE: return OIS::KC_GRAVE;
	case SDL_SCANCODE_LSHIFT: return OIS::KC_LSHIFT;
	case SDL_SCANCODE_BACKSLASH: return OIS::KC_BACKSLASH;
	case SDL_SCANCODE_Z: return OIS::KC_Z;
	case SDL_SCANCODE_X: return OIS::KC_X;
	case SDL_SCANCODE_C: return OIS::KC_C;
	case SDL_SCANCODE_V: return OIS::KC_V;
	case SDL_SCANCODE_B: return OIS::KC_B;
	case SDL_SCANCODE_N: return OIS::KC_N;
	case SDL_SCANCODE_M: return OIS::KC_M;
	case SDL_SCANCODE_COMMA: return OIS::KC_COMMA;
	case SDL_SCANCODE_PERIOD: return OIS::KC_PERIOD;
	case SDL_SCANCODE_SLASH: return OIS::KC_SLASH;
	case SDL_SCANCODE_RSHIFT: return OIS::KC_RSHIFT;
	case SDL_SCANCODE_KP_MULTIPLY: return OIS::KC_MULTIPLY;
	case SDL_SCANCODE_LALT: return OIS::KC_LMENU;
	case SDL_SCANCODE_SPACE: return OIS::KC_SPACE;
	case SDL_SCANCODE_CAPSLOCK: return OIS::KC_CAPITAL;
	case SDL_SCANCODE_F1: return OIS::KC_F1;
	case SDL_SCANCODE_F2: return OIS::KC_F2;
	case SDL_SCANCODE_F3: return OIS::KC_F3;
	case SDL_SCANCODE_F4: return OIS::KC_F4;
	case SDL_SCANCODE_F5: return OIS::KC_F5;
	case SDL_SCANCODE_F6: return OIS::KC_F6;
	case SDL_SCANCODE_F7: return OIS::KC_F7;
	case SDL_SCANCODE_F8: return OIS::KC_F8;
	case SDL_SCANCODE_F9: return OIS::KC_F9;
	case SDL_SCANCODE_F10: return OIS::KC_F10;
	case SDL_SCANCODE_F11: return OIS::KC_F11;
	case SDL_SCANCODE_F12: return OIS::KC_F12;
	case SDL_SCANCODE_NUMLOCKCLEAR: return OIS::KC_NUMLOCK;
	case SDL_SCANCODE_SCROLLLOCK: return OIS::KC_SCROLL;
	case SDL_SCANCODE_KP_7: return OIS::KC_NUMPAD7;
	case SDL_SCANCODE_KP_8: return OIS::KC_NUMPAD8;
	case SDL_SCANCODE_KP_9: return OIS::KC_NUMPAD9;
	case SDL_SCANCODE_KP_MINUS: return OIS::KC_SUBTRACT;
	case SDL_SCANCODE_KP_4: return OIS::KC_NUMPAD4;
	case SDL_SCANCODE_KP_5: return OIS::KC_NUMPAD5;
	case SDL_SCANCODE_KP_6: return OIS::KC_NUMPAD6;
	case SDL_SCANCODE_KP_PLUS: return OIS::KC_ADD;
	case SDL_SCANCODE_KP_1: return OIS::KC_NUMPAD1;
	case SDL_SCANCODE_KP_2: return OIS::KC_NUMPAD2;
	case SDL_SCANCODE_KP_3: return OIS::KC_NUMPAD3;
	case SDL_SCANCODE_KP_0: return OIS::KC_NUMPAD0;
	case SDL_SCANCODE_KP_PERIOD: return OIS::KC_DECIMAL;
	case SDL_SCANCODE_RCTRL: return OIS::KC_RCONTROL;
	case SDL_SCANCODE_KP_DIVIDE: return OIS::KC_DIVIDE;
	case SDL_SCANCODE_RALT: return OIS::KC_RMENU;
	case SDL_SCANCODE_HOME: return OIS::KC_HOME;
	case SDL_SCANCODE_UP: return OIS::KC_UP;
	case SDL_SCANCODE_PAGEUP: return OIS::KC_PGUP;
	case SDL_SCANCODE_LEFT: return OIS::KC_LEFT;
	case SDL_SCANCODE_RIGHT: return OIS::KC_RIGHT;
	case SDL_SCANCODE_END: return OIS::KC_END;
	case SDL_SCANCODE_DOWN: return OIS::KC_DOWN;
	case SDL_SCANCODE_PAGEDOWN: return OIS::KC_PGDOWN;
	case SDL_SCANCODE_INSERT: return OIS::KC_INSERT;
	case SDL_SCANCODE_DELETE: return OIS::KC_DELETE;
	default: return OIS::KC_UNASSIGNED;
	}
}

void BaseApplication::handleSDLEvent(const SDL_Event& event) {
	ImGui_ImplSDL2_ProcessEvent(&event);

	ImGuiIO& io = ImGui::GetIO();

	if (event.type == SDL_QUIT) {
		shut_down = true;
	}
	else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
		bool isSystemShortcut = false;
		bool isCtrlPressed = (event.key.keysym.mod & KMOD_CTRL) != 0;

		if (isCtrlPressed) {
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_C:  // Ctrl+C (Copy)
			case SDL_SCANCODE_V:  // Ctrl+V (Paste)
			case SDL_SCANCODE_X:  // Ctrl+X (Cut)
			case SDL_SCANCODE_A:  // Ctrl+A (Select All)
			case SDL_SCANCODE_Z:  // Ctrl+Z (Undo)
			case SDL_SCANCODE_Y:  // Ctrl+Y (Redo)
			case SDL_SCANCODE_S:  // Ctrl+S (Save)
			case SDL_SCANCODE_N:  // Ctrl+N (New)
			case SDL_SCANCODE_O:  // Ctrl+O (Open)
			case SDL_SCANCODE_F:  // Ctrl+F (Find)
				isSystemShortcut = true;
				break;
			}
		}


		if ((!io.WantCaptureKeyboard && !io.WantTextInput) || isSystemShortcut) {
			OIS::KeyCode oisKey = SDLScancodeToOIS(event.key.keysym.scancode);
			if (oisKey != OIS::KC_UNASSIGNED) {
				OIS::KeyEvent keyEvent(nullptr, oisKey, event.key.keysym.sym);

				if (event.type == SDL_KEYDOWN) {
					keyPressed(keyEvent);
				}
				else {
					keyReleased(keyEvent);
				}
			}
		}
	}
}

void BaseApplication::initializeImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.MouseDrawCursor = false;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);

	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.TabRounding = 0.0f;

	ImFontConfig font_config;
	font_config.OversampleH = 2;
	font_config.OversampleV = 1;
	font_config.PixelSnapH = true;

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, &font_config);

	HWND hwnd = NULL;
	window->getCustomAttribute("WINDOW", &hwnd);

#ifdef _WIN32
	BOOL useDarkMode = TRUE;
	DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(SONICGLVL_ICON));
	if (hIcon) {
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}
#endif

	sdl_window = SDL_CreateWindowFrom(hwnd);
	ImGui_ImplSDL2_InitForD3D(sdl_window);

	IDirect3DDevice9* d3d_device = Ogre::D3D9RenderSystem::getActiveD3D9Device();
	ImGui_ImplDX9_Init(d3d_device);

	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendPlatformName = "imgui_impl_sdl2";

	while (ShowCursor(TRUE) < 0);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void BaseApplication::shutdownImGui() {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (sdl_window) {
		SDL_DestroyWindow(sdl_window);
		sdl_window = nullptr;
	}
}

void BaseApplication::renderImGui() {

}

bool BaseApplication::renderOneFrame() {
	return root->renderOneFrame();
}

bool BaseApplication::setup(void) {
	Ogre::LogManager* lm = new Ogre::LogManager();
	lm->createLog(SONICGLVL_LOG_NAME, true, false, false);

	root = new Ogre::Root(plugin_config);

	setupResources();

	bool carryOn = configure();
	if (!carryOn) return false;

	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	Ogre::Animation::setDefaultInterpolationMode(Ogre::Animation::IM_LINEAR);
	Ogre::Animation::setDefaultRotationInterpolationMode(Ogre::Animation::RIM_LINEAR);

	createResourceListener();
	loadResources();

	createScene();
	createFrameListener();
	return true;
};


bool BaseApplication::frameRenderingQueued(const Ogre::FrameEvent& evt) {
	if (window->isClosed()) return false;
	if (shut_down) return false;

	mouse->capture();
	return true;
}

void BaseApplication::windowResized(Ogre::RenderWindow* rw) {
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState& ms = mouse->getMouseState();
	ms.width = width;
	ms.height = height;

	screen_width = width;
	screen_height = height;
}

void BaseApplication::windowClosed(Ogre::RenderWindow* rw) {
	if (rw == window) {
		if (input_manager) {
			input_manager->destroyInputObject(mouse);
			input_manager->destroyInputObject(keyboard);

			OIS::InputManager::destroyInputSystem(input_manager);
			input_manager = 0;
		}
	}
}