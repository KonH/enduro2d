/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_high.hpp"

#include "gobject.hpp"

namespace e2d
{
    class node;
    using node_iptr = intrusive_ptr<node>;
    using const_node_iptr = intrusive_ptr<const node>;

    class node_children_ilist_tag {};
    using node_children = intrusive_list<node, node_children_ilist_tag>;
}

namespace e2d
{
    class node
        : private noncopyable
        , public ref_counter<node>
        , public intrusive_list_hook<node_children_ilist_tag> {
    public:
        virtual ~node() noexcept;

        [[nodiscard]] static node_iptr create();
        [[nodiscard]] static node_iptr create(const node_iptr& parent);

        [[nodiscard]] static node_iptr create(const gobject_iptr& owner);
        [[nodiscard]] static node_iptr create(const gobject_iptr& owner, const node_iptr& parent);

        void owner(const gobject_iptr& owner) noexcept;

        [[nodiscard]] gobject_iptr owner() noexcept;
        [[nodiscard]] const_gobject_iptr owner() const noexcept;

        void transform(const t3f& transform) noexcept;
        [[nodiscard]] const t3f& transform() const noexcept;

        void translation(const v3f& translation) noexcept;
        [[nodiscard]] const v3f& translation() const noexcept;

        void rotation(const q4f& rotation) noexcept;
        [[nodiscard]] const q4f& rotation() const noexcept;

        void scale(const v3f& scale) noexcept;
        [[nodiscard]] const v3f& scale() const noexcept;

        [[nodiscard]] const m4f& local_matrix() const noexcept;
        [[nodiscard]] const m4f& world_matrix() const noexcept;

        [[nodiscard]] node_iptr root() noexcept;
        [[nodiscard]] const_node_iptr root() const noexcept;

        [[nodiscard]] node_iptr parent() noexcept;
        [[nodiscard]] const_node_iptr parent() const noexcept;

        [[nodiscard]] bool has_parent() const noexcept;
        [[nodiscard]] bool has_parent_recursive(
            const const_node_iptr& parent) const noexcept;

        [[nodiscard]] bool has_children() const noexcept;
        [[nodiscard]] bool has_child_recursive(
            const const_node_iptr& child) const noexcept;

        bool remove_from_parent() noexcept;
        std::size_t remove_all_children() noexcept;

        [[nodiscard]] std::size_t child_count() const noexcept;
        [[nodiscard]] std::size_t child_count_recursive() const noexcept;

        bool add_child(
            const node_iptr& child) noexcept;

        bool add_child_to_back(
            const node_iptr& child) noexcept;

        bool add_child_to_front(
            const node_iptr& child) noexcept;

        bool add_child_before(
            const node_iptr& before,
            const node_iptr& child) noexcept;

        bool add_child_after(
            const node_iptr& after,
            const node_iptr& child) noexcept;

        bool add_sibling_before(
            const node_iptr& sibling) noexcept;

        bool add_sibling_after(
            const node_iptr& sibling) noexcept;

        bool remove_child(
            const node_iptr& child) noexcept;

        bool send_backward() noexcept;
        bool bring_to_back() noexcept;

        bool send_forward() noexcept;
        bool bring_to_front() noexcept;

        [[nodiscard]] node_iptr first_child() noexcept;
        [[nodiscard]] const_node_iptr first_child() const noexcept;

        [[nodiscard]] node_iptr last_child() noexcept;
        [[nodiscard]] const_node_iptr last_child() const noexcept;

        [[nodiscard]] node_iptr prev_sibling() noexcept;
        [[nodiscard]] const_node_iptr prev_sibling() const noexcept;

        [[nodiscard]] node_iptr next_sibling() noexcept;
        [[nodiscard]] const_node_iptr next_sibling() const noexcept;

        template < typename F >
        void for_each_child(F&& f);

        template < typename F >
        void for_each_child(F&& f) const;

        template < typename Iter >
        std::size_t extract_all_nodes(Iter iter);

        template < typename Iter >
        std::size_t extract_all_nodes(Iter iter) const;
    protected:
        node() = default;
        node(const gobject_iptr& owner);
    private:
        enum flag_masks : u32 {
            fm_dirty_local_matrix = 1u << 0,
            fm_dirty_world_matrix = 1u << 1,
        };
        void mark_dirty_local_matrix_() noexcept;
        void mark_dirty_world_matrix_() noexcept;
        void update_local_matrix_() const noexcept;
        void update_world_matrix_() const noexcept;
    private:
        t3f transform_;
        gobject_iptr owner_;
        node* parent_{nullptr};
        node_children children_;
    private:
        mutable u32 flags_{0u};
        mutable m4f local_matrix_;
        mutable m4f world_matrix_;
    };
}

#include "node.inl"
