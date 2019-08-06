/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/core/batcher.hpp>
#include "render_impl/render_opengl_base.hpp"
#include "render_impl/render_opengl_impl.hpp"

namespace 
{
    using namespace e2d;
    
    template < typename T >
    size_t hash_of(const T& x) noexcept {
        return std::hash<T>()(x);
    }
    
    size_t hash_of(str_hash x) noexcept {
        return x.hash();
    }

    size_t hash_of(const vertex_declaration::attribute_info& x) noexcept {
        size_t h = 0;
        h = utils::hash_combine(h, hash_of(x.stride));
        h = utils::hash_combine(h, hash_of(x.name));
        h = utils::hash_combine(h, hash_of(x.rows));
        h = utils::hash_combine(h, hash_of(x.columns));
        h = utils::hash_combine(h, hash_of(x.type));
        h = utils::hash_combine(h, hash_of(x.normalized));
        return h;
    }

    size_t hash_of(const vertex_declaration& x) noexcept {
        size_t h = hash_of(x.attribute_count());
        for ( size_t i = 0; i < x.attribute_count(); ++i ) {
            h = utils::hash_combine(h, hash_of(x.attribute(i)));
        }
        h = utils::hash_combine(h, hash_of(x.bytes_per_vertex()));
        return h;
    }
}

namespace e2d
{
    size_t batcher::vertex_declaration_hash::operator ()(const vertex_declaration& x) const noexcept {
        return hash_of(x);
    }

    bool batcher::vertex_declaration_equal::operator ()(
        const vertex_declaration& l,
        const vertex_declaration& r) const noexcept
    {
        // TODO
        return l.bytes_per_vertex() == r.bytes_per_vertex();
    }
    
    batcher::batch_::batch_(const material &mtr)
    : mtr(mtr) {}
    
    size_t batcher::buffer_::available(size_t align) const noexcept {
        return math::align_ceil(data.size() - offset, align );
    }

    bool operator == (const batcher::material& l, const batcher::material& r) noexcept {
        return l.shader_ == r.shader_
            && l.sampler_ == r.sampler_
            //&& l.properties_ == r.properties_
            && l.blend_ == r.blend_;
    }
    
    batcher::batcher(debug& d)
    : debug_(d) {}

    batcher::~batcher() noexcept = default;

    batcher::vert_decl_ptr batcher::cache_vert_decl_(const vertex_declaration& decl) {
        return unique_vert_decl_.insert(decl).first.operator->();
    }

    batcher::batch_& batcher::append_batch_(
        const material& mtr,
        topology topo,
        vert_decl_ptr vert_decl,
        size_t vert_stride,
        size_t min_vb_size,
        size_t min_ib_size)
    {
        // try to reuse last batch
        if ( batches_.size() ) {
            auto& last = batches_.back();
            E2D_ASSERT(last.vb_index < vertex_buffers_.size());
            E2D_ASSERT(last.ib_index < index_buffers_.size());
            auto& vb = vertex_buffers_[last.vb_index];
            auto& ib = index_buffers_[last.ib_index];

            if ( last.mtr == mtr &&
                 last.vert_decl == vert_decl &&
                 last.topo == topo &&
                 vb.available(vert_stride) >= min_vb_size &&
                 ib.available(index_stride_) >= min_ib_size )
            {
                return last;
            }
        }

        // create new batch
        batch_& result = batches_.emplace_back(mtr);

        if ( vertex_buffers_.empty() ||
             vertex_buffers_.back().available(vert_stride) < min_vb_size )
        {
            auto& vb = vertex_buffers_.emplace_back();
            vb.data.resize(vertex_buffer_size_);
        }

        if ( index_buffers_.empty() ||
             index_buffers_.back().available(index_stride_) < min_ib_size )
        {
            auto& ib = index_buffers_.emplace_back();
            ib.data.resize(index_buffer_size_);
        }
        
        result.vert_decl = vert_decl;
        result.topo = topo;
        result.vb_index = u32(vertex_buffers_.size()-1);
        result.ib_index = u32(index_buffers_.size()-1);
        result.vert_offset = vertex_buffers_.back().offset;
        result.idx_offset = index_buffers_.back().offset;

        return result;
    }
    
    void batcher::begin(const render_target_ptr& rt) {
        render_target_ = rt;
    }

    void batcher::flush() {
        using namespace opengl;

        //std::bitset<16> enabled_attribs;
        vert_decl_ptr curr_decl = nullptr;
        gl_buffer_id curr_vb(debug_);
        gl_buffer_id curr_ib(debug_);
        u32 curr_vb_index = ~0u;
        u32 curr_ib_index = ~0u;

        for ( auto& batch : batches_ ) {
            // update buffers
            if ( curr_vb_index != batch.vb_index ) {
                curr_decl = nullptr;
                curr_vb_index = batch.vb_index;
                curr_vb = gl_buffer_id::create(debug_, GL_ARRAY_BUFFER);
                with_gl_bind_buffer(debug_, curr_vb, [this, &batch]() noexcept {
                    E2D_ASSERT(batch.vb_index < vertex_buffers_.size());
                    auto& buf = vertex_buffers_[batch.vb_index].data;
                    GL_CHECK_CODE(debug_, glBufferData(
                        GL_ARRAY_BUFFER,
                        buf.size(),
                        buf.data(),
                        GL_STATIC_DRAW));
                });
            }
            if ( curr_ib_index != batch.ib_index ) {
                curr_ib_index = batch.ib_index;
                curr_ib = gl_buffer_id::create(debug_, GL_ELEMENT_ARRAY_BUFFER);
                with_gl_bind_buffer(debug_, curr_ib, [this, &batch]() noexcept {
                    E2D_ASSERT(batch.ib_index < index_buffers_.size());
                    auto& buf = index_buffers_[batch.ib_index].data;
                    GL_CHECK_CODE(debug_, glBufferData(
                        GL_ELEMENT_ARRAY_BUFFER,
                        buf.size(),
                        buf.data(),
                        GL_STATIC_DRAW));
                });
            }

            // update vertex attribs
            if ( curr_decl != batch.vert_decl ) {
                curr_decl = batch.vert_decl;
                with_gl_bind_buffer(debug_, curr_vb, [this, &batch]() noexcept {
                    const GLsizei stride = math::numeric_cast<GLsizei>(math::align_ceil(batch.vert_decl->bytes_per_vertex(), vertex_stride_));
                    for ( std::size_t i = 0, e = batch.vert_decl->attribute_count(); i < e; ++i ) {
                        const vertex_declaration::attribute_info& vai = batch.vert_decl->attribute(i);
                        batch.mtr.shader_->state().with_attribute_location(vai.name, [this, &vai, stride](const attribute_info& ai) noexcept {
                            const GLuint rows = math::numeric_cast<GLuint>(vai.rows);
                            for ( GLuint row = 0; row < rows; ++row ) {
                                GL_CHECK_CODE(debug_, glEnableVertexAttribArray(
                                    math::numeric_cast<GLuint>(ai.location) + row));
                                GL_CHECK_CODE(debug_, glVertexAttribPointer(
                                    math::numeric_cast<GLuint>(ai.location) + row,
                                    math::numeric_cast<GLint>(vai.columns),
                                    convert_attribute_type(vai.type),
                                    vai.normalized ? GL_TRUE : GL_FALSE,
                                    stride,
                                    reinterpret_cast<const GLvoid*>(vai.stride + row * vai.row_size())));
                            }
                        });
                    }
                });
            }
            
            GL_CHECK_CODE(debug_, glDrawElements(
                convert_topology(batch.topo),
                batch.idx_count,
                GL_UNSIGNED_SHORT,
                reinterpret_cast<const void*>(batch.idx_offset)));
        }

        vertex_buffers_.clear();
        index_buffers_.clear();
        batches_.clear();
    }
}
