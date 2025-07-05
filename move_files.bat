@echo off
echo Reorganizing source files into modules...

REM 创建模块目录
mkdir src\core 2>nul
mkdir src\rendering 2>nul
mkdir src\input 2>nul
mkdir src\navigation 2>nul
mkdir src\ui 2>nul

REM 移动核心模块文件
echo Moving core module files...
move src\Application.cpp src\core\ 2>nul
move src\Logger.cpp src\core\ 2>nul
move src\Command.cpp src\core\ 2>nul
move src\CreateCommand.cpp src\core\ 2>nul
move src\GeometryObject.cpp src\core\ 2>nul
move src\GeometryFactory.cpp src\core\ 2>nul
move src\DPIManager.cpp src\core\ 2>nul
move src\DPIAwareRendering.cpp src\core\ 2>nul

REM 移动渲染模块文件
echo Moving rendering module files...
move src\RenderingEngine.cpp src\rendering\ 2>nul
move src\ViewportManager.cpp src\rendering\ 2>nul
move src\SceneManager.cpp src\rendering\ 2>nul
move src\CoordinateSystemRenderer.cpp src\rendering\ 2>nul
move src\PickingAidManager.cpp src\rendering\ 2>nul

REM 移动输入模块文件
echo Moving input module files...
move src\InputManager.cpp src\input\ 2>nul
move src\EventCoordinator.cpp src\input\ 2>nul
move src\MouseHandler.cpp src\input\ 2>nul
move src\DefaultInputState.cpp src\input\ 2>nul
move src\PickingInputState.cpp src\input\ 2>nul

REM 移动导航模块文件
echo Moving navigation module files...
move src\NavigationController.cpp src\navigation\ 2>nul
move src\NavigationStyle.cpp src\navigation\ 2>nul
move src\NavigationCube.cpp src\navigation\ 2>nul
move src\NavigationCubeManager.cpp src\navigation\ 2>nul
move src\CuteNavCube.cpp src\navigation\ 2>nul
move src\NavigationCubeConfigDialog.cpp src\navigation\ 2>nul

REM 移动UI模块文件
echo Moving UI module files...
move src\MainFrame.cpp src\ui\ 2>nul
move src\Canvas.cpp src\ui\ 2>nul
move src\ObjectTreePanel.cpp src\ui\ 2>nul
move src\PropertyPanel.cpp src\ui\ 2>nul
move src\PositionDialog.cpp src\ui\ 2>nul

echo File reorganization completed!
echo Note: main.cpp remains in src/ directory as the application entry point.
pause 