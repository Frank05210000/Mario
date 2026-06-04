#include "App.hpp"

#include "Core/Context.hpp"
#include "GameLogger.hpp"
#include "Util/Logger.hpp"

int main(int, char**) {
    GameLogger::Init();
    LOG_INFO("Game started.");

    auto context = Core::Context::GetInstance();
    GameLogger::Init();
    App app;

    while (!context->GetExit()) {
        switch (app.GetCurrentState()) {
            case App::State::START:
                app.Start();
                break;

            case App::State::UPDATE:
                app.Update();
                break;

            case App::State::END:
                app.End();
                context->SetExit(true);
                break;
        }
        context->Update();
    }

    LOG_INFO("Game exiting.");
    GameLogger::Shutdown();
    return 0;
}
