#pragma once

#include "CameraComponent.h"
#include "ColliderComponent.h"
#include "InputComponent.h"
#include "MeshComponent.h"
#include "NameComponent.h"
#include "PhysicsComponent.h"
#include "SpriteComponent.h"
#include "TextureComponent.h"
#include "TransformComponent.h"
#include "NameComponent.h"

namespace engine {


template <class T> struct Reflect;

template <> struct Reflect<NameComponent> {
  static constexpr const char* kName = "Name";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("name", c.name);
  }
};

template <> struct Reflect<TransformComponent> {
  static constexpr const char* kName = "Transform";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("position", c.position);
    visit("rotation", c.rotation_quat);
    visit("scale", c.scale);
  }
};

template <> struct Reflect<CameraComponent> {
  static constexpr const char* kName = "Camera";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("fov", c.fov);
    visit("near_plane", c.near_plane);
    visit("far_plane", c.far_plane);
    visit("active", c.active);
  }
};

template <> struct Reflect<MeshComponent> {
  static constexpr const char* kName = "Mesh";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("mesh", c.mesh);
  }
};

template <> struct Reflect<TextureComponent> {
  static constexpr const char* kName = "Texture";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("texture", c.texture);
  }
};

template <> struct Reflect<SpriteComponent> {
  static constexpr const char* kName = "Sprite";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("texture", c.texture);
    visit("width", c.width);
    visit("height", c.height);
  }
};

template <> struct Reflect<RigidBodyComponent> {
  static constexpr const char* kName = "RigidBody";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("linear_velocity", c.linear_velocity);
    visit("acceleration", c.acceleration);
    visit("angular_velocity", c.angular_velocity);
    visit("angular_acceleration", c.angular_acceleration);
    visit("inverse_inertia", c.inverse_inertia);
    visit("gravity_scale", c.gravity_scale);
    visit("restitution", c.restitution);
    visit("inverse_mass", c.inverse_mass);

  }
};

template <> struct Reflect<MovementComponent> {
  static constexpr const char* kName = "Movement";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("move_direction", c.move_direction);
    visit("facing", c.facing);
    visit("speed", c.speed);
    visit("yaw", c.yaw);
    visit("pitch", c.pitch);
  }
};

template <> struct Reflect<ColliderComponent> {
  static constexpr const char* kName = "Collider";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("is_static", c.is_static);
    visit("child_shapes", c.child_shapes);
  }
};

template <> struct Reflect<InputComponent> {
  static constexpr const char* kName = "Input";
  template <class C, class V> static void Fields(C& c, V&& visit) {
    visit("up", c.up);
    visit("down", c.down);
    visit("left", c.left);
    visit("right", c.right);
    visit("rotate_left", c.rotate_left);
    visit("rotate_right", c.rotate_right);
    visit("rotate_up", c.rotate_up);
    visit("rotate_down", c.rotate_down);
    visit("camera_up", c.camera_up);
    visit("camera_down", c.camera_down);
  }
};

}  // namespace engine
