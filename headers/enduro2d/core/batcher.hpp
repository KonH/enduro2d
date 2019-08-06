/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "render.hpp"

namespace e2d
{

    class batcher final : public module<batcher> {
    public:
        template < typename T >
        class iterator final {
        public:
            iterator(void* data, size_t size, size_t stride);
            T& operator [](u32 index) const noexcept;
            void operator = (const T& r) noexcept;
            iterator<T>& operator ++() noexcept;
        private:
            u8* data_;
            size_t size_;
            const size_t stride_;
        };

        using batch_index_t = u16;
        using topology = render::topology;

        template < typename VertexType >
        class rectangle_batch {
        public:
            using vertex_type = VertexType;
        public:
            rectangle_batch() = default;
            rectangle_batch(const b2f& pos, const b2f& uv, color32 col);
            void get_indices(iterator<batch_index_t> iter, batch_index_t offset) const noexcept;
            void get_vertices(iterator<VertexType> iter) const noexcept;
            static topology topology() noexcept;
            static u32 index_count() noexcept;
            static u32 vertex_count() noexcept;
        public:
            b2f pos;
            b2f uv;
            color32 col;
        };

        template < typename VertexType >
        class nine_patch_batch {
        public:
            using vertex_type = VertexType;
        public:
            void get_indices(iterator<batch_index_t> iter, batch_index_t offset) const noexcept;
            void get_vertices(iterator<VertexType> iter) const noexcept;
            static topology topology() noexcept;
            static u32 index_count() noexcept;
            static u32 vertex_count() noexcept;
        public:
            color32 col;
        };
        
        template < typename VertexType >
        class circle_batch {
        public:
            using vertex_type = VertexType;
        public:
            circle_batch() = default;
            circle_batch(const b2f& pos, const b2f& uv, color32 col, u32 segments);
            void get_indices(iterator<batch_index_t> iter, batch_index_t offset) const noexcept;
            void get_vertices(iterator<VertexType> iter) const noexcept;
            static topology topology() noexcept;
            static u32 index_count() noexcept;
            static u32 vertex_count() noexcept;
        public:
            b2f pos;
            b2f uv;
            u32 segment_count = 8;
            color32 col;
        };

    public:
        enum class blend_mode : u8 {
            disabled,
            additive,
            additive_pma,
        };

        using sampler_state = render::sampler_state;

        class material final {
        public:
            material() = default;
            material& shader(const shader_ptr& value) noexcept;
            material& sampler(str_view name, const sampler_state& value) noexcept;
            template < typename T >
            material& property(str_hash name, T&& v);
        public:
            using sampler_state_t = std::pair<str_hash, render::sampler_state>;
            using properties_t = render::property_map<render::property_value>;
            shader_ptr shader_;
            sampler_state_t sampler_;
            blend_mode blend_ = blend_mode::disabled;
            properties_t properties_; // TODO: optimize
        };

    private:
        struct vertex_declaration_hash {
            size_t operator ()(const vertex_declaration& x) const noexcept;
        };

        struct vertex_declaration_equal {
            bool operator ()(const vertex_declaration& l, const vertex_declaration& r) const noexcept;
        };

        using unique_vert_decl_t = std::unordered_set<
            vertex_declaration,
            vertex_declaration_hash,
            vertex_declaration_equal>;
        using vert_decl_ptr = const vertex_declaration*;

        class batch_ final {
        public:
            batch_(const material &mtr);
        public:
            material mtr;
            topology topo;
            size_t vert_offset = 0;
            size_t idx_offset = 0;
            u32 vert_count = 0;
            u32 idx_count = 0;
            u32 vb_index = ~0u;
            u32 ib_index = ~0u;
            vert_decl_ptr vert_decl = nullptr;
        };

        class buffer_ final {
        public:
            size_t available(size_t align) const noexcept;
        public:
            buffer data;
            size_t offset;
        };

        static constexpr size_t vertex_stride_ = 16;
        static constexpr size_t index_stride_ = sizeof(batch_index_t);
        static constexpr size_t max_vertex_count_ = 1u << 15;
        static constexpr size_t vertex_buffer_size_ = max_vertex_count_ * vertex_stride_;
        static constexpr size_t index_buffer_size_ = max_vertex_count_ * 3 * index_stride_;

    public:
        batcher(debug& d);
        ~batcher() noexcept final;

        template < typename BatchType >
        void add_batch(const material& mtr, const BatchType& batch);

        void begin(const render_target_ptr& rt);
        void flush();

    private:
        template < typename BatchType >
        void break_strip_(
            const BatchType& src,
            batch_& dst,
            iterator<typename BatchType::vertex_type> vert_iter,
            iterator<batch_index_t> idx_iter) const;
        
        template < typename BatchType >
        void continue_list_(
            const BatchType& src,
            batch_& dst,
            iterator<typename BatchType::vertex_type> vert_iter,
            iterator<batch_index_t> idx_iter) const;

        batch_& append_batch_(
            const material& mtr,
            topology topo,
            vert_decl_ptr vert_decl,
            size_t vert_stride,
            size_t min_vb_size,
            size_t min_ib_size);

        vert_decl_ptr cache_vert_decl_(const vertex_declaration& decl);

    private:
        debug& debug_;
        render_target_ptr render_target_;
        std::vector<batch_> batches_;
        std::vector<buffer_> vertex_buffers_;
        std::vector<buffer_> index_buffers_;
        unique_vert_decl_t unique_vert_decl_;
        bool dirty_ = false;
    };

}

namespace e2d
{
    //
    // batcher::iterator
    //

    template < typename T >
    batcher::iterator<T>::iterator(void* data, size_t size, size_t stride)
    : data_(static_cast<u8*>(data)), size_(size), stride_(stride) {}
    
    template < typename T >
    T& batcher::iterator<T>::operator [](u32 index) const noexcept {
        E2D_ASSERT(index * stride_ < size_);
        return *reinterpret_cast<T*>(data_ + (index * stride_));
    }
    
    template < typename T >
    void batcher::iterator<T>::operator = (const T& r) noexcept {
        operator[](0) = r;
    }

    template < typename T >
    batcher::iterator<T>& batcher::iterator<T>::operator ++() noexcept {
        E2D_ASSERT(size_ >= stride_);
        size_ -= stride_;
        data_ -= stride_;
        return *this;
    }
    
    //
    // batcher::rectangle_batch
    //
    
    template < typename VertexType >
    batcher::rectangle_batch<VertexType>::rectangle_batch(
        const b2f& pos,
        const b2f& uv,
        color32 col)
    : pos(pos)
    , uv(uv)
    , col(col) {}

    template < typename VertexType >
    void batcher::rectangle_batch<VertexType>::get_indices(
        iterator<batch_index_t> iter,
        batch_index_t offset) const noexcept
    {
        iter   = offset + 0;
        ++iter = offset + 1;
        ++iter = offset + 2;
        ++iter = offset + 1;
        ++iter = offset + 2;
        ++iter = offset + 3;
    }
    
    template < typename VertexType >
    void batcher::rectangle_batch<VertexType>::get_vertices(
        iterator<VertexType> iter) const noexcept
    {
        iter   = VertexType(pos.position,                         uv.position,                        col);
        ++iter = VertexType(pos.position + v2f(0.0f, pos.size.y), uv.position + v2f(0.0f, uv.size.y), col);
        ++iter = VertexType(pos.position + v2f(pos.size.x, 0.0f), uv.position + v2f(uv.size.x, 0.0f), col);
        ++iter = VertexType(pos.position + pos.size,              uv.position + uv.size,              col);
    }
    
    template < typename VertexType >
    batcher::topology batcher::rectangle_batch<VertexType>::topology() noexcept {
        return topology::triangles;
    }
    
    template < typename VertexType >
    u32 batcher::rectangle_batch<VertexType>::index_count() noexcept {
        return 6;
    }
    
    template < typename VertexType >
    u32 batcher::rectangle_batch<VertexType>::vertex_count() noexcept {
        return 4;
    }

    //
    // batcher::material
    //
    
    inline batcher::material& batcher::material::shader(const shader_ptr& value) noexcept {
        shader_ = value;
        return *this;
    }
    
    inline batcher::material& batcher::material::sampler(
        str_view name,
        const sampler_state& value) noexcept
    {
        sampler_.first = name;
        sampler_.second = value;
        return *this;
    }
    
    template < typename T >
    batcher::material& batcher::material::property(str_hash name, T&& v) {
        properties_.assign(name, v);
        return *this;
    }

    //
    // batcher
    //

    template < typename BatchType >
    void batcher::add_batch(const material& mtr, const BatchType& src_batch) {
        const bool is_strip = src_batch.topology() != topology::triangles;
        const size_t vert_stride = math::align_ceil(sizeof(typename BatchType::vertex_type), vertex_stride_);
        const size_t vb_size = src_batch.vertex_count() * vert_stride;
        const size_t ib_size = src_batch.index_count() * index_stride_ + (is_strip ? 2 : 0);
        const vert_decl_ptr vert_decl = cache_vert_decl_(BatchType::vertex_type::decl());
        batch_& dst_batch = append_batch_(mtr, src_batch.topology(), vert_decl, vert_stride, vb_size, ib_size);

        E2D_ASSERT(dst_batch.vb_index < vertex_buffers_.size());
        E2D_ASSERT(dst_batch.ib_index < index_buffers_.size());
        auto& vb = vertex_buffers_[dst_batch.vb_index];
        auto& ib = index_buffers_[dst_batch.ib_index];

        auto vert_iter = iterator<typename BatchType::vertex_type>(vb.data.data(), vb_size, vert_stride);
        auto idx_iter = iterator<batch_index_t>(ib.data.data(), ib_size, index_stride_);

        /*if ( is_strip ) {
            break_strip_(src_batch, dst_batch, vert_iter, idx_iter);
        } else {
            continue_list_(src_batch, dst_batch, vert_iter, idx_iter);
        }*/

        vb.offset += vb_size;
        ib.offset += ib_size;
        dst_batch.vert_count += src_batch.vertex_count();
        dst_batch.idx_count += src_batch.index_count();
        dirty_ = true;
    }

    template < typename BatchType >
    void batcher::break_strip_(
        const BatchType& src,
        batch_& dst,
        iterator<typename BatchType::vertex_type> vert_iter,
        iterator<batch_index_t> idx_iter) const
    {
        E2D_ASSERT(false);
    }
        
    template < typename BatchType >
    void batcher::continue_list_(
        const BatchType& src,
        batch_& dst,
        iterator<typename BatchType::vertex_type> vert_iter,
        iterator<batch_index_t> idx_iter) const
    {
        src.get_vertices(vert_iter);
        src.get_indices(idx_iter, batch_index_t(dst.vert_count));
    }
}
