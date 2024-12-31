#pragma once
#include "shape_view.h"
#include "grid_view.h"
#include "view_3d.h"
#include "main_controller.h"
#include "screen_saver.h"
#include "global_settings.h"

namespace ms {

class Setup {
public:
    static void initialize();
    static void setupEventListeners(MainController* controller);
    static void setupGlobalSettings();

private:
    Setup() = delete; // Static class
    
    static void createCanvases();
    static void setupViews();
    static void setupController();
    
    // Canvas references
    static Canvas* canvas1;
    static Canvas* gridCanvas;
    static Canvas* canvas2;
    static Canvas* canvas3d;
    
    // View references
    static ShapeView* shapeView;
    static GridView* gridView;
    static View3D* view3d;
    
    // Controller reference
    static MainController* controller;
};

} // namespace ms 