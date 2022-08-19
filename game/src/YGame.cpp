#include "YGame.h"
#include "Engine/YCanvas.h"
#include "Engine/YInputManager.h"
#include "Engine/YWindowEventManger.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "Engine/YCanvasUtility.h"
#include "SObject/SWorld.h"
#include "Render/YForwardRenderer.h"
#include "SObject/SObjectManager.h"
#include "Engine/YLog.h"
#include "YFbxImporter.h"
#include "SObject/STexture.h"
#include <shellapi.h>
#include "Utility/YJsonHelper.h"


GameApplication::GameApplication()
{
    YApplication::is_editor = false;
}

GameApplication::~GameApplication()
{

}
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT GameApplication::MyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw()
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    //WARNING_INFO("run into Editor msg process");
    //POINT p;
    //static POINT last_p = {};
    //const BOOL b = GetCursorPos(&p);
    // under some unknown (permission denied...) rare circumstances GetCursorPos fails, we returns last known position
    //if (!b) p = last_p;
    //last_p = p;
    int x = ((int)(short)LOWORD(lParam));
    int y = ((int)(short)HIWORD(lParam));

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        g_input_manager->OnEventLButtonDown(x, y);
        break;
    case WM_LBUTTONUP:
        g_input_manager->OnEventLButtonUp(x, y);
        ReleaseCapture();
        break;
    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        g_input_manager->OnEventRButtonDown(x, y);
        break;

    case WM_RBUTTONUP:
        g_input_manager->OnEventRButtonUp(x, y);
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        g_input_manager->OnMouseMove(x, y);
        break;

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
        {
            SendMessageW(hwnd, WM_CLOSE, 0, 0);
            break;
        }
        }
        char key = (char)wParam;
        g_input_manager->OnEventKeyDown(key);
        break;
    }

    case WM_KEYUP:
        g_input_manager->OnEventKeyUp((char)wParam);
        break;
    case WM_MOUSEWHEEL:
    {
        int z_delta = (int)GET_WHEEL_DELTA_WPARAM(wParam);
        float z_normal = (float)z_delta / (float)WHEEL_DELTA;
        g_input_manager->OnMouseWheel(x, y, z_normal);
        break;
    }
    case WM_SIZE:
    {
        int g_winWidth = LOWORD(lParam);
        int g_winHeight = HIWORD(lParam);
        {
            switch (wParam)
            {
            case SIZE_MINIMIZED:
            {
                break;
            }
            case SIZE_MAXIMIZED:
            case SIZE_RESTORED:
            {
                if (g_windows_event_manager)
                {
                    g_windows_event_manager->OnWindowSizeChange(g_winWidth, g_winHeight);
                }
                break;
            }
            default:
                break;
            }
        }
        break;
    }
    case WM_ENTERSIZEMOVE:
    {
        break;
    }
    case WM_EXITSIZEMOVE:
    {
        break;
    }
    case WM_SYSCOMMAND:
    {
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    }
    case WM_DROPFILES:
    {
        HDROP h_drop = (HDROP)wParam;
        UINT file_num = DragQueryFile(h_drop, 0xFFFFFFFF, NULL, 0);
        std::vector<std::string> file_pathes;
        char strFileName[MAX_PATH];
        for (int i = 0; i <(int) file_num; i++)
        {
            DragQueryFile(h_drop, i, strFileName, MAX_PATH);
        }
        std::string file_name = strFileName;
        LOG_INFO("drop file: ", file_name);
        file_pathes.push_back(file_name);
        DragFinish(h_drop);
        ConverteModel(file_pathes);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return YApplication::MyProc(hwnd, msg, wParam, lParam);
}

bool GameApplication::Initial()
{
    int windows_x = defaut_windows_width;
    int windows_y = defaut_windows_height;
    windows_x = 2560;
    windows_y = 1440;
    WindowCreate(windows_x, windows_y);
    HWND current_window = windows_[0]->GetHWND();
    DragAcceptFiles(current_window, TRUE);
    //create engine
    YEngine* engine = YEngine::GetEngine();
    engine->SetApplication(this);
    engine->Init();
    // create editor
    // g_editor = std::make_unique<Editor>(engine);
    // g_editor->Init(windows_[0]->GetHWND());
    engine->TestLoad();
    g_input_manager->key_down_funcs_.push_back([this](char c) {
        this->OnKeyDown(c);
        });

    static_modle_path = {
           "E:/fbx/static_mesh/plane/free-spaceship/source/Spaceship_05.fbx",    //very good
           "E:/fbx/test_case/nija/mirror_nija_no_seam_small.fbx",    //测试 uv 镜像
           "E:/fbx/static_mesh/plane/free-spaceship/source/Spaceship_05.fbx",    //very good
           "E:/fbx/test_case/head.obj",    //测试 uv seam
           "E:/fbx/test_case/nija/uv_mirror_plane.fbx",    //测试 uv 镜像
          "E:/fbx/test_case/nija/qq_plane_uv_mirror.fbx",    //测试 uv 镜像
          "E:/fbx/static_mesh/plane/space-ship/source/space_ship.fbx",
          "E:/fbx/static_mesh/sword/dragon-sword/source/Dragon_Bone_Sword.fbx",
          "E:/fbx/static_mesh/bose/felstrider-mount/source/FelstriderAnimation.fbx", //multi mesh
          "E:/fbx/static_mesh/bose/imp/source/Imp_Posed.fbx",
          "E:/fbx/static_mesh/cloth/haveto-red-mage-helmet-flow-ver-preview/source/RedMageHelmetFlowVer.fbx",   // good
          "E:/fbx/static_mesh/cloth/red-mage-mask-procedural-reshade-ver/source/RedMageMask.fbx",   // good
          "E:/fbx/static_mesh/gun/75-fbx/source/Handgun_fbx_7.4_binary.fbx",   // no detail
          "E:/fbx/static_mesh/gun/ar15-corrected/source/WPN_MK18.fbx",   // good
          "E:/fbx/static_mesh/gun/rifle/source/V308.fbx",   // good,顶点没有焊
          "E:/fbx/static_mesh/gun/hmg-379/source/ERifle.fbx",   // good,模型很干净，适合测试normal map
          "E:/fbx/static_mesh/plane/AH64A/AH64A.fbx",    //very good
          "E:/fbx/static_mesh/human/male-armour-4-game-ready/source/ManAr4.fbx",    //good armour
          "E:/fbx/static_mesh/mask/Jonathan_BENAINOUS/source/sci-fi-helmet-blue-neon-jonathan-benainous.fbx",   //花钱买的
          "E:/fbx/static_mesh/plane/buster-drone/source/BusterDrone.fbx",   // good
          "E:/fbx/static_mesh/funature/aging-paint-material/source/Scene_matman.fbx", // test for PBR
          "E:/fbx/static_mesh/bose/anthro-shark/source/SHARK FULL ARMOR.obj", // test for PBR
          "E:/fbx/static_mesh/gun/ar-15-style-rifle/source/ar style gun.fbx",   // good
          "E:/fbx/static_mesh/human/baphomet/source/body1.fbx",   // good
          "E:/fbx/static_mesh/bose/black-fish/source/BlackFish.obj",   // good
          "E:/fbx/static_mesh/car/buggy-2/source/VHC_SC_Buggy_01.fbx",   // good,老爷车, 测试 uv 镜像
          "E:/fbx/static_mesh/bose/xeno-raven/source/XenoRaven.fbx",   // 异形
          "E:/fbx/static_mesh/bose/cthulhu-statuette/source/Horror_low_subd.obj",   // 异形
          "E:/fbx/static_mesh/plane/spaceship-nortend/source/model.dae",   // good texture ,scale 10
          "E:/fbx/static_mesh/bose/toxic-toby-dae-stylized-creation/source/SC_FInal_FullSCene.fbx",   // good    
          "E:/fbx/static_mesh/car/futuristic-truck/source/SketchXport.fbx",   // good for testing soft edege   
          "E:/fbx/static_mesh/car/heavy-runner-2087/source/HeavyRunner_1.fbx",   // good for testing soft edege, should scale 0.01   
          "E:/fbx/static_mesh/car/honda-cb-750-f-super-sport-1970/source/Honda.obj",   // no materials ,good   
          "E:/fbx/static_mesh/car/hw7-details-2-xyz-draft-punk/source/2.obj",   // no materials ,good   
          "E:/fbx/static_mesh/car/mazda-rx-7/source/rx7.fbx",   //crash, good for testing soft edege, should scale 0.01 
          "E:/fbx/static_mesh/car/motorcycle-szh2i2-demo/source/SZH2I2HDAndLDExport.fbx",   // crash
          "E:/fbx/static_mesh/car/oshkosh-m-atv-reinvented/source/Model_1.fbx",   // good for test AA
          "E:/fbx/static_mesh/car/ray-ii-szh2i2-merry-christmas-ver/source/MerryChristmas.fbx",   // crash
          "E:/fbx/static_mesh/sword/1788-heavy-cavalry-sword-with-quarter-basket/source/1788_0526sketchfab.obj",    // very good   
          "E:/fbx/static_mesh/style/higokumaru-honkai-impact-3rd/source/Higokumaru.fbx",    // very good   ,二次元 
          "E:/fbx/static_mesh/robot/yellow-heavy-robot-8k-download/source/Mech_non_P.fbx",    // very good ,大机甲 
          "E:/fbx/static_mesh/robot/security-robot/source/texDev003.fbx",    // very good  ,小机甲 
          "E:/fbx/static_mesh/robot/industrial-robot/source/armLOW.fbx",    // very good   ,机械臂
          "E:/fbx/static_mesh/robot/blue-rig-procedural-reshade-version/source/Blue5.fbx",    // very good  ,机械人 
          "E:/fbx/static_mesh/robot/64-iron_man_mark_44_hulkbuster/Iron_Man_Mark_44_Hulkbuster_fbx.fbx",    // 反浩克装甲，已经修复
          "E:/fbx/static_mesh/cloth/red-mage-mask-procedural-reshade-ver/source/RedMageMask.fbx",   // good
          "E:/fbx/static_mesh/gun/75-fbx/source/Handgun_fbx_7.4_binary.fbx",   // no detail
          "E:/fbx/static_mesh/gun/ar15-corrected/source/WPN_MK18.fbx",   // good
          "E:/fbx/static_mesh/gun/rifle/source/V308.fbx",   // good,顶点没有焊
          "E:/fbx/static_mesh/plane/scifi-space-fighter-based-on-abrasax-wraith/source/Wraith.fbx",   // good texture ,scale 10
          "E:/fbx/static_mesh/architecture/rocket-orbiting-moon/source/Maan_Totaal.fbx",    //月球
          "E:/fbx/static_mesh/human/a-set-of-overalls-in-the-style-of-cyberpunk/source/clo.fbx",    // cyber femal

          "E:/fbx/static_mesh/plane/ornithopter-dune-2021-se-build-by-zeo/source/Ornithopter_v011_Wings_Spread.obj",    // 沙丘 扑翼机
          "E:/fbx/static_mesh/bose/mandalorian-grogu-floating-pram/source/Mandalorian_The_Child_D.fbx",  // 尤达, hi poly
          "E:/fbx/static_mesh/architecture/gothic-building-for-aria-pack-10/source/GothicBuildingForAriaPack.fbx", // test for AA,large scale
          "E:/fbx/static_mesh/funature/cathedral/source/combined02.obj",   // good,我的世界--教堂
          "E:/fbx/static_mesh/architecture/abandoned-house/source/abandonhouse.fbx", // test for AO
          "E:/fbx/static_mesh/car/6e48z1kc7r40-bugatti/bugatti.obj",   // crash, no uv, no materials   
          "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog.fbx", // good for degenerate triangle
          "E:/fbx/static_mesh/car/ray-ii-szh2i2-merry-christmas-ver/source/MerryChristmas.fbx",   // crash
          "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog3.fbx", // good for degenerate triangle
          "E:/fbx/static_mesh/cloth/haveto-red-mage-helmet-flow-ver-preview/source/RedMageHelmetFlowVer.fbx",   // good
          "E:/fbx/static_mesh/human/alien-soldier/source/Alien.obj",   // good, 100m vertex 
    };
    //Json::Value root;
    //for (std::string file_name : static_modle_path)
    //{
    //    Json::Value tmp;
    //    tmp["path"] = file_name;
    //    tmp["scale"] = YJsonHelper::ConvertVectorToJson(YVector(1.0, 1.0, 1.0));
    //    tmp["translation"] = YJsonHelper::ConvertVectorToJson(YVector(0.0, 0.0, 0.0));
    //    root.append(tmp);
    //}
    ////std::string result_json = root.toStyledString();
    //YJsonHelper::SaveJsonToFile("auto_test.json", root);

    std::unique_ptr<YFbxImporter> static_mesh_importer = std::make_unique<YFbxImporter>();
    //const std::string file_path = "E:/topo_split/head.fbx";
    //const std::string file_path = "E:/fbx/female_Rig_DH_01.fbx";
    //const std::string file_path = "E:/fbx/tube_with_animaiton.fbx";
    //const std::string file_path = "E:/fbx/cube_animation.fbx";
    //const std::string file_path = "E:/fbx/deer.fbx";
    //const std::string file_path = "E:/fbx/aoteman_attack01.fbx";
    //const std::string file_path = "E:/fbx/bs/ball_bs.fbx";
    //const std::string file_path = "E:/fbx/avata/aishapelive_material/male/male_head_BS.fbx";
    //const std::string file_path = "E:/fbx/bs/animation/man_talking.fbx";
    //const std::string file_path = "E:/fbx/bs/animation/female_talking2.fbx";
    //const std::string file_path = "E:/fbx/EpicCharacter_Run.fbx";
    //const std::string file_path = "E:/fbx/test_case/nija/qq_plane_uv_mirror.fbx";
    //const std::string file_path = "E:/fbx/static_mesh/large/Set_02.fbx";

    // static mesh
    //const std::string file_path = "E:/fbx/static_mesh/plane/free-spaceship/source/Spaceship_05.fbx";    //very good
    //const std::string file_path = "E:/fbx/static_mesh/plane/space-ship/source/space_ship.fbx";
    //const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog.fbx"; // good for degenerate triangle
    //const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog3.fbx"; // good for degenerate triangle
    //const std::string file_path = "E:/fbx/static_mesh/sword/dragon-sword/source/Dragon_Bone_Sword.fbx";
    //const std::string file_path = "E:/fbx/static_mesh/architecture/gothic-building-for-aria-pack-10/source/GothicBuildingForAriaPack.fbx"; // test for AA,large scale
    //const std::string file_path = "E:/fbx/static_mesh/bose/felstrider-mount/source/FelstriderAnimation.fbx"; //multi mesh
    //const std::string file_path = "E:/fbx/static_mesh/bose/imp/source/Imp_Posed.fbx"; 
    //const std::string file_path = "E:/fbx/static_mesh/bose/mandalorian-grogu-floating-pram/source/Mandalorian_The_Child_D.fbx";  // 尤达, hi poly
    //const std::string file_path = "E:/fbx/static_mesh/bose/toxic-toby-dae-stylized-creation/source/SC_FInal_FullSCene.fbx";   // good    
    //const std::string file_path = "E:/fbx/static_mesh/car/6e48z1kc7r40-bugatti/bugatti.obj";   // crash, no uv, no materials   
    //const std::string file_path = "E:/fbx/static_mesh/car/futuristic-truck/source/SketchXport.fbx";   // good for testing soft edege   
    //const std::string file_path = "E:/fbx/static_mesh/car/heavy-runner-2087/source/HeavyRunner_1.fbx";   // good for testing soft edege, should scale 0.01   
    //const std::string file_path = "E:/fbx/static_mesh/car/honda-cb-750-f-super-sport-1970/source/Honda.obj";   // no materials ,good   
    //const std::string file_path = "E:/fbx/static_mesh/car/hw7-details-2-xyz-draft-punk/source/2.obj";   // no materials ,good   
    //const std::string file_path = "E:/fbx/static_mesh/car/mazda-rx-7/source/rx7.fbx";   //crash, good for testing soft edege, should scale 0.01 
    //const std::string file_path = "E:/fbx/static_mesh/mask/Jonathan_BENAINOUS/source/sci-fi-helmet-blue-neon-jonathan-benainous.fbx";   //花钱买的
    //const std::string file_path = "E:/fbx/static_mesh/car/motorcycle-szh2i2-demo/source/SZH2I2HDAndLDExport.fbx";   // crash
    //const std::string file_path = "E:/fbx/static_mesh/car/oshkosh-m-atv-reinvented/source/Model_1.fbx";   // good for test AA
    const std::string file_path = "E:/fbx/static_mesh/car/ray-ii-szh2i2-merry-christmas-ver/source/MerryChristmas.fbx";   // crash
    //const std::string file_path = "E:/fbx/static_mesh/sword/1788-heavy-cavalry-sword-with-quarter-basket/source/1788_0526sketchfab.obj";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/style/higokumaru-honkai-impact-3rd/source/Higokumaru.fbx";    // very good   ,二次元 
    //const std::string file_path = "E:/fbx/static_mesh/robot/yellow-heavy-robot-8k-download/source/Mech_non_P.fbx";    // very good ,大机甲 
    //const std::string file_path = "E:/fbx/static_mesh/robot/security-robot/source/texDev003.fbx";    // very good  ,小机甲 
    //const std::string file_path = "E:/fbx/static_mesh/robot/industrial-robot/source/armLOW.fbx";    // very good   ,机械臂
    //const std::string file_path = "E:/fbx/static_mesh/robot/blue-rig-procedural-reshade-version/source/Blue5.fbx";    // very good  ,机械人 
    //const std::string file_path = "E:/fbx/static_mesh/robot/64-iron_man_mark_44_hulkbuster/Iron_Man_Mark_44_Hulkbuster_fbx.fbx";    // 反浩克装甲，已经修复
    //const std::string file_path = "E:/fbx/static_mesh/car/ray-ii-szh2i2-merry-christmas-ver/source/MerryChristmas.fbx";   // crash
    //const std::string file_path = "E:/fbx/static_mesh/cloth/haveto-red-mage-helmet-flow-ver-preview/source/RedMageHelmetFlowVer.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/cloth/red-mage-mask-procedural-reshade-ver/source/RedMageMask.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/gun/75-fbx/source/Handgun_fbx_7.4_binary.fbx";   // no detail
    //const std::string file_path = "E:/fbx/static_mesh/gun/ar15-corrected/source/WPN_MK18.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/gun/rifle/source/V308.fbx";   // good,顶点没有焊
    //const std::string file_path = "E:/fbx/static_mesh/gun/hmg-379/source/ERifle.fbx";   // good,模型很干净，适合测试normal map
    //const std::string file_path = "E:/fbx/static_mesh/plane/AH64A/AH64A.fbx";    //very good
    //const std::string file_path = "E:/fbx/static_mesh/plane/ornithopter-dune-2021-se-build-by-zeo/source/Ornithopter_v011_Wings_Spread.obj";    // 沙丘 扑翼机
    //const std::string file_path = "E:/fbx/static_mesh/human/male-armour-4-game-ready/source/ManAr4.fbx";    //good armour
    //const std::string file_path = "E:/fbx/static_mesh/car/buggy-2/source/VHC_SC_Buggy_01.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/plane/buster-drone/source/BusterDrone.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/architecture/abandoned-house/source/abandonhouse.fbx"; // test for AO
    //const std::string file_path = "E:/fbx/static_mesh/funature/aging-paint-material/source/Scene_matman.fbx"; // test for PBR
    //const std::string file_path = "E:/fbx/static_mesh/bose/anthro-shark/source/SHARK FULL ARMOR.obj"; // test for PBR
    //const std::string file_path = "E:/fbx/static_mesh/gun/ar-15-style-rifle/source/ar style gun.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/human/baphomet/source/body1.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/bose/black-fish/source/BlackFish.obj";   // good
    //const std::string file_path = "E:/fbx/static_mesh/car/buggy-2/source/VHC_SC_Buggy_01.fbx";   // good,老爷车
    //const std::string file_path = "E:/fbx/static_mesh/funature/cathedral/source/combined02.obj";   // good,我的世界--教堂
    //const std::string file_path = "E:/fbx/static_mesh/bose/xeno-raven/source/XenoRaven.fbx";   // 异形
   // const std::string file_path = "E:/fbx/static_mesh/bose/cthulhu-statuette/source/Horror_low_subd.obj";   // 异形
    //const std::string file_path = "E:/fbx/static_mesh/plane/spaceship-nortend/source/model.dae";   // good texture ,scale 10
    //const std::string file_path = "E:/fbx/static_mesh/plane/scifi-space-fighter-based-on-abrasax-wraith/source/Wraith.fbx";   // good texture ,scale 10
    //const std::string file_path = "E:/fbx/static_mesh/architecture/rocket-orbiting-moon/source/Maan_Totaal.fbx";    //月球
    //const std::string file_path = "E:/fbx/static_mesh/human/a-set-of-overalls-in-the-style-of-cyberpunk/source/clo.fbx";    // cyber femal
    //const std::string file_path = "E:/fbx/test_case/nija/uv_mirror_plane.fbx";    //测试 uv 镜像
    //const std::string file_path = "E:/fbx/test_case/nija/qq_plane_uv_mirror.fbx";    //测试 uv 镜像
    //const std::string file_path = "E:/fbx/test_case/nija/mirror_nija_no_seam_small.fbx";    //测试 uv 镜像
    //const std::string file_path = "E:/fbx/test_case/nija/mirror_nija_no_seam_small.fbx";    //测试 uv seam
    //const std::string file_path = "E:/fbx/test_case/uv_seam.fbx";    //测试 uv seam


    // skeletal mesh
    //const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-character-dragon-warrior-futuristic/source/dragonArmor_retopo_pose.fbx"; //skeltal mesh
    //const std::string file_path = "E:/fbx/static_mesh/animal/awil-werewolf/source/Werewolf.fbx"; // crash ,boen has the same name
    //const std::string file_path = "E:/fbx/static_mesh/bose/four-horsemen-24-pestilence-and-famine/source/Pestilence_4.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/four-horsemen-44-conquest/source/ConquestSketchfab.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/ibis/source/Ibis.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/rafaj-the-mulberry-warlock/source/Rafaj.fbx";  // bug ,skin error
    //const std::string file_path = "E:/fbx/static_mesh/bose/red-cherub/source/CherubTest.fbx";   // good  skeletal mesh  
    //const std::string file_path = "E:/fbx/static_mesh/robot/robot-from-the-series-love-death-and-robots/source/Robot.fbx";    // very good  ,爱死机 
    //const std::string file_path = "E:/fbx/static_mesh/human/Arcana/Arcana.fbx";   // normal is not right
    //const std::string file_path = "E:/fbx/static_mesh/cloth/shape-of-clothes-for-ray-ii-by-cloth-sim-demo/source/ClothesSimShowcase03.fbx";   // skin error

    //const std::string file_path = "E:/fbx/static_mesh/cloth/red-mage-helmet-flow-ver2/source/RedMageHelmetFlowVer2.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/cloth/steampunk-glasses-goggles/source/a.fbx";   
    //const std::string file_path = "E:/fbx/static_mesh/cloth/the-plate-armor-of-ray/source/RayArmor.fbx";   // to large
    //const std::string file_path = "E:/fbx/static_mesh/cloth/the-plate-armor-of-ray/source/RayArmor.fbx";   // to large
    //const std::string file_path = "E:/fbx/static_mesh/human/armor-for-ray-ii/source/ArmorForRayII.fbx";   // good    
    //const std::string file_path = "E:/fbx/static_mesh/human/BattlefieldV/Hanna.fbx";   // skin error   
    //const std::string file_path = "E:/fbx/static_mesh/human/bulky-knight/source/BigNIght.fbx";   // very good 
    //const std::string file_path = "E:/fbx/static_mesh/human/Cyberpunk2077_Chr_EvelynParker/Evelyn.fbx";   // skin error
    //const std::string file_path = "E:/fbx/static_mesh/human/deathstroke-from-bak/source/Deathstroke.fbx";   // very good , normal error
    //const std::string file_path = "E:/fbx/static_mesh/human/dress-suit-retopo-and-bake-for-ray-ii/source/DressSuitRetopoAndBakeForRayII.fbx";   // very good
    //const std::string file_path = "E:/fbx/static_mesh/human/genshin-impact-shenhe/source/Shenhe.fbx";   //model mirror
    //const std::string file_path = "E:/fbx/static_mesh/human/haute-couture-for-ray-ii-pose-2/source/RayIIHauteCoutureHDPose2.fbx";   //error: material crash
    //const std::string file_path = "E:/fbx/static_mesh/human/head-ring-jewly-hair-pin-retopo-and-hairstyle/source/HeadRing&HairPinRetopoAndMixedHairstyleForRayII.fbx";    // very good
    //const std::string file_path = "E:/fbx/static_mesh/human/high-heels-for-ray-ii-retopo-and-bake/source/HighHeelForRayIIRetopoAndBake.fbx";    //only leg
    //const std::string file_path = "E:/fbx/static_mesh/human/nova/pilot_medium_nova.fbx";    //skin error
    //const std::string file_path = "E:/fbx/static_mesh/human/pivot-demo-journey/source/Jorney_clothes_v3.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/ray-a-girl-with-her-equipmentshowcase-ver/source/RayBasicShowcase02.fbx";    //crash
    //const std::string file_path = "E:/fbx/static_mesh/human/rococoframe/source/rococoSketchFab.obj";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/samurai-girl/source/Samurai_Girl.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/silk-shirt-suit-retopo-for-ray-ii/source/SilkShirtSuitRetopoForRayII.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/subsurface-scattering-sss-demo-lara/source/sss.fbx";    //skin error
    ConverteModel(std::vector<std::string>{file_path});
   /* const std::string package_path = "/model/Spaceship_05/Spaceship_05.json";
    TRefCountPtr<SStaticMesh> ss = SObjectManager::ConstructFromPackage<SStaticMesh>(package_path, nullptr);
    ss->GetStaticMesh()->AllocGpuResource();
    engine->static_mesh_ = ss;
    SWorld::GetWorld()->GetMainScene()->static_meshes_.clear();
    SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_->GetStaticMesh());*/
    //AutoTest();
    return true;
}

void GameApplication::Render()
{
    double delta_time = 0.0;
    double game_time = 0.0;
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
        delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count();
        delta_time *= 0.000001; // to second
        last_frame_time = current_time;
        game_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - game_start_time).count();
        game_time *= 0.000001;
    }
    // g_editor->Update(delta_time);
    YEngine* engine = YEngine::GetEngine();
    engine->Update();
    //g_device->GetDC()->CopyResource(g_device->GetMainRTV(),)


}

void GameApplication::Exit()
{
    // g_editor->Close();
    YEngine* engine = YEngine::GetEngine();
    engine->ShutDown();
}

void GameApplication::OnKeyDown(char c)
{
    LOG_INFO("puss char", (int)c);
    if (((int)c) == 37)
    {
        current_index--;
        if (current_index <= 0)
        {
            current_index = 0;
        }
        SwitchModel();
    }

    if (((int)c) == 39)
    {
        current_index++;
        if (current_index > static_modle_path.size())
        {
            current_index = (int)static_modle_path.size() - 1;
        }
        SwitchModel();
    }

}

void GameApplication::SwitchModel()
{
    std::string file_path = static_modle_path[current_index];
    ConverteModel(std::vector<std::string>{file_path});
}

void GameApplication::ConverteModel(const std::vector<std::string>& file_pathes)
{
    for (int i = 0; i < (int)file_pathes.size(); ++i)
    {
        std::unique_ptr<YFbxImporter> static_mesh_importer = std::make_unique<YFbxImporter>();
        std::string file_path = file_pathes[i];
        if (static_mesh_importer->ImportFile(file_path))
        {
            FbxImportParam importer_param;
            const FbxImportSceneInfo* scene_info = static_mesh_importer->GetImportedSceneInfo();
            importer_param.model_name = scene_info->model_name;
            importer_param.transform_vertex_to_absolute = true;
            importer_param.import_scaling = YVector(1.0, 1.0, 1.0);
            if (scene_info->has_skin)
            {
                importer_param.import_as_skelton = true;
                ConvertedResult result;
                if (static_mesh_importer->ParseFile(importer_param, result))
                {
                    //auto& converted_static_mesh_vec = result.static_meshes;
                    //for (auto& mesh : converted_static_mesh_vec)
                    //{
                        //mesh->SaveV0("head");
                    //}
                    YEngine* engine = YEngine::GetEngine();
                    engine->skeleton_mesh_ = std::move(result.skeleton_mesh);
                    SWorld::GetWorld()->GetMainScene()->skeleton_meshes_.push_back(engine->skeleton_mesh_.get());

                }
                else
                {
                    ERROR_INFO("parse file ", file_path, "failed!");
                }
            }
            else
            {
                importer_param.import_as_skelton = false;
                ConvertedResult result;
                if (static_mesh_importer->ParseFile(importer_param, result))
                {
                    YEngine* engine = YEngine::GetEngine();
                    auto& converted_static_mesh_vec = result.static_meshes;
                    std::string package_path;
                    for (auto& mesh : converted_static_mesh_vec)
                    {
                        //mesh->SaveToPackage()
                        mesh->GetStaticMesh()->AllocGpuResource();
                        engine->static_mesh_ = mesh; //hold life
                        std::string model_name = mesh->GetStaticMesh()->model_name;
                        std::string model_base = "model";
                        std::string model_final_name = YPath::PathCombine(model_base, model_name);
                        //model_final_name = YPath::PathCombine(model_final_name, model_name);
                        mesh->SetSavedPath(model_final_name);
                        Json::Value root;
                        mesh->SaveToJson(root);
                        std::string json_path = YPath::PathCombine(model_final_name, model_name+SObject::json_extension_with_dot);
                        package_path = '/'+ json_path;
                        YJsonHelper::SaveJsonToFile(json_path, root);
                    }
                    SWorld::GetWorld()->GetMainScene()->static_meshes_.clear();
                    SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_->GetStaticMesh());
                    {
                        TRefCountPtr<SStaticMesh> ss = SObjectManager::ConstructFromPackage<SStaticMesh>(package_path,nullptr);
                        ss->GetStaticMesh()->AllocGpuResource();
                        engine->static_mesh_ = ss;
                        SWorld::GetWorld()->GetMainScene()->static_meshes_.clear();
                        SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_->GetStaticMesh());
                    }
                }
                else
                {
                    ERROR_INFO("parse file ", file_path, "failed!");
                }
            }
        }
        else
        {
            ERROR_INFO("open file ", file_path, "failed!");
        }
    }
}

void GameApplication::AutoTest()
{
    Json::Value files_to_import;
    if (!YJsonHelper::LoadJsonFromFile("C:/Users/admin/Desktop/converter/auto_test.json", files_to_import))
    {
        return;
    }

    int fbx_to_imported = files_to_import.size();
    for (int i = 0; i < fbx_to_imported; ++i)
    {
        Json::Value& import_item = files_to_import[i];
        std::string file_path;
        if (import_item.isMember("path"))
        {
            file_path = import_item["path"].asString();
        }

        YVector scale = YVector(1.0, 1.0, 1.0);
        if (import_item.isMember("scale"))
        {
            YJsonHelper::ConvertJsonToVector(import_item["scale"],scale);
        }

        YVector translation = YVector(0.0, 0.0, 0.0);
        if (import_item.isMember("translation"))
        {
            YJsonHelper::ConvertJsonToVector(import_item["translation"], translation);
        }
        std::unique_ptr<YFbxImporter> static_mesh_importer = std::make_unique<YFbxImporter>();
        if (static_mesh_importer->ImportFile(file_path))
        {
            FbxImportParam importer_param;
            const FbxImportSceneInfo* scene_info = static_mesh_importer->GetImportedSceneInfo();
            importer_param.model_name = scene_info->model_name;
            importer_param.transform_vertex_to_absolute = true;
            importer_param.import_scaling = YVector(1.0, 1.0, 1.0);
            if (scene_info->has_skin)
            {
                WARNING_INFO(file_path, " is a skeleton mesh, skip");
                continue;
                importer_param.import_as_skelton = true;
                ConvertedResult result;
                if (static_mesh_importer->ParseFile(importer_param, result))
                {
                    //auto& converted_static_mesh_vec = result.static_meshes;
                    //for (auto& mesh : converted_static_mesh_vec)
                    //{
                        //mesh->SaveV0("head");
                    //}
                    YEngine* engine = YEngine::GetEngine();
                    engine->skeleton_mesh_ = std::move(result.skeleton_mesh);
                    SWorld::GetWorld()->GetMainScene()->skeleton_meshes_.push_back(engine->skeleton_mesh_.get());

                }
                else
                {
                    ERROR_INFO("parse file ", file_path, "failed!");
                }
            }
            else
            {
                importer_param.import_as_skelton = false;
                importer_param.import_scaling = scale;
                importer_param.import_translation = translation;
                ConvertedResult result;
                if (static_mesh_importer->ParseFile(importer_param, result))
                {
                    YEngine* engine = YEngine::GetEngine();
                    auto& converted_static_mesh_vec = result.static_meshes;
                    std::string package_path;
                    for (auto& mesh : converted_static_mesh_vec)
                    {
                        //mesh->SaveToPackage()
                        mesh->GetStaticMesh()->AllocGpuResource();
                        engine->static_mesh_ = mesh; //hold life
                        std::string model_name = mesh->GetStaticMesh()->model_name;
                        std::string model_base = "model";
                        std::string model_final_name = YPath::PathCombine(model_base, model_name);
                        mesh->SetSavedPath(model_final_name);
                        Json::Value root;
                        mesh->SaveToJson(root);
                        std::string json_path = YPath::PathCombine(model_final_name, model_name + SObject::json_extension_with_dot);
                        package_path = '/' + json_path;
                        YJsonHelper::SaveJsonToFile(json_path, root);
                    }
                    SWorld::GetWorld()->GetMainScene()->static_meshes_.clear();
                    SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_->GetStaticMesh());
                    {
                        TRefCountPtr<SStaticMesh> ss = SObjectManager::ConstructFromPackage<SStaticMesh>(package_path, nullptr);
                        ss->GetStaticMesh()->AllocGpuResource();
                        engine->static_mesh_ = ss;
                        SWorld::GetWorld()->GetMainScene()->static_meshes_.clear();
                        SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_->GetStaticMesh());
                    }
                }
                else
                {
                    ERROR_INFO("parse file ", file_path, "failed!");
                }
            }
        }
        else
        {
            ERROR_INFO("open file ", file_path, "failed!");
        }

    }

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
    GameApplication app;
    app.SetInstance(hInstance);
    app.Initial();
    app.Run();
    app.Exit();
}

