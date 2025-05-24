
#define VOLK_IMPLEMENTATION
#include "utils/logger.h"
#include "platform/platform.h"
#include "platform/platform_context.h"
#include <filesystem/filesystem.h>
#include "platform/window.h"

#include <memory>
#include <Windows.h>

#include "demo/demo.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    frame::platform::WindowsPlatformContext context(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWDEFAULT);
    
    frame::platform::Platform platform(context);

    frame::filesystem::initWithContext(context);
    
    frame::platform::Window::OptionalProperties window_props;
    window_props.title = "My Application";
    window_props.extent = { 800, 600 };
    window_props.resizable = true;
    window_props.mode = frame::platform::Window::Mode::Default;
    window_props.vsync = frame::platform::Window::Vsync::On;

    platform.setWindowProperties(window_props);
    
    std::unique_ptr<frame::platform::Application> app = std::make_unique<RenderDemo>();
    
    frame::platform::ExitCode exit_code = platform.initialize();

    platform.startApplication(std::move(app));

    if (exit_code != frame::platform::ExitCode::Success) {
        return static_cast<int>(exit_code);
    }
    
    exit_code = platform.mainLoop();
    
    platform.terminate(exit_code);

    return static_cast<int>(exit_code);
    
}
