-- Grammar Editor for 3DS Max
-- Based on Unity GrammarEditorWindow.cs
-- UI implementation with DLL integration

-- DLL Function Declarations
-- DLL path - adjust this path to match your system
dllPath = "C:\\PMUGG\\cpp_version\\x64\\Release\\pmugg dll.dll"

-- Try to load DLL using different methods
fn loadGrammarDLL = 
(
    -- First check if file exists
    if not doesFileExist dllPath then
    (
        format "DLL file not found: %\n" dllPath
        messageBox ("DLL file not found: " + dllPath) title:"DLL Error"
        return false
    )
    
    try
    (
        -- Try method 1: loadDLL
        loadDLL dllPath
        format "DLL loaded successfully with loadDLL: %\n" dllPath
        return true
    )
    catch
    (
        format "loadDLL failed, trying alternative method...\n"
        
        try
        (
            -- Try method 2: dotNetObject approach
            local assembly = dotNetClass "System.Reflection.Assembly"
            assembly.LoadFile dllPath
            format "DLL loaded successfully with dotNetObject: %\n" dllPath
            return true
        )
        catch
        (
            format "All DLL loading methods failed for: %\n" dllPath
            messageBox ("Failed to load DLL: " + dllPath + "\nPlease ensure:\n1. The DLL exists\n2. The DLL is compatible with your system\n3. All dependencies are available") title:"DLL Error"
            return false
        )
    )
)

-- Unload DLL
fn unloadGrammarDLL = 
(
    try
    (
        unloadDLL dllPath
        format "DLL unloaded: %\n" dllPath
    )
    catch
    (
        format "Error unloading DLL: %\n" (getCurrentException())
    )
)

-- Test DLL availability
fn testDll = 
(
    if loadGrammarDLL() then
    (
        format "DLL loaded successfully: %\n" dllPath
        true
    )
    else
    (
        messageBox "Failed to load PMUGG DLL. Please ensure the DLL is in the correct location." title:"DLL Error"
        false
    )
)

-- Placeholder DLL functions for testing - replace with actual DLL calls once loading works
fn initialize filePath seed = 
(
    format "DLL Call: initialize with file: % seed: %\n" filePath seed
    "Success" -- Return success for testing
)

fn resetFunc seed = 
(
    format "DLL Call: reset with seed: %\n" seed
)

fn iterate steps = 
(
    format "DLL Call: iterate with steps: %\n" steps
)

fn iterateToTime timeSeconds = 
(
    format "DLL Call: iterateToTime with time: %\n" timeSeconds
    1 -- Return 1 step completed for testing
)

fn setModelSize x y z = 
(
    format "DLL Call: setSize with x:% y:% z:%\n" x y z
)

fn getMesh = 
(
    format "DLL Call: getMesh\n"
    -- Return dummy mesh data for testing
    #(#(), #(), #(), #(), 0, 0, 0)
)

fn destroyMesh meshData = 
(
    format "DLL Call: destroyMesh\n"
)

rollout GrammarEditorRollout "Graph Grammar Generator" width:400 -- height:600
(
    -- Constants
    local MAX_ITERATION = 200
    
    -- UI State Variables
    local fileQueue = #()
    local maxIteration = MAX_ITERATION
    local grammarName = ""
    local iterationCount = 0
    local isAnimating = false
    local seed = 0
    local sizeX = 30.0
    local sizeY = 20.0
    local sizeZ = 10.0
    local startTime = 0
    local timerActive = false
    
    -- UI Elements
    label labelStatus "No grammar loaded" height:20
    
    group "Parameters"
    (
        spinner spnSeed "Seed:" type:#integer range:[-999999, 999999, 0] width:80 across:4
        spinner spnSizeX "Size X:" type:#float range:[0.1, 1000.0, 30.0] width:80
        spinner spnSizeY "Size Y:" type:#float range:[0.1, 1000.0, 20.0] width:80
        spinner spnSizeZ "Size Z:" type:#float range:[0.1, 1000.0, 10.0] width:80
    )
    
    group "File Operations"
    (
        button btnLoadGrammar "Load Grammar" width:120 height:25 across:2
        button btnLoadFolder "Load Folder" width:120 height:25
    )
    
    group "Generation Controls"
    (
        button btnPlay "Play" width:60 height:25 enabled:false across:4
        button btnStop "Stop" width:60 height:25 enabled:false
        button btnReset "Reset" width:60 height:25 enabled:false
        button btnStep "Step" width:60 height:25 enabled:false
    )
	
    -- Helper Functions
    fn updateStatusDisplay =
    (
        local stepText = if maxIteration > 0 then "Step: " + iterationCount as string + " / " + maxIteration as string else "Step: " + iterationCount as string
        labelStatus.text = grammarName + " - " + stepText
    )
    
    fn getGrammarDataDirectory =
    (
        -- Try to find the grammar data directory relative to the 3DS Max scene
        local scenePath = maxFilePath
        local grammarDataPath = scenePath + "grammar data\\"
        
        -- If the grammar data folder exists in the scene directory, use it
        if doesFileExist grammarDataPath then
        (
            return grammarDataPath
        )
        
        -- Fallback: try to find it in common locations
        local possiblePaths = #(
            scenePath + "grammar data\\",
            scenePath + "..\\grammar data\\",
            scenePath + "..\\..\\grammar data\\",
            scenePath + "..\\..\\..\\grammar data\\"
        )
        
        for path in possiblePaths do
        (
            if doesFileExist path then
            (
                return path
            )
        )
        
        -- If not found, return the scene directory
        return scenePath
    )
	
    fn handleSizeChange =
    (
        setModelSize sizeX sizeY sizeZ
    )
    
    fn startAnimation =
    (
        if isAnimating then
        (
            stopAnimation()
        )
        
        startTime = timestamp()
        timerActive = true
        isAnimating = true
        btnPlay.enabled = false
        btnStop.enabled = true
        
        -- Start the animation timer
        callbacks.addScript #systemPostNew "animationUpdate()" id:#grammarEditor
    )
    
    fn stopAnimation =
    (
        callbacks.removeScripts id:#grammarEditor
        timerActive = false
        isAnimating = false
        btnPlay.enabled = true
        btnStop.enabled = false
        updateStatusDisplay()
    )
    
    fn resetGeneration =
    (
        resetFunc seed
        iterationCount = 0
        startTime = timestamp()
        timerActive = false
        updateStatusDisplay()
    )
    
    fn iterateSteps steps =
    (
        iterate steps
        iterationCount += steps
        updateMesh()
        updateStatusDisplay()
    )
    
    fn loadGrammarFile filePath =
    (
        local result = initialize filePath seed
        if result == "Success" then
        (
            grammarName = getFilenameFile filePath
            iterationCount = 0
            btnPlay.enabled = true
            btnReset.enabled = true
            btnStep.enabled = true
            updateStatusDisplay()
            format "Loaded grammar: %\n" grammarName
        )
        else
        (
            grammarName = ""
            format "Error loading grammar: %\n" result
        )
    )
    
    fn loadGrammarFolder folderPath =
    (
        -- Get all JSON files in the folder and subfolders
        fileQueue = #()
        local files = getFiles (folderPath + "\\*.json")
        for f in files do append fileQueue f
        
        -- Also search subdirectories
        local subdirs = getDirectories (folderPath + "\\*")
        for dir in subdirs do
        (
            local subfiles = getFiles (dir + "\\*.json")
            for f in subfiles do append fileQueue f
        )
        
        if fileQueue.count == 0 then
        (
            messageBox "No JSON files found in the selected folder." title:"Warning"
            return false
        )
        
        maxIteration = MAX_ITERATION
        loadGrammarFile fileQueue[1]
        deleteItem fileQueue 1
        startAnimation()
        true
    )
    
    fn animationUpdate =
    (
        if not isAnimating then return false
        
        if iterationCount >= maxIteration and maxIteration > 0 then
        (
            if fileQueue.count > 0 then
            (
                iterationCount = 0
                loadGrammarFile fileQueue[1]
                deleteItem fileQueue 1
            )
            else
            (
                stopAnimation()
                return false
            )
        )
        
        local stepsCompleted = iterateToTime 0.1 -- Use 0.1 seconds per frame
        iterationCount += stepsCompleted
        
        updateMesh()
        
        updateStatusDisplay()
        true
    )
    
    fn updateMesh =
    (
        local meshData = getMesh()
        
        -- Copy data into local variables
        local positions = meshData[1]
        local normals = meshData[2]
        local triangles = meshData[3]
        local faceIndices = meshData[4]
        local numVertices = meshData[5]
        local numTriangles = meshData[6]
        local numFaces = meshData[7]
        
        if numVertices > 0 then
        (
            -- Create or find the mesh object
            local meshObj = getNodeByName "Generated Mesh"
            if meshObj == undefined then
            (
                meshObj = Editable_Mesh()
                meshObj.name = "Generated Mesh"
            )
            
            -- Convert the mesh data to 3DS Max format
            local vertices = #()
            local normals = #()
            local faces = #()
            
            -- Process vertices (switch Y and Z coordinates like in Unity)
            for i = 1 to numVertices do
            (
                local posIndex = (i - 1) * 3 + 1
                local x = positions[posIndex]
                local y = positions[posIndex + 2] -- Z becomes Y
                local z = positions[posIndex + 1] -- Y becomes Z
                append vertices [x, y, z]
                
                local normIndex = (i - 1) * 3 + 1
                local nx = normals[normIndex]
                local ny = normals[normIndex + 2] -- Z becomes Y
                local nz = normals[normIndex + 1] -- Y becomes Z
                append normals [nx, ny, nz]
            )
            
            -- Process faces
            for i = 1 to numTriangles do
            (
                local faceIndex = (i - 1) * 3 + 1
                local v1 = triangles[faceIndex] + 1 -- Convert to 1-based indexing
                local v2 = triangles[faceIndex + 1] + 1
                local v3 = triangles[faceIndex + 2] + 1
                append faces [v1, v2, v3]
            )
            
            -- Update the mesh
            meshObj.mesh.vertices = vertices
            meshObj.mesh.faces = faces
            meshObj.mesh.normals = normals
            
            -- Free the mesh data
            destroyMesh meshData
            
            -- Force viewport update
            forceCompleteRedraw()
        )
    )
    
    -- Event Handlers
    on GrammarEditorRollout open do
    (
        -- Load the DLL first
        loadGrammarDLL()
        
        -- Initialize UI state
        spnSeed.value = seed
        spnSizeX.value = sizeX
        spnSizeY.value = sizeY
        spnSizeZ.value = sizeZ
        updateStatusDisplay()
    )
    
    on spnSeed changed val do
    (
        seed = val
        resetFunc seed
    )
    
    on spnSizeX changed val do
    (
        sizeX = val
        handleSizeChange()
    )
    
    on spnSizeY changed val do
    (
        sizeY = val
        handleSizeChange()
    )
    
    on spnSizeZ changed val do
    (
        sizeZ = val
        handleSizeChange()
    )
    
    on spnMaxIteration changed val do
    (
        maxIteration = val
    )
    
    on btnLoadGrammar pressed do
    (
        local defaultPath = getGrammarDataDirectory()
        local filePath = getOpenFileName caption:"Load Grammar" filename:defaultPath types:"JSON Files (*.json)|*.json|All Files (*.*)|*.*"
        if filePath != undefined then
        (
            maxIteration = 0
            loadGrammarFile filePath
        )
    )
    
    on btnLoadFolder pressed do
    (
        local defaultPath = getGrammarDataDirectory()
        local folderPath = getSavePath caption:"Select Grammar Folder" initialDir:defaultPath
        if folderPath != undefined then
        (
            loadGrammarFolder folderPath
        )
    )
    
    on btnPlay pressed do
    (
        startAnimation()
    )
    
    on btnStop pressed do
    (
        stopAnimation()
    )
    
    on btnReset pressed do
    (
        resetGeneration()
    )
    
    on btnStep pressed do
    (
        iterateSteps 1
    )
    
    on btnStep1 pressed do
    (
        iterateSteps 1
    )
    
    on btnStep10 pressed do
    (
        iterateSteps 10
    )
    
    on btnStep100 pressed do
    (
        iterateSteps 100
    )
    
    on GrammarEditorRollout close do
    (
        -- Unload the DLL when closing
        unloadGrammarDLL()
    )
)

-- Global function to create and show the grammar editor window
fn createGrammarEditor =
(
    if GrammarEditorRollout != undefined then
    (
        DestroyDialog GrammarEditorRollout
    )
    CreateDialog GrammarEditorRollout
)

-- Create the grammar editor window
createGrammarEditor() 