#pragma once
#include <cmath>
#include "MeshData.h"

namespace engine::MeshFactory {

    // All primitives use the layout: position(3) + color(3), stride 6 floats.
    // Sizes are unit (centered on the origin); scale via the entity transform.

    inline MeshData Cube() {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}};
        mesh.vertices = {
            -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f, 0.2f, 0.2f, 0.2f,
        };
        mesh.indices = {
            0, 1, 2, 2, 3, 0,  // back
            4, 5, 6, 6, 7, 4,  // front
            0, 4, 7, 7, 3, 0,  // left
            1, 5, 6, 6, 2, 1,  // right
            3, 2, 6, 6, 7, 3,  // top
            0, 1, 5, 5, 4, 0,  // bottom
        };
        return mesh;
    }

    // Horizontal ground plane on the XZ axis (y = 0), facing up.
    inline MeshData Plane() {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}};
        mesh.vertices = {
            -0.5f, 0.0f, -0.5f, 0.6f, 0.6f, 0.6f,
             0.5f, 0.0f, -0.5f, 0.6f, 0.6f, 0.6f,
             0.5f, 0.0f,  0.5f, 0.7f, 0.7f, 0.7f,
            -0.5f, 0.0f,  0.5f, 0.7f, 0.7f, 0.7f,
        };
        mesh.indices = {
            0, 1, 2, 2, 3, 0,
        };
        return mesh;
    }

    // Vertical quad on the XY plane (z = 0), facing +Z. Useful for billboards.
    inline MeshData Quad() {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}};
        mesh.vertices = {
            -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
        };
        mesh.indices = {
            0, 1, 2, 2, 3, 0,
        };
        return mesh;
    }

    // UV sphere of unit diameter (radius 0.5). `segments` controls both the
    // number of stacks and sectors, so higher = smoother (and more vertices).
    inline MeshData Sphere(int segments = 64) {
        MeshData mesh;
        mesh.layout = {{0, 3}, {1, 3}};

        const float pi = 3.14159265358979323846f;
        const float radius = 0.5f;
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

                // Color from the surface normal mapped to [0, 1] for visibility.
                mesh.vertices.push_back(x / radius * 0.5f + 0.5f);
                mesh.vertices.push_back(y / radius * 0.5f + 0.5f);
                mesh.vertices.push_back(z / radius * 0.5f + 0.5f);
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

}
