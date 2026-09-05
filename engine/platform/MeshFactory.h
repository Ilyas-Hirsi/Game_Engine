#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "MeshData.h"

namespace engine::MeshFactory {

// Appends `src` into `dst`, rotating then offsetting its positions, to bake
// several primitives into one mesh. Assumes matching layouts with position
// first; an empty `dst` adopts `src`'s layout.
inline void Append(MeshData& dst, const MeshData& src,
                   const glm::vec3& offset,
                   const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
    if (dst.layout.empty()) dst.layout = src.layout;
    const int stride = src.Stride();
    const unsigned int base =
        static_cast<unsigned int>(dst.vertices.size()) / stride;

    dst.vertices.reserve(dst.vertices.size() + src.vertices.size());
    for (std::size_t v = 0; v + stride <= src.vertices.size(); v += stride) {
        const glm::vec3 p = rotation * glm::vec3(src.vertices[v],
                                                 src.vertices[v + 1],
                                                 src.vertices[v + 2]) + offset;
        dst.vertices.push_back(p.x);
        dst.vertices.push_back(p.y);
        dst.vertices.push_back(p.z);
        for (int k = 3; k < stride; ++k) dst.vertices.push_back(src.vertices[v + k]);
    }

    dst.indices.reserve(dst.indices.size() + src.indices.size());
    for (unsigned int index : src.indices) dst.indices.push_back(base + index);
}

    // All primitives use the layout: position(3) + color(3), stride 6 floats.
    // Sizes are unit (centered on the origin); scale via the entity transform.

// position(3) + color(3) + uv(2), stride 8. 24 verts so each face gets
// its own UVs (a shared 8-vert cube can't map a texture cleanly).
inline MeshData Cube(float size = 1.0f) {
    const float h = size / 2.0f;
    MeshData mesh;
    mesh.layout = {{0, 3}, {1, 3}, {6, 2}};  // uv at location 6
    mesh.vertices = {
        // front (+z)
        -h,-h, h, 1,1,1, 0,0,   h,-h, h, 1,1,1, 1,0,   h, h, h, 1,1,1, 1,1,  -h, h, h, 1,1,1, 0,1,
        // back (-z)
         h,-h,-h, 1,1,1, 0,0,  -h,-h,-h, 1,1,1, 1,0,  -h, h,-h, 1,1,1, 1,1,   h, h,-h, 1,1,1, 0,1,
        // left (-x)
        -h,-h,-h, 1,1,1, 0,0,  -h,-h, h, 1,1,1, 1,0,  -h, h, h, 1,1,1, 1,1,  -h, h,-h, 1,1,1, 0,1,
        // right (+x)
         h,-h, h, 1,1,1, 0,0,   h,-h,-h, 1,1,1, 1,0,   h, h,-h, 1,1,1, 1,1,   h, h, h, 1,1,1, 0,1,
        // top (+y)
        -h, h, h, 1,1,1, 0,0,   h, h, h, 1,1,1, 1,0,   h, h,-h, 1,1,1, 1,1,  -h, h,-h, 1,1,1, 0,1,
        // bottom (-y)
        -h,-h,-h, 1,1,1, 0,0,   h,-h,-h, 1,1,1, 1,0,   h,-h, h, 1,1,1, 1,1,  -h,-h, h, 1,1,1, 0,1,
    };
    mesh.indices = {
        0,1,2, 2,3,0,        4,5,6, 6,7,4,
        8,9,10, 10,11,8,     12,13,14, 14,15,12,
        16,17,18, 18,19,16,  20,21,22, 22,23,20,
    };
    return mesh;
}

    // Horizontal ground plane on the XZ axis (y = 0), facing up.
    inline MeshData Plane() {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}, {6, 2}};
        mesh.vertices = {
            -0.5f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
             0.5f, 0.0f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
             0.5f, 0.0f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.0f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        };
        mesh.indices = {
            0, 1, 2, 2, 3, 0,
        };
        return mesh;
    }

    // Vertical quad on the XY plane (z = 0), facing +Z. Useful for billboards.
    inline MeshData Quad() {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}, {6, 2}};
        mesh.vertices = {
            -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        };
        mesh.indices = {
            0, 1, 2, 2, 3, 0,
        };
        return mesh;
    }

    // UV sphere of unit diameter (radius 0.5). `segments` controls both the
    // number of stacks and sectors, so higher = smoother (and more vertices).
    inline MeshData Sphere(int segments = 32, float radius = 0.5f) {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}, {6, 2}};  // pos, color, uv (uv at location 6)
    
        const float pi = 3.14159265358979323846f;
        const int stacks = segments;
        const int sectors = segments;
    
        for (int i = 0; i <= stacks; ++i) {
            const float stack_angle = pi / 2.0f - static_cast<float>(i) / stacks * pi;
            const float xy = std::cos(stack_angle) * radius;
            const float z = std::sin(stack_angle) * radius;
    
            for (int j = 0; j <= sectors; ++j) {
                const float sector_angle =
                    static_cast<float>(j) / sectors * 2.0f * pi;
                const float x = xy * std::cos(sector_angle);
                const float y = xy * std::sin(sector_angle);
    
                mesh.vertices.push_back(x);
                mesh.vertices.push_back(y);
                mesh.vertices.push_back(z);
    
                mesh.vertices.push_back(1.0f);
                mesh.vertices.push_back(1.0f);
                mesh.vertices.push_back(1.0f);
    
                // UV: wrap horizontally by sector, top-to-bottom by stack.
                mesh.vertices.push_back(static_cast<float>(j) / sectors);
                mesh.vertices.push_back(static_cast<float>(i) / stacks);
            }
        }
        for (int i = 0; i < stacks; ++i) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    mesh.indices.push_back(k1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k1 + 1);
                }
                if (i != stacks - 1) {
                    mesh.indices.push_back(k1 + 1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k2 + 1);
                }
            }
        }

        return mesh;
    }
    // Capsule with long axis on Y; `height` is the cylinder section only, so
    // total height = height + 2 * radius. Sizes are baked in, so keep entity
    // scale at 1 (non-uniform scale breaks the shape).
    inline MeshData Capsule(int segments = 32, float radius = 0.5f, float height = 1.0f) {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}, {6, 2}};  // pos, color, uv (uv at location 6)

        const float pi = 3.14159265358979323846f;
        const int stacks = segments + (segments & 1);  // even, so the equator lands on a ring
        const int sectors = segments;
        const int half = stacks / 2;
        const float half_height = height / 2.0f;

        // Profile arc length from the top pole, so the texture doesn't
        // stretch across the wall: cap arc + wall + cap arc.
        const float cap_arc = 0.5f * pi * radius;
        const float total_arc = 2.0f * cap_arc + height;

        for (int i = 0; i <= stacks + 1; ++i) {
            const bool top = i <= half;
            const int ring = top ? i : i - 1;  // the equator ring is emitted twice
            const float stack_angle = pi / 2.0f - static_cast<float>(ring) / stacks * pi;
            const float ring_radius = std::cos(stack_angle) * radius;
            const float y = std::sin(stack_angle) * radius + (top ? half_height : -half_height);
            const float arc = top ? (pi / 2.0f - stack_angle) * radius
                                  : cap_arc + height - stack_angle * radius;

            for (int j = 0; j <= sectors; ++j) {
                const float sector_angle = static_cast<float>(j) / sectors * 2.0f * pi;
                const float x = ring_radius * std::sin(sector_angle);
                const float z = ring_radius * std::cos(sector_angle);

                mesh.vertices.push_back(x);
                mesh.vertices.push_back(y);
                mesh.vertices.push_back(z);

                mesh.vertices.push_back(1.0f);
                mesh.vertices.push_back(1.0f);
                mesh.vertices.push_back(1.0f);

                mesh.vertices.push_back(static_cast<float>(j) / sectors);
                mesh.vertices.push_back(arc / total_arc);
            }
        }

        const int rows = stacks + 1;  // one more band of quads than the sphere
        for (int i = 0; i < rows; ++i) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    mesh.indices.push_back(k1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k1 + 1);
                }
                if (i != rows - 1) {
                    mesh.indices.push_back(k1 + 1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k2 + 1);
                }
            }
        }
        return mesh;
    }

}
