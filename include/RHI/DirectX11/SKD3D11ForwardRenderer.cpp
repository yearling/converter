#ifdef SKWAI_DX11
#    include "Renderer/DirectX11/SKD3D11ForwardRenderer.h"
#    include "SKThreeDImpl.h"
#    include "Depreciated/SKEngine.h"
#    include "Engine/SKMaterial.h"
#    include "Depreciated/Elements/SKCamera.h"
#    include "Renderer/SKRenderable.h"
#    include "Renderer/Metal/SKMetalDevice.h"
#    include "Renderer/SKRuntimeMeshManager.h"
#    include "Renderer/SKShadingTechnique.h"
#    include "Renderer/SKShadingTechniqueUnlit.h"
#    include "Renderer/SKShadingTechniqueDirLit.h"
#    include "Renderer/SKShadingTechniqueDepth.h"
#    include "Renderer/DirectX11/SKD3D11Device.h"
namespace SKwai {

D3D11ForwardRenderer::D3D11ForwardRenderer(ThreeDImpl* ctx, RendererType renderer_type) : IRenderer(ctx, renderer_type) {}


void D3D11ForwardRenderer::OnResize(int width, int height) {}

void D3D11ForwardRenderer::BeginFrame() {}

void D3D11ForwardRenderer::EndFrame() {}

void D3D11ForwardRenderer::Shading(const RenderParam& render_param) {
    std::shared_ptr<RenderScene> render_scene = render_param.render_scene;
    std::shared_ptr<CameraBase> camera = render_scene->GetViewport().camera;
    std::shared_ptr<RenderQueueManager> render_queue_manager = render_scene->GetRenderQueueManager();
    bool use_depth_tex = render_param.require_depth_tex && render_param.has_depth_tex;
    int32_t render_queue_group_num = render_queue_manager->GetRenderQueueGroupNum();
    for (int32_t render_queue_group_index = 0; render_queue_group_index < render_queue_group_num; render_queue_group_index++) {
        const RenderQueueGroup& render_queue_group = render_queue_manager->GetRenderQueueGroup(render_queue_group_index);
        EFaceWinding face_winding = EFaceWinding::kNone;
        if (has_render_target_) {
            face_winding = render_param.is_mirror ? EFaceWinding::kCounterClockWise : EFaceWinding::kClockWise;
        } else {
            face_winding = render_param.is_mirror ? EFaceWinding::kClockWise : EFaceWinding::kCounterClockWise;
        }

        // debug
        face_winding = EFaceWinding::kNone;
        render_queue_group[RQT_BACKGROUND]->Render(render_scene, *unlit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);

        if (use_depth_tex) {
            // render_queue_group[RQT_TRANSLUCENT]->SetBackBufferDepth(mtl_render_command_->GetBackBufferDepth());
            // render_queue_group[RQT_ADDITIVE]->SetBackBufferDepth(mtl_render_command_->GetBackBufferDepth());
        }

        // opaque/translucent_z/additive_z pass
        int32_t render_queue_list[] = {RQT_OPAQUE, RQT_TRANSLUCENT_Z, RQT_ADDITIVE_Z};
        const int32_t kRenderQueueNum = sizeof(render_queue_list) / sizeof(RenderQueueType);
        for (int32_t rq_index = 0; rq_index < kRenderQueueNum; rq_index++) {
            int32_t rq = render_queue_list[rq_index];
            if (render_queue_group[rq]->IsEmpty()) {
                continue;
            }

            render_queue_group[rq]->Render(render_scene, *unlit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
            render_queue_group[rq]->Render(render_scene, *dir_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
            render_queue_group[rq]->Render(render_scene, *custom_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
            render_queue_group[rq]->Render(render_scene, *generic_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        }

        // translucent pass
        render_queue_group[RQT_TRANSLUCENT]->Render(render_scene, *unlit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_TRANSLUCENT]->Render(render_scene, *dir_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_TRANSLUCENT]->Render(render_scene, *custom_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_TRANSLUCENT]->Render(render_scene, *generic_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        // additive pass
        render_queue_group[RQT_ADDITIVE]->Render(render_scene, *unlit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_ADDITIVE]->Render(render_scene, *dir_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_ADDITIVE]->Render(render_scene, *custom_lit_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
        render_queue_group[RQT_ADDITIVE]->Render(render_scene, *generic_tech_, camera, nullptr, ctx_, render_param.order_by_dis2cam);
    }
}

void D3D11ForwardRenderer::Render(const RenderParam& render_param) {
    render_frame_count_++;

    std::shared_ptr<RenderScene> render_scene = render_param.render_scene;
    if (render_scene == nullptr) {
        return;
    }

    Viewport& viewport = render_scene->GetViewport();

    if (viewport.w == 0 || viewport.h == 0) {
        return;
    }

    std::shared_ptr<CameraBase> camera = viewport.camera;
    if (!camera) {
        return;
    }

    D3D11Device* device = ctx_->GetD3D11Device();
    Engine* engine = ctx_->GetEngine();
    render_scene->UpdateLightInfo(ctx_);

    RID render_target_rid = render_param.render_target_rid;
    RID depth_stencil_rid = render_param.depth_stencil_rid;
    has_render_target_ = render_target_rid || depth_stencil_rid;
    // to do render target
    /*
    if (!has_render_target_) {
        render_target_rid = ctx_->GetMainRenderTargetRID();
        depth_stencil_rid = ctx_->GetMainRenderTargetDSRID();
        if (!render_target_rid && !depth_stencil_rid) {
            return;
        }
        has_render_target_ = true;
    }
    */
    int width = device->GetDeviceWidth();
    int height = device->GetDeviceHeight();
    std::shared_ptr<RenderQueueManager> render_queue_manager = render_scene->GetRenderQueueManager();
    ConfigUnlitShadingTechnique(render_param, width, height);
    ConfigDirectionShadingTechnique(render_param, width, height);
    ConfigShadowmapGenShadingTechnique(render_param, width, height);
    ConfigCustomLitShadingTechnique(render_param, width, height);
    // ConfigDepthShadingTechnique(render_param, width, height);
    ConfigGenericShadingTechnique(render_param, width, height);
    if (render_scene->IsResourceCompleted()) {
        Shading(render_param);
        if (!render_queue_manager->IsEmpty()) {
            real_render_frame_count_++;
        }
    }

    //device->Present();
}
}  // namespace SKwai

#endif
