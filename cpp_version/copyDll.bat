set "TARGET_PATH_UNITY=..\unity pmugg\My project\Assets\Plugins\pmugg dll.dll"
set "TARGET_PATH_GODOT=..\godot\demo\bin\pmugg dll.dll"
set "TEMP_PATH=x64\Debug\pmugg dll.dll"

copy "$(TargetPath)" "%TEMP_PATH%"

:COPY_LOOP_UNITY
copy "%TEMP_PATH%" "%TARGET_PATH_UNITY%" > nul 2>&1
if errorlevel 1 (
    timeout /t 1 > nul
    goto COPY_LOOP_UNITY
)

:COPY_LOOP_GODOT
copy "%TEMP_PATH%" "%TARGET_PATH_GODOT%" > nul 2>&1
if errorlevel 1 (
    timeout /t 1 > nul
    goto COPY_LOOP_GODOT
)