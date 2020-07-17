NOTE: Development of this project has been moved to https://github.com/spire-engine/spire-engine.
This repo is no longer being updated.


# SpireMiniEngine

[![Build status](https://ci.appveyor.com/api/projects/status/mw8aht0tabk677h1/branch/master?svg=true)](https://ci.appveyor.com/project/csyonghe/spireminiengine/branch/master)

SpireMiniEngine is a mini game engine that uses Spire to manange shader library and cross-compile shaders for different platforms.

## How to Run:
- Run "prepare.ps1" script, which downloads the Autodesk FBX SDK binaries required for building ModelImporter.
- Open "GameEngine.sln" in Visual Studio 2017.
- Build the solution. You may want to change Windows SDK Version in project settings to use a locally installed Windows SDK.
- Set GameEngine as start-up project.
- Right click GameEngine project and set the following start-up command:

`
-enginedir "$(SolutionDir)EngineContent" -dir "$(SolutionDir)ExampleGame" -gl
`
- Run.

To run the engine in editor mode, add `-editor` argument when launching GameEngine.exe:
```
GameEngine.exe -editor -enginedir "$(SolutionDir)EngineContent" -dir "$(SolutionDir)ExampleGame"
```
## Screenshot

![](https://github.com/csyonghe/SpireMiniEngineExtBinaries/raw/master/screenshot0.png)
