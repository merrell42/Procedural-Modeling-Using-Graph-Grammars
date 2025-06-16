set "TARGET_PATH=..\unity pmugg\My project\Assets\Plugins\pmugg dll.dll"
set "TEMP_PATH=x64\Debug\pmugg dll.dll"

copy "$(TargetPath)" "%TEMP_PATH%"

:COPY_LOOP
copy "%TEMP_PATH%" "%TARGET_PATH%" > nul 2>&1
if errorlevel 1 (
    timeout /t 1 > nul
    goto COPY_LOOP
)