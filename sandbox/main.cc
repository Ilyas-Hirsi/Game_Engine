#include "platform/Application.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/ColliderComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TextureComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "core/Log.h"
#include "platform/MeshFactory.h"
#include "ui/Inspector.h"

#include <imgui.h>
#include <random>

// Arena is a +/-kHalf box. The floor sits at y = -kHalf; four invisible
// wall planes face inward so nothing escapes sideways.
constexpr float kHalf       = 100.0f;
constexpr float kBodyExtent = 4.0f;  // spawn margin: roughly a character's reach
// Each character is 6 primitives, so entity-pair narrow phase is ~36 child
// tests; fewer, larger bodies than the old sphere demo keeps the cost sane.
constexpr int   kBodyCount  = 3000;

// A character is one rigid body of primitive children: a sphere head, capsule
// torso, capsule arms (T-pose) and legs. One table drives both the render mesh
// and the collider, so visuals and physics cannot drift apart. Sizes are world
// units; entities keep scale = 1 (see the MeshFactory capsule note).
struct BodyPart {
  bool is_sphere;
  float radius;
  float height;  // capsule cylinder section; ignored for spheres
  glm::vec3 offset;
  glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

static const glm::quat kArmRot =
    glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

static const BodyPart kCharacterParts[] = {
    {true,  0.8f, 0.0f, { 0.0f,  2.6f, 0.0f}},           // head
    {false, 1.0f, 2.0f, { 0.0f,  0.0f, 0.0f}},           // torso
    {false, 0.4f, 1.6f, {-1.7f,  0.6f, 0.0f}, kArmRot},  // left arm
    {false, 0.4f, 1.6f, { 1.7f,  0.6f, 0.0f}, kArmRot},  // right arm
    {false, 0.5f, 1.6f, {-0.55f, -2.5f, 0.0f}},          // left leg
    {false, 0.5f, 1.6f, { 0.55f, -2.5f, 0.0f}}           // right leg
};

static engine::MeshData BuildCharacterMesh() {
  engine::MeshData mesh;
  for (const BodyPart& part : kCharacterParts) {
    const engine::MeshData prim = part.is_sphere
        ? engine::MeshFactory::Sphere(16, part.radius)
        : engine::MeshFactory::Capsule(16, part.radius, part.height);
    engine::MeshFactory::Append(mesh, prim, part.offset, part.rotation);
  }
  return mesh;
}

static engine::ColliderComponent BuildCharacterCollider() {
  engine::ColliderComponent collider;
  for (const BodyPart& part : kCharacterParts) {
    engine::ChildShape child;
    if (part.is_sphere) {
      child.shape = engine::Sphere{part.radius};
    } else {
      child.shape = engine::Capsule{part.radius, part.height};
    }
    child.local_position = part.offset;
    child.local_rotation = part.rotation;
    collider.child_shapes.push_back(child);
  }
  return collider;
}

// --- Prefabs ---------------------------------------------------------------
// Each returns the entity it created, keeping one archetype's component layout
// in one place. These are the seam a scene loader or editor would call later.

static engine::Entity SpawnCamera(engine::Scene& scene, const glm::vec3& position) {
  engine::Entity camera = scene.CreateEntity();

  engine::TransformComponent transform{};
  transform.position = position;
  scene.emplace<engine::TransformComponent>(camera, transform);

  engine::CameraComponent camera_component{};
  camera_component.fov = 60.0f;
  camera_component.near_plane = 0.1f;
  camera_component.far_plane = 1000.0f;
  scene.emplace<engine::CameraComponent>(camera, camera_component);

  engine::RigidBodyComponent rigid_body{};
  rigid_body.inverse_mass = 0.0f;
  scene.emplace<engine::RigidBodyComponent>(camera, rigid_body);

  engine::MovementComponent movement{};
  movement.facing = glm::normalize(glm::vec3(0.0f, -0.25f, -1.0f));
  movement.speed = 40.0f;
  scene.emplace<engine::MovementComponent>(camera, movement);

  scene.emplace<engine::InputComponent>(camera, engine::InputComponent{});
  return camera;
}

static engine::Entity SpawnFloor(engine::Scene& scene, engine::MeshHandle plane_mesh) {
  engine::Entity floor = scene.CreateEntity();

  engine::TransformComponent transform{};
  transform.position = glm::vec3(0.0f, -kHalf, 0.0f);
  transform.scale = glm::vec3(2.0f * kHalf, 1.0f, 2.0f * kHalf);
  scene.emplace<engine::TransformComponent>(floor, transform);

  engine::MeshComponent mesh{};
  mesh.mesh = plane_mesh;
  mesh.entity_id = floor.GetId();
  scene.emplace<engine::MeshComponent>(floor, mesh);

  engine::ColliderComponent collider{};
  collider.child_shapes = {{engine::Plane{glm::vec3(0.0f, 1.0f, 0.0f), -kHalf}}};
  collider.is_static = true;
  scene.emplace<engine::ColliderComponent>(floor, collider);
  return floor;
}

// Invisible static plane collider facing inward.
static engine::Entity SpawnWall(engine::Scene& scene, const glm::vec3& normal) {
  engine::Entity wall = scene.CreateEntity();
  scene.emplace<engine::TransformComponent>(wall, engine::TransformComponent{});

  engine::ColliderComponent collider{};
  collider.child_shapes = {{engine::Plane{normal, -kHalf}}};
  collider.is_static = true;
  scene.emplace<engine::ColliderComponent>(wall, collider);
  return wall;
}

static engine::Entity SpawnCharacter(engine::Scene& scene, engine::MeshHandle mesh_handle,
                                     engine::TextureHandle texture, const glm::vec3& position) {
  engine::Entity body = scene.CreateEntity();

  engine::TransformComponent transform{};
  transform.position = position;
  // Character sizes are baked into the mesh and collider; scaling the
  // transform would desync them (and break the capsules), so stay at 1.
  transform.scale = glm::vec3(1.0f);
  scene.emplace<engine::TransformComponent>(body, transform);

  engine::MeshComponent mesh{};
  mesh.mesh = mesh_handle;
  mesh.entity_id = body.GetId();
  scene.emplace<engine::MeshComponent>(body, mesh);
  scene.emplace<engine::TextureComponent>(body, engine::TextureComponent{texture});

  scene.emplace<engine::ColliderComponent>(body, BuildCharacterCollider());

  engine::RigidBodyComponent rigid_body{};
  rigid_body.inverse_mass = 1.0f;
  rigid_body.inverse_inertia = glm::vec3(0.1f, 0.1f, 0.1f);
  // Seed the interpolation history at the spawn point, otherwise the first
  // rendered frame lerps from the origin.
  rigid_body.previous_position = position;
  scene.emplace<engine::RigidBodyComponent>(body, rigid_body);
  return body;
}

// --- App -------------------------------------------------------------------

class Sandbox : public engine::Application {
 public:
  using Application::Application;

 protected:
  void OnStartup() override {
    engine::Scene& scene = GetScene();
    engine::AssetRegistry& assets = GetAssets();

    assets.RegisterMeshBuilder("plane", engine::MeshFactory::Plane);
    assets.RegisterMeshBuilder("character", BuildCharacterMesh);

    SpawnCamera(scene, glm::vec3(0.0f, 20.0f, 130.0f));

    SpawnFloor(scene, assets.Mesh("plane"));
    SpawnWall(scene, glm::vec3( 1.0f, 0.0f,  0.0f));  // left  (x >= -kHalf)
    SpawnWall(scene, glm::vec3(-1.0f, 0.0f,  0.0f));  // right (x <=  kHalf)
    SpawnWall(scene, glm::vec3( 0.0f, 0.0f,  1.0f));  // back  (z >= -kHalf)
    SpawnWall(scene, glm::vec3( 0.0f, 0.0f, -1.0f));  // front (z <=  kHalf)

    // One baked mesh shared by every character; the compound collider mirrors
    // it part-for-part via kCharacterParts.
    const engine::MeshHandle character_mesh = assets.Mesh("character");
    const engine::TextureHandle crate_tex =
        assets.Texture("C:/Users/ilyas/Downloads/mauga.jpeg");  // path to test texture

    std::mt19937 rng(1337u);
    std::uniform_real_distribution<float> spread(-kHalf + kBodyExtent, kHalf - kBodyExtent);
    std::uniform_real_distribution<float> height(10.0f, 80.0f);
    for (int i = 0; i < kBodyCount; ++i) {
      SpawnCharacter(scene, character_mesh, crate_tex,
                     glm::vec3(spread(rng), height(rng), spread(rng)));
    }

    engine::LogInfo(
        "Collision demo: compound character ragdolls fall into a walled pen. "
        "WASD/arrows move the camera.");
  }

  void OnImGui() override {
    ImGui::Begin("Engine");
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
    // Writes straight into the live PhysicsSettings the fixed step reads.
    ImGui::DragFloat3("Gravity", &GetScene().GetPhysicsSettings().gravity.x, 0.5f);
    ImGui::End();

    engine::DrawInspector(GetScene(), GetAssets(), selected_);
  }

 private:
  entity_t selected_ = 0;
};

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  return Sandbox("Collision Demo", 1280, 720).Run();
}
