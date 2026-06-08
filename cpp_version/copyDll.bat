@echo off
set "DEBUG_TARGET_PATH_UNITY=..\unity pmugg\Assets\Plugins\pmugg debug.dll"
set "RELEASE_TARGET_PATH_UNITY=..\unity pmugg\Assets\Plugins\pmugg release.dll"
set "RELEASE_TARGET_PATH_GODOT=..\godot\demo\bin\pmugg dll.dll"

set "DEBUG_TARGET_PATH_UNREAL=..\unreal\MyProject\Plugins\GrammarEditor\Binaries\Win64\pmugg debug.dll"
set "RELEASE_TARGET_PATH_UNREAL=..\unreal\MyProject\Plugins\GrammarEditor\Binaries\Win64\pmugg release.dll"

set "DEBUG_PATH=x64\Debug\pmugg dll.dll"
set "RELEASE_PATH=x64\Release\pmugg dll.dll"

echo Copying DLL files...

REM Check if Debug build exists and copy it
if exist "%DEBUG_PATH%" (
    echo Copying Debug DLL to Unity...
    :COPY_LOOP_DEBUG_UNITY
    copy "%DEBUG_PATH%" "%DEBUG_TARGET_PATH_UNITY%" > nul 2>&1
    if errorlevel 1 (
        timeout /t 1 > nul
        goto COPY_LOOP_DEBUG_UNITY
    )
    echo Debug DLL copied to Unity successfully.

    echo Copying Debug DLL to Unreal...
    :COPY_LOOP_DEBUG_UNREAL
    copy "%DEBUG_PATH%" "%DEBUG_TARGET_PATH_UNREAL%" > nul 2>&1
    if errorlevel 1 (
        timeout /t 1 > nul
        goto COPY_LOOP_DEBUG_UNREAL
    )
)

REM Check if Release build exists and copy it
if exist "%RELEASE_PATH%" (
    echo Copying Release DLL to Unity...
    :COPY_LOOP_RELEASE_UNITY
    copy "%RELEASE_PATH%" "%RELEASE_TARGET_PATH_UNITY%" > nul 2>&1
    if errorlevel 1 (
        timeout /t 1 > nul
        goto COPY_LOOP_RELEASE_UNITY
    )
    echo Release DLL copied to Unity successfully.

    echo Copying Release DLL to Godot...
    :COPY_LOOP_RELEASE_GODOT
    copy "%RELEASE_PATH%" "%RELEASE_TARGET_PATH_GODOT%" > nul 2>&1
    if errorlevel 1 (
        timeout /t 1 > nul
        goto COPY_LOOP_RELEASE_GODOT
    )
    echo Release DLL copied to Godot successfully.

    echo Copying Release DLL to Unreal...
    :COPY_LOOP_RELEASE_UNREAL
    copy "%RELEASE_PATH%" "%RELEASE_TARGET_PATH_UNREAL%" > nul 2>&1
    if errorlevel 1 (
        timeout /t 1 > nul
        goto COPY_LOOP_RELEASE_UNREAL
    )
    echo Release DLL copied to Unreal successfully.
)

echo Copy operations completed.

