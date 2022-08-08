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
		g_input_manager->OnEventLButtonDown(x, y);
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
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return YApplication::MyProc(hwnd, msg, wParam, lParam);
}

bool GameApplication::Initial()
{

	WindowCreate(defaut_windows_width, defaut_windows_height);
	//create engine
	YEngine* engine = YEngine::GetEngine();
	engine->SetApplication(this);
	engine->Init();
	// create editor
	// g_editor = std::make_unique<Editor>(engine);
	// g_editor->Init(windows_[0]->GetHWND());
	engine->TestLoad();

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
    //const std::string file_path = "E:/fbx/static_mesh/plane/space-ship/source/space_ship.fbx";
    //const std::string file_path = "E:/fbx/test_case/two_uv_two_material/test_uv_set.fbx";
    // error
    //const std::string file_path = "E:/fbx/static_mesh/animal/awil-werewolf/source/Werewolf.fbx"; // crash ,boen has the same name
    // error
    //const std::string file_path = "E:/fbx/static_mesh/sword/dragon-sword/source/Dragon_Bone_Sword.fbx";
    //const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-character-dragon-warrior-futuristic/source/dragonArmor_retopo_pose.fbx"; //skeltal mesh
    //const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog.fbx"; // good for degenerate triangle
    const std::string file_path = "E:/fbx/static_mesh/animal/sci-fi-dog/source/dog3.fbx"; // good for degenerate triangle
    //const std::string file_path = "E:/fbx/static_mesh/architecture/gothic-building-for-aria-pack-10/source/GothicBuildingForAriaPack.fbx"; // test for AA
    //const std::string file_path = "E:/fbx/static_mesh/bose/felstrider-mount/source/FelstriderAnimation.fbx"; 
    //const std::string file_path = "E:/fbx/static_mesh/bose/four-horsemen-24-pestilence-and-famine/source/Pestilence_4.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/four-horsemen-44-conquest/source/ConquestSketchfab.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/ibis/source/Ibis.fbx";  // crash
    //const std::string file_path = "E:/fbx/static_mesh/bose/imp/source/Imp_Posed.fbx"; 
    //const std::string file_path = "E:/fbx/static_mesh/bose/mandalorian-grogu-floating-pram/source/Mandalorian_The_Child_D.fbx";  // crash сх╢О
    //const std::string file_path = "E:/fbx/static_mesh/bose/rafaj-the-mulberry-warlock/source/Rafaj.fbx";  // bug ,skin error
    //const std::string file_path = "E:/fbx/static_mesh/bose/red-cherub/source/CherubTest.fbx";   // good  skeletal mesh  
    //const std::string file_path = "E:/fbx/static_mesh/bose/toxic-toby-dae-stylized-creation/source/SC_FInal_FullSCene.fbx";   // good    
    //const std::string file_path = "E:/fbx/static_mesh/car/6e48z1kc7r40-bugatti/bugatti.obj";   // no materials   
    //const std::string file_path = "E:/fbx/static_mesh/car/futuristic-truck/source/SketchXport.fbx";   // good for testing soft edege   
    //const std::string file_path = "E:/fbx/static_mesh/car/heavy-runner-2087/source/HeavyRunner_1.fbx";   // good for testing soft edege, should scale 0.01   
    //const std::string file_path = "E:/fbx/static_mesh/car/honda-cb-750-f-super-sport-1970/source/Honda.obj";   // no materials ,good   
    //const std::string file_path = "E:/fbx/static_mesh/car/hw7-details-2-xyz-draft-punk/source/2.obj";   // no materials ,good   
    //const std::string file_path = "E:/fbx/static_mesh/car/mazda-rx-7/source/rx7.fbx";   // good for testing soft edege, should scale 0.01 
    //const std::string file_path = "E:/fbx/static_mesh/car/motorcycle-szh2i2-demo/source/SZH2I2HDAndLDExport.fbx";   // crash
    //const std::string file_path = "E:/fbx/static_mesh/car/oshkosh-m-atv-reinvented/source/Model_1.fbx";   // good for test AA
    //const std::string file_path = "E:/fbx/static_mesh/car/ray-ii-szh2i2-merry-christmas-ver/source/MerryChristmas.fbx";   // crash
    //const std::string file_path = "E:/fbx/static_mesh/cloth/haveto-red-mage-helmet-flow-ver-preview/source/RedMageHelmetFlowVer.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/cloth/red-mage-helmet-flow-ver2/source/RedMageHelmetFlowVer2.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/cloth/red-mage-mask-procedural-reshade-ver/source/RedMageMask.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/cloth/shape-of-clothes-for-ray-ii-by-cloth-sim-demo/source/ClothesSimShowcase03.fbx";   // skin error
    //const std::string file_path = "E:/fbx/static_mesh/cloth/steampunk-glasses-goggles/source/a.fbx";   
    //const std::string file_path = "E:/fbx/static_mesh/cloth/the-plate-armor-of-ray/source/RayArmor.fbx";   // to large
    //const std::string file_path = "E:/fbx/static_mesh/cloth/the-plate-armor-of-ray/source/RayArmor.fbx";   // to large
    //const std::string file_path = "E:/fbx/static_mesh/gun/75-fbx/source/Handgun_fbx_7.4_binary.fbx";   // no detail
    //const std::string file_path = "E:/fbx/static_mesh/gun/ar15-corrected/source/WPN_MK18.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/gun/rifle/source/V308.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/gun/hmg-379/source/ERifle.fbx";   // good
    //const std::string file_path = "E:/fbx/static_mesh/human/alien-soldier/source/Alien.obj";   // good
    //const std::string file_path = "E:/fbx/static_mesh/human/Arcana/Arcana.fbx";   // normal is not right
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
    //const std::string file_path = "E:/fbx/static_mesh/human/male-armour-4-game-ready/source/ManAr4.fbx";    //good armour
    //const std::string file_path = "E:/fbx/static_mesh/human/nova/pilot_medium_nova.fbx";    //skin error
    //const std::string file_path = "E:/fbx/static_mesh/human/pivot-demo-journey/source/Jorney_clothes_v3.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/ray-a-girl-with-her-equipmentshowcase-ver/source/RayBasicShowcase02.fbx";    //crash
    //const std::string file_path = "E:/fbx/static_mesh/human/rococoframe/source/rococoSketchFab.obj";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/samurai-girl/source/Samurai_Girl.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/silk-shirt-suit-retopo-for-ray-ii/source/SilkShirtSuitRetopoForRayII.fbx";    //good
    //const std::string file_path = "E:/fbx/static_mesh/human/subsurface-scattering-sss-demo-lara/source/sss.fbx";    //skin error
    //const std::string file_path = "E:/fbx/static_mesh/plane/AH64A/AH64A.fbx";    //very good
    //const std::string file_path = "E:/fbx/static_mesh/plane/free-spaceship/source/Spaceship_02.fbx";    //very good
    //const std::string file_path = "E:/fbx/static_mesh/plane/ornithopter-dune-2021-se-build-by-zeo/source/Ornithopter_v011_Wings_Spread.obj";    // normal error
    //const std::string file_path = "E:/fbx/static_mesh/plane/space-ship/source/space_ship.fbx";    // very good    
    //const std::string file_path = "E:/fbx/static_mesh/robot/64-iron_man_mark_44_hulkbuster/Iron_Man_Mark_44_Hulkbuster_fbx.fbx";    // crash   
    //const std::string file_path = "E:/fbx/static_mesh/robot/blue-rig-procedural-reshade-version/source/Blue5.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/robot/industrial-robot/source/armLOW.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/robot/robot-from-the-series-love-death-and-robots/source/Robot.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/robot/security-robot/source/texDev003.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/robot/yellow-heavy-robot-8k-download/source/Mech_non_P.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/style/higokumaru-honkai-impact-3rd/source/Higokumaru.fbx";    // very good   
    //const std::string file_path = "E:/fbx/static_mesh/sword/1788-heavy-cavalry-sword-with-quarter-basket/source/1788_0526sketchfab.obj";    // very good   
    
    if (static_mesh_importer->ImportFile(file_path))
	{
		FbxImportParam importer_param;
        const FbxImportSceneInfo* scene_info = static_mesh_importer->GetImportedSceneInfo();
        importer_param.model_name = scene_info->model_name;
		importer_param.transform_vertex_to_absolute = true;
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
                for (auto& mesh : converted_static_mesh_vec)
                {
                    //mesh->SaveV0("head");
                    mesh->AllocGpuResource();
                    engine->static_mesh_ = std::move(mesh);
                }
                SWorld::GetWorld()->GetMainScene()->static_meshes_.push_back(engine->static_mesh_.get());
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	GameApplication app;
	app.SetInstance(hInstance);
	app.Initial();
	app.Run();
	app.Exit();
}

