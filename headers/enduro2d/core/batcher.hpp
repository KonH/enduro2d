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
        using batch_index_t = u16;
        using topology = render::topology;
        using blending_color_mask = render::blending_color_mask;
        using blending_factor = render::blending_factor;
        using blending_equation = render::blending_equation;

        template < typename T >
        class vertex_iterator final {
        public:
            vertex_iterator() = default;
            vertex_iterator(u8* data, size_t size, size_t stride);
            void operator = (const T& r) noexcept;
            vertex_iterator<T>& operator ++() noexcept;
            [[nodiscard]] vertex_iterator<T> operator ++(int) noexcept;
            [[nodiscard]] T& operator [](u32 index) const noexcept;
            [[nodiscard]] size_t size() const noexcept;
        private:
            u8* data_ = nullptr;
            size_t size_ = 0;
            size_t stride_ = 0;
        };

        class index_iterator final {
        public:
            index_iterator() = default;
            index_iterator(u8* data, size_t size, batch_index_t offset);
            void operator = (const batch_index_t& r) noexcept;
            index_iterator& operator ++() noexcept;
            [[nodiscard]] index_iterator operator ++(int) noexcept;
            [[nodiscard]] size_t size() const noexcept;
        private:
            batch_index_t* indices_ = nullptr;
            size_t size_ = 0;
            batch_index_t offset_ = 0;
        };

        template < typename VertexType >
        class rectangle_batch {
        public:
            using vertex_type = VertexType;
        public:
            rectangle_batch() = default;
            rectangle_batch(const b2f& pos, const b2f& uv, color32 col);
            void get_indices(index_iterator iter) const noexcept;
            void get_vertices(vertex_iterator<VertexType> iter) const noexcept;
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
            void get_indices(index_iterator iter) const noexcept;
            void get_vertices(vertex_iterator<VertexType> iter) const noexcept;
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
            void get_indices(index_iterator iter) const noexcept;
            void get_vertices(vertex_iterator<VertexType> iter) const noexcept;
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
        class blend_mode final {
        public:
            blend_mode() = default;

            blend_mode& factor(blending_factor src, blending_factor dst) noexcept;
            blend_mode& src_factor(blending_factor src) noexcept;
            blend_mode& dst_factor(blending_factor dst) noexcept;

            blend_mode& rgb_factor(blending_factor src, blending_factor dst) noexcept;
            blend_mode& src_rgb_factor(blending_factor src) noexcept;
            blend_mode& dst_rgb_factor(blending_factor dst) noexcept;

            blend_mode& alpha_factor(blending_factor src, blending_factor dst) noexcept;
            blend_mode& src_alpha_factor(blending_factor src) noexcept;
            blend_mode& dst_alpha_factor(blending_factor dst) noexcept;

            blend_mode& equation(blending_equation equation) noexcept;
            blend_mode& rgb_equation(blending_equation equation) noexcept;
            blend_mode& alpha_equation(blending_equation equation) noexcept;

            blend_mode& enable(bool value) noexcept;

            [[nodiscard]] blending_factor src_rgb_factor() const noexcept;
            [[nodiscard]] blending_factor dst_rgb_factor() const noexcept;

            [[nodiscard]] blending_factor src_alpha_factor() const noexcept;
            [[nodiscard]] blending_factor dst_alpha_factor() const noexcept;

            [[nodiscard]] blending_equation rgb_equation() const noexcept;
            [[nodiscard]] blending_equation alpha_equation() const noexcept;

            [[nodiscard]] bool enabled() const noexcept;

            [[nodiscard]] bool operator == (const blend_mode& r) const noexcept;
        private:
            blending_factor src_rgb_factor_ = blending_factor::one;
            blending_factor dst_rgb_factor_ = blending_factor::zero;
            blending_equation rgb_equation_ = blending_equation::add;
            blending_factor src_alpha_factor_ = blending_factor::one;
            blending_factor dst_alpha_factor_ = blending_factor::zero;
            blending_equation alpha_equation_ = blending_equation::add;
            bool enabled_ = false;
        };

        using sampler_state = render::sampler_state;

        class material final {
        public:
            using sampler_state_t = std::pair<str_hash, render::sampler_state>;
            using properties_t = render::property_map<render::property_value>;
        public:
            material() = default;

            template < typename T >
            material& property(str_hash name, T&& v);
            material& shader(const shader_ptr& value) noexcept;
            material& sampler(str_view name, const sampler_state& value) noexcept;
            material& blend(const blend_mode& value) noexcept;

            [[nodiscard]] const shader_ptr& shader() const noexcept;
            [[nodiscard]] const sampler_state_t& sampler() const noexcept;
            [[nodiscard]] const properties_t& properties() const noexcept;
            [[nodiscard]] const blend_mode& blend() const noexcept;

            [[nodiscard]] bool operator == (const material& r) const noexcept;
        private:
            shader_ptr shader_;
            sampler_state_t sampler_;
            blend_mode blend_;
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
            //size_t vert_offset = 0;
            size_t idx_offset = 0;
            size_t vert_count = 0;
            size_t idx_count = 0;
            u32 vb_index = ~0u;
            u32 ib_index = ~0u;
            vert_decl_ptr vert_decl = nullptr;
        };

        class buffer_ final {
        public:
            size_t available(size_t align) const noexcept;
        public:
            buffer content;
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
        
        template < typename VertexType >
        struct allocated_batch {
            vertex_iterator<VertexType> vertices;
            index_iterator indices;
        };
        template < typename VertexType >
        [[nodiscard]] allocated_batch<VertexType> alloc_batch(
            size_t vertex_count,
            size_t index_count,
            topology topo,
            const material& mtr);

        void flush();

    private:
        template < typename BatchType >
        void break_strip_(
            const BatchType& src,
            vertex_iterator<typename BatchType::vertex_type> vert_iter,
            index_iterator idx_iter) const noexcept;
        
        template < typename BatchType >
        void continue_list_(
            const BatchType& src,
            vertex_iterator<typename BatchType::vertex_type> vert_iter,
            index_iterator idx_iter) const noexcept;

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
    // batcher::vertex_iterator
    //

    template < typename T >
    inline batcher::vertex_iterator<T>::vertex_iterator(u8* data, size_t size, size_t stride)
    : data_(data)
    , size_(size)
    , stride_(stride) {}
    
    template < typename T >
    inline T& batcher::vertex_iterator<T>::operator [](u32 index) const noexcept {
        E2D_ASSERT(index * stride_ < size_);
        return *reinterpret_cast<T*>(data_ + (index * stride_));
    }
    
    template < typename T >
    inline void batcher::vertex_iterator<T>::operator = (const T& r) noexcept {
        operator[](0) = r;
    }

    template < typename T >
    inline batcher::vertex_iterator<T>& batcher::vertex_iterator<T>::operator ++() noexcept {
        E2D_ASSERT(size_ >= stride_);
        size_ -= stride_;
        data_ += stride_;
        return *this;
    }
    
    template < typename T >
    inline batcher::vertex_iterator<T> batcher::vertex_iterator<T>::operator ++(int) noexcept {
        auto result = *this;
        operator++();
        return result;
    }
    
    template < typename T >
    inline size_t batcher::vertex_iterator<T>::size() const noexcept {
        return size_ / stride_;
    }

    //
    // batcher::index_iterator
    //

    inline batcher::index_iterator::index_iterator(u8* data, size_t size, batch_index_t offset)
    : indices_(reinterpret_cast<batch_index_t*>(data))
    , size_(size / sizeof(batch_index_t))
    , offset_(offset) {}

    inline void batcher::index_iterator::operator = (const batch_index_t& r) noexcept {
        E2D_ASSERT(size_ > 0);
        *indices_ = r + offset_;
    }

    inline batcher::index_iterator& batcher::index_iterator::operator ++() noexcept {
        E2D_ASSERT(size_ > 0);
        --size_;
        ++indices_;
        return *this;
    }

    inline batcher::index_iterator batcher::index_iterator::operator ++(int) noexcept {
        auto result = *this;
        operator++();
        return result;
    }
    
    inline size_t batcher::index_iterator::size() const noexcept {
        return size_;
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
        index_iterator iter) const noexcept
    {
        iter   = 0;  ++iter = 1;  ++iter = 2;
        ++iter = 1;  ++iter = 2;  ++iter = 3;
    }
    
    template < typename VertexType >
    void batcher::rectangle_batch<VertexType>::get_vertices(
        vertex_iterator<VertexType> iter) const noexcept
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
    // batcher::blend_mode
    //

    inline batcher::blend_mode& batcher::blend_mode::factor(blending_factor src, blending_factor dst) noexcept {
        rgb_factor(src, dst);
        alpha_factor(src, dst);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::src_factor(blending_factor src) noexcept {
        src_rgb_factor(src);
        src_alpha_factor(src);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::dst_factor(blending_factor dst) noexcept {
        dst_rgb_factor(dst);
        dst_alpha_factor(dst);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::rgb_factor(blending_factor src, blending_factor dst) noexcept {
        src_rgb_factor(src);
        dst_rgb_factor(dst);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::src_rgb_factor(blending_factor src) noexcept {
        src_rgb_factor_ = src;
        enabled_ = true;
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::dst_rgb_factor(blending_factor dst) noexcept {
        dst_rgb_factor_ = dst;
        enabled_ = true;
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::alpha_factor(blending_factor src, blending_factor dst) noexcept {
        src_alpha_factor(src);
        dst_alpha_factor(dst);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::src_alpha_factor(blending_factor src) noexcept {
        src_alpha_factor_ = src;
        enabled_ = true;
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::dst_alpha_factor(blending_factor dst) noexcept {
        dst_alpha_factor_ = dst;
        enabled_ = true;
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::equation(blending_equation equation) noexcept {
        rgb_equation(equation);
        alpha_equation(equation);
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::rgb_equation(blending_equation equation) noexcept {
        rgb_equation_ = equation;
        enabled_ = true;
        return *this;
    }

    inline batcher::blend_mode& batcher::blend_mode::alpha_equation(blending_equation equation) noexcept {
        alpha_equation_ = equation;
        enabled_ = true;
        return *this;
    }
    
    inline batcher::blend_mode& batcher::blend_mode::enable(bool value) noexcept {
        enabled_ = value;
        return *this;
    }

    inline batcher::blending_factor batcher::blend_mode::src_rgb_factor() const noexcept {
        return src_rgb_factor_;
    }

    inline batcher::blending_factor batcher::blend_mode::dst_rgb_factor() const noexcept {
        return dst_rgb_factor_;
    }

    inline batcher::blending_factor batcher::blend_mode::src_alpha_factor() const noexcept {
        return src_alpha_factor_;
    }

    inline batcher::blending_factor batcher::blend_mode::dst_alpha_factor() const noexcept {
        return dst_alpha_factor_;
    }

    inline batcher::blending_equation batcher::blend_mode::rgb_equation() const noexcept {
        return rgb_equation_;
    }

    inline batcher::blending_equation batcher::blend_mode::alpha_equation() const noexcept {
        return alpha_equation_;
    }
    
    inline bool batcher::blend_mode::enabled() const noexcept {
        return enabled_;
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
    inline batcher::material& batcher::material::property(str_hash name, T&& v) {
        properties_.assign(name, v);
        return *this;
    }
    
    inline batcher::material& batcher::material::blend(const blend_mode& value) noexcept {
        blend_ = value;
        return *this;
    }
    
    inline const shader_ptr& batcher::material::shader() const noexcept {
        return shader_;
    }

    inline const batcher::material::sampler_state_t& batcher::material::sampler() const noexcept {
        return sampler_;
    }

    inline const batcher::material::properties_t& batcher::material::properties() const noexcept {
        return properties_;
    }

    inline const batcher::blend_mode& batcher::material::blend() const noexcept {
        return blend_;
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

        auto& vb = vertex_buffers_[dst_batch.vb_index];
        auto& ib = index_buffers_[dst_batch.ib_index];
        vb.offset = math::align_ceil(vb.offset, vert_stride);
        
        batch_index_t idx_offset = math::numeric_cast<batch_index_t>((vb.offset + vert_stride-1) / vert_stride);
        auto vert_iter = vertex_iterator<typename BatchType::vertex_type>(vb.content.data() + vb.offset, vb_size, vert_stride);
        auto idx_iter = index_iterator(ib.content.data() + ib.offset, ib_size, idx_offset);

        if ( is_strip ) {
            break_strip_(src_batch, vert_iter, idx_iter);
        } else {
            continue_list_(src_batch, vert_iter, idx_iter);
        }
        
        vb.offset += vb_size;
        ib.offset += ib_size;
        dst_batch.vert_count += src_batch.vertex_count();
        dst_batch.idx_count += src_batch.index_count() + (is_strip ? 2 : 0);
        dirty_ = true;
    }
    
    template < typename VertexType >
    batcher::allocated_batch<VertexType> batcher::alloc_batch(
        size_t vertex_count,
        size_t index_count,
        topology topo,
        const material& mtr)
    {
        const size_t vert_stride = math::align_ceil(sizeof(VertexType), vertex_stride_);
        const size_t vb_size = vertex_count * vert_stride;
        const size_t ib_size = index_count * index_stride_;
        const vert_decl_ptr vert_decl = cache_vert_decl_(VertexType::decl());
        batch_& dst_batch = append_batch_(mtr, topo, vert_decl, vert_stride, vb_size, ib_size);
        
        auto& vb = vertex_buffers_[dst_batch.vb_index];
        auto& ib = index_buffers_[dst_batch.ib_index];
        vb.offset = math::align_ceil(vb.offset, vert_stride);

        allocated_batch<VertexType> result;
        batch_index_t idx_offset = math::numeric_cast<batch_index_t>((vb.offset + vert_stride-1) / vert_stride);
        result.vertices = vertex_iterator<VertexType>(vb.content.data() + vb.offset, vb_size, vert_stride);
        result.indices = index_iterator(ib.content.data() + ib.offset, ib_size, idx_offset);
        
        vb.offset += vb_size;
        ib.offset += ib_size;
        dst_batch.vert_count += vertex_count;
        dst_batch.idx_count += index_count;
        dirty_ = true;

        return result;
    }

    template < typename BatchType >
    void batcher::break_strip_(
        const BatchType& src,
        vertex_iterator<typename BatchType::vertex_type> vert_iter,
        index_iterator idx_iter) const noexcept
    {
        E2D_ASSERT(false);
    }
        
    template < typename BatchType >
    void batcher::continue_list_(
        const BatchType& src,
        vertex_iterator<typename BatchType::vertex_type> vert_iter,
        index_iterator idx_iter) const noexcept
    {
        src.get_vertices(vert_iter);
        src.get_indices(idx_iter);
    }
}