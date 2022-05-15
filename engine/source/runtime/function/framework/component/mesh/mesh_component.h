#pragma once

#include "runtime/function/framework/component/component.h"

#include "runtime/resource/res_type/components/mesh.h"

#include <vector>

// TODO PGameObjectComponentDesc should be converted to StaticMeshData
#include "runtime/function/scene/scene_object.h"

namespace Pilot
{
    REFLECTION_TYPE(MeshComponent)
    CLASS(MeshComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(MeshComponent)
    private:
        META(Enable)
        MeshComponentRes m_mesh_res;

        std::vector<GameObjectComponentDesc> m_raw_meshes;

    public:
        MeshComponent() {};
        MeshComponent(const MeshComponentRes& mesh_ast, GObject* parent_object);

        const std::vector<GameObjectComponentDesc>& getRawMeshes() const { return m_raw_meshes; }

        void tick(float delta_time) override;
    };
} // namespace Pilot
