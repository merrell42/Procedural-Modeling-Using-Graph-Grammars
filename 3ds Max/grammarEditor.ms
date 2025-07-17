-- Grammar Editor for 3DS Max
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
        local grammarDataPath = scenePath + "grammar data"
        
        -- If the grammar data folder exists in the scene directory, use it
        if doesFileExist grammarDataPath then
        (
            return grammarDataPath
        )
        
        -- Fallback: try to find it in common locations
        local possiblePaths = #(
            scenePath + "grammar data",
            scenePath + "..\\grammar data\\",
            scenePath + "..\\..\\grammar data",
            scenePath + "..\\..\\..\\grammar data"
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
        -- TODO: Call DLL setSize function when implemented
        -- setSize(sizeX, sizeY, sizeZ)
        format "Size changed to: [%, %, %]\n" sizeX sizeY sizeZ
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
        -- TODO: Call DLL reset function when implemented
        -- reset(seed)
        iterationCount = 0
        startTime = timestamp()
        timerActive = false
        updateStatusDisplay()
        format "Reset generation with seed: %\n" seed
    )
    
    fn iterateSteps steps =
    (
        -- TODO: Call DLL iterate function when implemented
        -- iterate(steps)
        iterationCount += steps
        updateStatusDisplay()
        format "Iterated % steps\n" steps
    )
    
    fn loadGrammarFile filePath =
    (
        -- TODO: Call DLL initialize function when implemented
        -- local result = initialize(filePath, seed)
        -- if result == "Success" then
        (
            grammarName = getFilenameFile filePath
            iterationCount = 0
            btnPlay.enabled = true
            btnReset.enabled = true
            btnStep.enabled = true
            updateStatusDisplay()
            format "Loaded grammar: %\n" grammarName
        )
        -- else
        -- (
        --     grammarName = ""
        --     format "Error loading grammar: %\n" result
        -- )
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
        
        -- TODO: Call DLL iterateToTime function when implemented
        -- local stepsCompleted = iterateToTime(spnTimePerFrame.value)
        local stepsCompleted = 1 -- Placeholder
        iterationCount += stepsCompleted
        
        -- TODO: Call updateMesh function when implemented
        -- updateMesh()
        
        updateStatusDisplay()
        true
    )
    
    -- Event Handlers
    on GrammarEditorRollout open do
    (
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
        -- TODO: Call DLL reset function when implemented
        -- reset(seed)
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