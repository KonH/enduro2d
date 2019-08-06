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
    
    class property_block_value_visitor final : private noncopyable {
    public:
        property_block_value_visitor(debug& debug, opengl::uniform_info ui) noexcept
        : debug_(debug)
        , loc_(ui.location) {}

        void operator()(i32 v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform1i(loc_, v));
        }

        void operator()(f32 v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform1f(loc_, v));
        }

        void operator()(const v2i& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform2iv(loc_, 1, v.data()));
        }

        void operator()(const v3i& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform3iv(loc_, 1, v.data()));
        }

        void operator()(const v4i& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform4iv(loc_, 1, v.data()));
        }

        void operator()(const v2f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform2fv(loc_, 1, v.data()));
        }

        void operator()(const v3f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform3fv(loc_, 1, v.data()));
        }

        void operator()(const v4f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniform4fv(loc_, 1, v.data()));
        }

        void operator()(const m2f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniformMatrix2fv(loc_, 1, GL_TRUE, v.data()));
        }

        void operator()(const m3f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniformMatrix3fv(loc_, 1, GL_TRUE, v.data()));
        }

        void operator()(const m4f& v) const noexcept {
            GL_CHECK_CODE(debug_, glUniformMatrix4fv(loc_, 1, GL_TRUE, v.data()));
        }
    private:
        debug& debug_;
        GLuint loc_;
    };
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
        size_t off = math::align_ceil(offset, align);
        return off < content.size()
             ? math::align_ceil(content.size() - off, align)
             : 0;
    }

    bool batcher::material::operator == (const batcher::material& r) const noexcept {
        return shader_ == r.shader_
            && sampler_ == r.sampler_
            //&& properties_ == r.properties_   // TODO
            && blend_ == r.blend_;
    }
    
    bool batcher::blend_mode::operator == (const blend_mode& r) const noexcept {
        if ( !enabled_ ) {
            return !r.enabled_;
        }
        return src_rgb_factor_ == r.src_rgb_factor_
            && dst_rgb_factor_ == r.dst_rgb_factor_
            && rgb_equation_ == r.rgb_equation_
            && src_alpha_factor_ == r.src_alpha_factor_
            && dst_alpha_factor_ == r.dst_alpha_factor_
            && alpha_equation_ == r.alpha_equation_;
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
            vb.content.resize(vertex_buffer_size_);
        }

        if ( index_buffers_.empty() ||
             index_buffers_.back().available(index_stride_) < min_ib_size )
        {
            auto& ib = index_buffers_.emplace_back();
            ib.content.resize(index_buffer_size_);
        }
        
        result.vert_decl = vert_decl;
        result.topo = topo;
        result.vb_index = u32(vertex_buffers_.size()-1);
        result.ib_index = u32(index_buffers_.size()-1);
        result.idx_offset = index_buffers_.back().offset;

        return result;
    }

    void batcher::flush() {
        using namespace opengl;

        if ( !dirty_ ) {
            return;
        }
        dirty_ = false;

        using attrib_bits_t = std::bitset<8>;
        attrib_bits_t enabled_attribs;
        vert_decl_ptr curr_decl = nullptr;
        gl_buffer_id curr_vb(debug_);
        gl_buffer_id curr_ib(debug_);
        u32 curr_vb_index = ~0u;
        u32 curr_ib_index = ~0u;
        blend_mode curr_blend;
        GLuint curr_prog = 0;

        GL_CHECK_CODE(debug_, glDisable(GL_DEPTH_TEST));
        GL_CHECK_CODE(debug_, glDisable(GL_STENCIL_TEST));
        GL_CHECK_CODE(debug_, glDisable(GL_BLEND));

        for ( auto& batch : batches_ ) {
            // update buffers
            if ( curr_vb_index != batch.vb_index ) {
                curr_decl = nullptr;
                curr_vb_index = batch.vb_index;
                curr_vb = gl_buffer_id::create(debug_, GL_ARRAY_BUFFER);
                auto& buf = vertex_buffers_[batch.vb_index].content;
                GL_CHECK_CODE(debug_, glBindBuffer(
                    GL_ARRAY_BUFFER,
                    *curr_vb));
                GL_CHECK_CODE(debug_, glBufferData(
                    GL_ARRAY_BUFFER,
                    buf.size(),
                    buf.data(),
                    GL_STATIC_DRAW));
            }
            if ( curr_ib_index != batch.ib_index ) {
                curr_ib_index = batch.ib_index;
                curr_ib = gl_buffer_id::create(debug_, GL_ELEMENT_ARRAY_BUFFER);
                auto& buf = index_buffers_[batch.ib_index].content;
                GL_CHECK_CODE(debug_, glBindBuffer(
                    GL_ELEMENT_ARRAY_BUFFER,
                    *curr_ib));
                GL_CHECK_CODE(debug_, glBufferData(
                    GL_ELEMENT_ARRAY_BUFFER,
                    buf.size(),
                    buf.data(),
                    GL_STATIC_DRAW));
            }

            // update vertex attribs
            if ( curr_decl != batch.vert_decl ) {
                curr_decl = batch.vert_decl;
                attrib_bits_t curr_attribs;
                const GLsizei stride = math::numeric_cast<GLsizei>(math::align_ceil(batch.vert_decl->bytes_per_vertex(), vertex_stride_));
                for ( std::size_t i = 0, e = batch.vert_decl->attribute_count(); i < e; ++i ) {
                    const vertex_declaration::attribute_info& vai = batch.vert_decl->attribute(i);
                    batch.mtr.shader()->state().with_attribute_location(vai.name, [this, &vai, stride, &curr_attribs](const attribute_info& ai) noexcept {
                        const GLuint rows = math::numeric_cast<GLuint>(vai.rows);
                        for ( GLuint row = 0; row < rows; ++row ) {
                            auto index = math::numeric_cast<GLuint>(ai.location) + row;
                            curr_attribs[index] = true;
                            GL_CHECK_CODE(debug_, glEnableVertexAttribArray(index));
                            GL_CHECK_CODE(debug_, glVertexAttribPointer(
                                index,
                                math::numeric_cast<GLint>(vai.columns),
                                convert_attribute_type(vai.type),
                                vai.normalized ? GL_TRUE : GL_FALSE,
                                stride,
                                reinterpret_cast<const GLvoid*>(vai.stride + row * vai.row_size())));
                        }
                    });
                }
                for ( size_t i = 0; i < curr_attribs.size(); ++i ) {
                    if ( !curr_attribs[i] && enabled_attribs[i] ) {
                        GL_CHECK_CODE(debug_, glDisableVertexAttribArray(
                            math::numeric_cast<GLuint>(i)));
                    }
                }
                enabled_attribs = curr_attribs;
            }

            if ( batch.mtr.sampler().second.texture() ) {
                auto& samp = batch.mtr.sampler().second;
                auto& tex = batch.mtr.sampler().second.texture()->state().id();

                GL_CHECK_CODE(debug_, glActiveTexture(GL_TEXTURE0));
                GL_CHECK_CODE(debug_, glBindTexture(tex.target(), *tex));

                GL_CHECK_CODE(debug_, glTexParameteri(
                    tex.target(),
                    GL_TEXTURE_WRAP_S,
                    convert_sampler_wrap(samp.s_wrap())));
                GL_CHECK_CODE(debug_, glTexParameteri(
                    tex.target(),
                    GL_TEXTURE_WRAP_T,
                    convert_sampler_wrap(samp.t_wrap())));
                GL_CHECK_CODE(debug_, glTexParameteri(
                    tex.target(),
                    GL_TEXTURE_MIN_FILTER,
                    convert_sampler_filter(samp.min_filter())));
                GL_CHECK_CODE(debug_, glTexParameteri(
                    tex.target(),
                    GL_TEXTURE_MAG_FILTER,
                    convert_sampler_filter(samp.mag_filter())));
            }

            auto& sp = batch.mtr.shader()->state();
            if ( *sp.id() != curr_prog ) {
                curr_prog = *sp.id();
                GL_CHECK_CODE(debug_, glUseProgram(curr_prog));
            }

            batch.mtr.properties().foreach([this, &sp](str_hash name, const render::property_value& value) noexcept {
                sp.with_uniform_location(name, [this, &value](const uniform_info& ui) noexcept {
                    E2D_ASSERT(!value.valueless_by_exception());
                    stdex::visit(property_block_value_visitor(debug_, ui), value);
                });
            });
            
            auto& bs = batch.mtr.blend();
            if ( !(bs == curr_blend) ) {
                if ( bs.enabled() ) {
                    GL_CHECK_CODE(debug_, glEnable(GL_BLEND));
                    GL_CHECK_CODE(debug_, glBlendFuncSeparate(
                        convert_blending_factor(bs.src_rgb_factor()),
                        convert_blending_factor(bs.dst_rgb_factor()),
                        convert_blending_factor(bs.src_alpha_factor()),
                        convert_blending_factor(bs.dst_alpha_factor())));
                    GL_CHECK_CODE(debug_, glBlendEquationSeparate(
                        convert_blending_equation(bs.rgb_equation()),
                        convert_blending_equation(bs.alpha_equation())));
                } else {
                    GL_CHECK_CODE(debug_, glDisable(GL_BLEND));
                }
                curr_blend = bs;
            }

            GL_CHECK_CODE(debug_, glDrawElements(
                convert_topology(batch.topo),
                math::numeric_cast<GLsizei>(batch.idx_count),
                GL_UNSIGNED_SHORT,
                reinterpret_cast<const void*>(batch.idx_offset)));
        }

        vertex_buffers_.clear();
        index_buffers_.clear();
        batches_.clear();
    }
}
