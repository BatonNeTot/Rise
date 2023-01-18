//☀Rise☀

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <unordered_set>
#include <queue>

#include "rise.h"
#include "window.h"
#include "vertices.h"
#include "font.h"

#include "resource.h"

#include "pugixml.hpp"

#include <windows.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    Rise::Init();

    Rise::Instance()->ResourceGenerator().IndexResources();
    Rise::Instance()->ResourceGenerator().Get<Rise::Vertices>("funny_triangle");
    Rise::Instance()->ResourceGenerator().Get<Rise::Image>("texture");
    auto size = sizeof(Rise::Instance()->ResourceGenerator().Get<Rise::Font>("times_new_roman"));
    Rise::Instance()->Logger().Info(std::string("testing - ") + Rise::LexicalCast(size));

    Rise::Window* window1 = Rise::Instance()->ConstructWindow("default", WIDTH, HEIGHT);
    window1->Open();
    /*
    Rise::Window* window2 = Rise::Instance()->ConstructWindow("default", WIDTH, HEIGHT);
    window2->Open();
    /*
    Rise::Window* window3 = Rise::Instance()->ConstructWindow("default", WIDTH, HEIGHT);
    window3->Open();
    //*
    Rise::Window* window4 = Rise::Instance()->ConstructWindow("default", WIDTH, HEIGHT);
    window4->Open();
    //*
    Rise::Window* window5 = Rise::Instance()->ConstructWindow("default", WIDTH, HEIGHT);
    window5->Open();
    //*/

    Rise::Instance()->Loop();

    Rise::Destroy();

    return EXIT_SUCCESS;
    //*/
}
