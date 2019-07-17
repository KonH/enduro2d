/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "_utils.hpp"
using namespace e2d;

TEST_CASE("mesh") {
    {
        mesh m;
        REQUIRE(meshes::try_load_mesh(
            m,
            the<vfs>().read(url("resources", "bin/gnome/gnome.obj.gnome.e2d_mesh"))));

        REQUIRE(m.vertices().size() == 397);

        REQUIRE(m.indices_submesh_count() == 1);
        REQUIRE(m.indices(0).size() == 2028);

        REQUIRE(m.uvs_channel_count() == 1);
        REQUIRE(m.uvs(0).size() == 397);

        REQUIRE(m.colors_channel_count() == 0);
        REQUIRE(m.normals().size() == 397);
        REQUIRE(m.tangents().empty());
        REQUIRE(m.bitangents().empty());
    }
    {
        mesh m;
        REQUIRE(meshes::try_load_mesh(
            m,
            the<vfs>().read(url("resources", "bin/gnome/gnome.obj.yad.e2d_mesh"))));

        REQUIRE(m.vertices().size() == 46);

        REQUIRE(m.indices_submesh_count() == 1);
        REQUIRE(m.indices(0).size() == 264);

        REQUIRE(m.uvs_channel_count() == 1);
        REQUIRE(m.uvs(0).size() == 46);

        REQUIRE(m.colors_channel_count() == 0);
        REQUIRE(m.normals().size() == 46);
        REQUIRE(m.tangents().empty());
        REQUIRE(m.bitangents().empty());
    }
}
