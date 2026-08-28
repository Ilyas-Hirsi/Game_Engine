#pragma once
#include "Collision.h"
#include "Entity.h"
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <glm/vector_relational.hpp>
namespace engine {
struct BVHNode {
    AABB box;
    entity_t entity;
    int parent = -1;
    int left = -1;
    int right = -1;
    int height = 0;
  };

  inline bool IsLeaf(const BVHNode& node){ return node.left == -1; }

  inline AABB Union(const AABB& a, const AABB& b){
    return AABB{glm::min(a.min, b.min), glm::max(a.max, b.max)};
  }
  inline float SurfaceArea(const AABB& aabb){
    const glm::vec3 size = aabb.max - aabb.min;
    return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
  }
  inline bool Contains(const AABB& outer, const AABB& inner){
    return  glm::all(glm::lessThanEqual(outer.min, inner.min)) &&
            glm::all(glm::greaterThanEqual(outer.max, inner.max));
  }
  inline bool Intersects(const AABB& a, const AABB& b){
    return glm::all(glm::lessThanEqual(a.min, b.max)) &&
           glm::all(glm::lessThanEqual(b.min, a.max));
  }
  inline AABB Expand(const AABB& a, float margin = 1.5f){
    return AABB{a.min - glm::vec3(margin), a.max + glm::vec3(margin)};
  }

  class BVHTree {
  public:

    // Fattens the box, inserts it, and returns the node index used to update or
    // remove it later.
    int InsertLeaf(entity_t entity, const AABB& tight_box){
      const int leaf = AllocateNode();
      nodes[leaf].box = Expand(tight_box);
      nodes[leaf].entity = entity;
      nodes[leaf].height = 0;
      nodes[leaf].left = -1;
      nodes[leaf].right = -1;

      if (root == -1){
        root = leaf;
        nodes[leaf].parent = -1;
        return leaf;
      }

      const AABB leaf_box = nodes[leaf].box;
      int index = root;
      while (!IsLeaf(nodes[index])){
        const int left = nodes[index].left;
        const int right = nodes[index].right;

        const float area = SurfaceArea(nodes[index].box);
        const float combined = SurfaceArea(Union(nodes[index].box, leaf_box));
        const float cost = 2.0f * combined;
        const float inherit = 2.0f * (combined - area);

        // Cost of pushing the leaf further down each child.
        auto descent_cost = [&](int child){
          const float grown = SurfaceArea(Union(nodes[child].box, leaf_box));
          if (IsLeaf(nodes[child])) return grown + inherit;
          return grown - SurfaceArea(nodes[child].box) + inherit;
        };

        const float cost_left = descent_cost(left);
        const float cost_right = descent_cost(right);

        if (cost < cost_left && cost < cost_right) break;
        index = (cost_left < cost_right) ? left : right;
      }
      const int sibling = index;

      const int old_parent = nodes[sibling].parent;
      const int new_parent = AllocateNode();
      nodes[new_parent].parent = old_parent;
      nodes[new_parent].box = Union(leaf_box, nodes[sibling].box);
      nodes[new_parent].height = nodes[sibling].height + 1;
      nodes[new_parent].left = sibling;
      nodes[new_parent].right = leaf;
      nodes[sibling].parent = new_parent;
      nodes[leaf].parent = new_parent;

      if (old_parent != -1){
        if (nodes[old_parent].left == sibling)
          nodes[old_parent].left = new_parent;
        else
          nodes[old_parent].right = new_parent;
      } else {
        root = new_parent;
      }

      index = nodes[leaf].parent;
      while (index != -1){
        index = Balance(index);
        const int left = nodes[index].left;
        const int right = nodes[index].right;
        nodes[index].height = 1 + std::max(nodes[left].height, nodes[right].height);
        nodes[index].box = Union(nodes[left].box, nodes[right].box);
        index = nodes[index].parent;
      }
      return leaf;
    }

    void RemoveLeaf(int leaf){
      if (leaf == root){
        root = -1;
        FreeNode(leaf);
        return;
      }

      const int parent = nodes[leaf].parent;
      const int grandparent = nodes[parent].parent;
      const int sibling = (nodes[parent].left == leaf) ? nodes[parent].right
                                                       : nodes[parent].left;

      if (grandparent != -1){
        if (nodes[grandparent].left == parent)
          nodes[grandparent].left = sibling;
        else
          nodes[grandparent].right = sibling;
        nodes[sibling].parent = grandparent;
        FreeNode(parent);

        int index = grandparent;
        while (index != -1){
          index = Balance(index);
          const int left = nodes[index].left;
          const int right = nodes[index].right;
          nodes[index].box = Union(nodes[left].box, nodes[right].box);
          nodes[index].height = 1 + std::max(nodes[left].height, nodes[right].height);
          index = nodes[index].parent;
        }
      } else {
        root = sibling;
        nodes[sibling].parent = -1;
        FreeNode(parent);
      }
      FreeNode(leaf);
    }

    // Only touches the tree when the object leaves its fattened box. Returns the
    // node index, which changes on reinsertion.
    int UpdateLeaf(int leaf, const AABB& tight_box){
      if (Contains(nodes[leaf].box, tight_box)) return leaf;
      const entity_t entity = nodes[leaf].entity;
      RemoveLeaf(leaf);
      return InsertLeaf(entity, tight_box);
    }

    // Calls fn(entity_t) for every leaf whose box overlaps `box`. Not reentrant.
    template <typename Fn>
    void Query(const AABB& box, Fn&& fn) const {
      if (root == -1) return;
      static thread_local std::vector<int> stack;
      stack.clear();
      stack.push_back(root);
      while (!stack.empty()){
        const int index = stack.back();
        stack.pop_back();
        const BVHNode& node = nodes[index];
        if (!Intersects(node.box, box)) continue;
        if (IsLeaf(node)){
          fn(node.entity);
        } else {
          stack.push_back(node.left);
          stack.push_back(node.right);
        }
      }
    }

    bool Empty() const { return root == -1; }
    int Height() const { return root == -1 ? 0 : nodes[root].height; }
    void Clear(){ nodes.clear(); free_list.clear(); root = -1; }

  private:

    int AllocateNode(){
      if (free_list.empty()){
        nodes.emplace_back();
        return static_cast<int>(nodes.size() - 1);
      }
      const int node_index = free_list.back();
      free_list.pop_back();
      nodes[node_index].parent = -1;
      nodes[node_index].left = -1;
      nodes[node_index].right = -1;
      nodes[node_index].height = 0;
      return node_index;
    }

    void FreeNode(int node_index){
      nodes[node_index].height = -1;
      free_list.push_back(node_index);
    }

    // Rotates the taller child up one level so the two sides stay within one in
    // height. Returns the new root of this subtree.
    int Balance(int index){
      BVHNode& node = nodes[index];
      if (IsLeaf(node) || node.height < 2) return index;

      const int left = node.left;
      const int right = node.right;
      const int balance = nodes[right].height - nodes[left].height;

      if (balance > 1){
        const int rl = nodes[right].left;
        const int rr = nodes[right].right;

        nodes[right].left = index;
        nodes[right].parent = node.parent;
        node.parent = right;

        if (nodes[right].parent != -1){
          if (nodes[nodes[right].parent].left == index)
            nodes[nodes[right].parent].left = right;
          else
            nodes[nodes[right].parent].right = right;
        } else {
          root = right;
        }

        if (nodes[rl].height > nodes[rr].height){
          nodes[right].right = rl;
          node.right = rr;
          nodes[rr].parent = index;
          node.box = Union(nodes[left].box, nodes[rr].box);
          nodes[right].box = Union(node.box, nodes[rl].box);
          node.height = 1 + std::max(nodes[left].height, nodes[rr].height);
          nodes[right].height = 1 + std::max(node.height, nodes[rl].height);
        } else {
          nodes[right].right = rr;
          node.right = rl;
          nodes[rl].parent = index;
          node.box = Union(nodes[left].box, nodes[rl].box);
          nodes[right].box = Union(node.box, nodes[rr].box);
          node.height = 1 + std::max(nodes[left].height, nodes[rl].height);
          nodes[right].height = 1 + std::max(node.height, nodes[rr].height);
        }
        return right;
      }

      if (balance < -1){
        const int ll = nodes[left].left;
        const int lr = nodes[left].right;

        nodes[left].left = index;
        nodes[left].parent = node.parent;
        node.parent = left;

        if (nodes[left].parent != -1){
          if (nodes[nodes[left].parent].left == index)
            nodes[nodes[left].parent].left = left;
          else
            nodes[nodes[left].parent].right = left;
        } else {
          root = left;
        }

        if (nodes[ll].height > nodes[lr].height){
          nodes[left].right = ll;
          node.left = lr;
          nodes[lr].parent = index;
          node.box = Union(nodes[right].box, nodes[lr].box);
          nodes[left].box = Union(node.box, nodes[ll].box);
          node.height = 1 + std::max(nodes[right].height, nodes[lr].height);
          nodes[left].height = 1 + std::max(node.height, nodes[ll].height);
        } else {
          nodes[left].right = lr;
          node.left = ll;
          nodes[ll].parent = index;
          node.box = Union(nodes[right].box, nodes[ll].box);
          nodes[left].box = Union(node.box, nodes[lr].box);
          node.height = 1 + std::max(nodes[right].height, nodes[ll].height);
          nodes[left].height = 1 + std::max(node.height, nodes[lr].height);
        }
        return left;
      }

      return index;
    }

    std::vector<BVHNode> nodes;
    std::vector<int> free_list;
    int root = -1;
  };

}
