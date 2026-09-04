#include "DemoScene.h"
#include "platform/Application.h"

class Sandbox : public engine::Application {
 public:
  using Application::Application;

 protected:
  void OnStartup() override { BuildDemoScene(GetScene(), GetAssets()); }
};

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  return Sandbox("Collision Demo", 1280, 720).Run();
}
