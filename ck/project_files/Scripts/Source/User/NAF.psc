Scriptname NAF extends ScriptObject Native

Struct SceneId
    Int id1 = 0
    Int id2 = 0
EndStruct

Struct StartSceneResult
    Bool successful = False
    Int id1 = 0
    Int id2 = 0
EndStruct

Struct SceneSettings
    String position = ""
    Float duration = 0.0
    ObjectReference positionRef = None
    String includeTags = ""
    String excludeTags = ""
    String requireTags = ""
    Bool forceNPCControlled = False
    Bool ignoreCombat = False
EndStruct

;|-------------------|
;| General Functions |
;|-------------------|

;Opens or closes the main NAF menu
Function ToggleMenu() Native Global

; Get bDisableRescaler setting
Bool Function GetDisableRescaler() Native Global

; Set bDisableRescaler setting (will not persist unless saved via the NAF menu)
Function SetDisableRescaler(Bool bDisable) Native Global

;Returns false if the actor has either the NAF_InScene keyword or NAF_DoNotUse keyword, otherwise true
Bool Function IsActorUsable(Actor akActor) Native Global

;If bUsable is false, adds the NAF_DoNotUse keyword to the actor, otherwise removes the keyword.
;If an actor has the NAF_DoNotUse keyword on them, they will not show up when selecting actors for
;a New Scene in the NAF menu, and any calls to StartScene using that actor will return unsuccessful.
Function SetActorUsable(Actor akActor, Bool bUsable) Native Global

;Overrides all of an actor's other AI packages (including aliases, etc.) and sets their current package to the one provided.
;Almost nothing can make the actor exit the provided package except ClearPackageOverride.
;Returns false if the actor or package is invalid, or if the package override is currently reserved for NAF. (Actor is in a scene or walking to a scene.)
Bool Function SetPackageOverride(Actor akActor, Package akPackage) Native Global

;Clears an actor's AI package override. (Unless the override is currently reserved by NAF for an active scene.)
Function ClearPackageOverride(Actor akActor) Native Global

;Plays a custom animation on the corresponding actor using the provided filename, animation event, and behavior graph.
;
;sFileName should be the file path to a .hkx animation file, starting from the Data/Models folder.
;(For example, if you have a file at Data/Models/Animation.hkx, you would just put Animation.hkx as the file name.)
;
;sAnimEvent is the animation event used by the game to start the animation. On humans it's dyn_ActivationLoop for a
;looping animation, or dyn_Activation for a one-time animation. Other races may use different events. For information
;on those, check what events animation packs use or take a look at those races' anim events in the Creation Kit.
;
;sBehaviorGraph is the path to the actor's animation graph starting from the Data/Models folder. For humans, it's always
;'Actors\Character\Behaviors\RaiderRootBehavior.hkx'. If you leave this as default, NAF will attempt to figure out the
;correct behavior graph path for non-human races, but it might not be able to find it for modded races.
Function PlayDynamicIdle(Actor akActor, String sFileName, String sAnimEvent = "dyn_ActivationLoop", String sBehaviorGraph = "Actors\\Character\\Behaviors\\RaiderRootBehavior.hkx") Native Global

;|-----------------|
;| Scene Functions |
;|-----------------|

;Starts a scene using the provided actors and settings.
;If position name is empty, a random position will be used.
;If duration is less than 0, the scene will run until stopped by the StopScene function or via the NAF menu.
;If akPositionRef is None, the position of the first actor will be used. Otherwise, the position of akPositionRef will be used.
;
;Returns a struct with a bool named "successful" that indicates whether the scene started successfully.
;If successful, the SceneId can be obtained with the GetSceneFromResult function.
StartSceneResult Function StartScene(Actor[] akActors, SceneSettings akSettings = None) Native Global

;Makes actors walk to the position of the scene, then starts it. Register for the scene start event and check for the SceneId
;returned from this function to know when the scene actually starts. Before the scene starts, the SceneId will only work with
;the StopScene() function, which will cancel the walk.
StartSceneResult Function WalkToAndStartScene(Actor[] akActors, SceneSettings akSettings = None) Native Global

;Stops akScene. After this function is called, the SceneId will no longer point to a valid scene.
;If the provided SceneId does not point to any currently running scene or walking instance, nothing will happen.
Function StopScene(SceneId akScene) Native Global

;Sets the position for akScene. Returns true if the scene exists and the position was found, otherwise false.
Bool Function SetScenePosition(SceneId akScene, String akPosition) Native Global

;Returns all actors in akScene. If the scene does not exist, an empty array will be returned.
Actor[] Function GetSceneActors(SceneId akScene) Native Global

;Returns the HKX paths for the current animations playing on each actor in the scene.
;The array is returned in the same order as GetSceneActors.
;Paths are returned as all-lowercase for easy comparison.
String[] Function GetSceneHKXs(SceneId akScene) Native Global

;Returns the tags on the position that the provided scene is currently playing.
;For position trees, this will return the tags on the current node, not the tree itself.
;Tags are returned as all-lowercase.
String[] Function GetSceneTags(SceneId akScene) Native Global

;Sets the speed multipler for akScene. 100.0 is normal speed.
;The speed multiplier only affects animation speed, it does not affect the scene duration.
;If the provided SceneId does not point to any currently running scene, nothing will happen.
Function SetSceneSpeed(SceneId akScene, Float iSpeed) Native Global

;Gets the current speed multipler for akScene. 100.0 is normal speed.
;If the provided SceneId does not point to any currently running scene, 0 will be returned.
Float Function GetSceneSpeed(SceneId akScene) Native Global

;If akActor is in a currently running scene, returns the corresponding SceneId.
;If they are not in a scene, the returned SceneId will not point to any scene.
;Note: This function searches through every running scene to find which one the actor is in.
;For performance, it is better to save the SceneId provided by StartScene whenever possible.
SceneId Function GetSceneFromActor(Actor akActor) Native Global

;Returns the SceneId from a StartSceneResult.
;If the StartSceneResult was not successful, the corresponding SceneId will not point to any scene.
SceneId Function GetSceneFromResult(StartSceneResult akResult) Global
    SceneId id = new SceneId
    id.id1 = akResult.id1
    id.id2 = akResult.id2
    Return id
EndFunction

;Returns true if akScene1 and akScene2 point to the same scene, otherwise false.
Bool Function SceneIdsMatch(SceneId akScene1, SceneId akScene2) Global
    return (akScene1.id1 == akScene2.id1 && akScene1.id2 == akScene2.id2)
EndFunction

;Returns true if akScene points to a currently running scene, otherwise false.
Bool Function IsSceneRunning(SceneId akScene) Native Global

;Returns the value of the specified property from the specified scene. If the scene does not exist, None will be returned.
;List of valid sProperty values:
;"HasStarted":
;   Return Type: Bool
;   Details: If this returns true, then the scene has started and all other properties can be retrieved.
;            If this returns false, then the actors are walking to the scene, and no other properties can be retrieved yet.
;"LocationRef":
;   Return Type: ObjectReference
;   Details: The object reference chosen as this scene's location. If no location was chosen, this will be None.
;"TotalDuration":
;   Return Type: Float
;   Details: The starting duration of the scene, in seconds.
;"RemainingDuration":
;   Return Type: Float
;   Details: The currently remaining duration of the scene, in seconds. When this reaches 0, the scene will end.
;"Status":
;   Return Type: Int
;   Details: The internal status of the scene.
;   Values:
;       0 = Initializing (not yet fully started),
;       1 = Active,
;       2 = PendingDeletion (will be deleted once the scene finishes shutting down & all SceneEnd handlers return),
;       3 = Ending (waiting for the current position to end, then scene will be marked for deletion)
;"SyncStatus":
;   Return Type: Int
;   Details: The animation synchronization status for the scene.
;   Values:
;       0 = Synced (all animations are synchronized),
;       1 = SettingUp (waiting for actor graphs to finish any in-progress transitions)
;       2 = WaitingForLoad (waiting for all the animations to load into memory)
;       3 = SyncingTimes (the animations are currently synchronizing)
;"AnimationTime":
;   Return Type: Float
;   Details: The current time of the scene's synchronized animation, in seconds.
;            This value may be incorrect if SyncStatus is not 0.
;"StartEquipSet":
;   Return Type: String
;   Details: The ID of the start equipment set that the scene was started with.
;"StopEquipSet":
;   Return Type: String
;   Details: The ID of the stop equipment set that the scene was started with. Will be applied when the scene ends.
;"AutoAdvance":
;   Return Type: Bool
;   Details: Value of the forceNPCControlled setting.
;"IgnoreCombat":
;   Return Type: Bool
;   Details: Value of the ignoreCombat setting.
;"PositionType":
;   Return Type: String
;   Details: A descriptor of the scene's current position type.
;"AnimationID":
;   Return Type: String
;   Details: The ID of the scene's current animation.
;"PositionID":
;   Return Type: String
;   Details: The ID of the scene's current position.
;"Location":
;   Return Type: Var[] (use Utility.VarToVarArray) with 3 Floats
;   Details: The XYZ location of the scene. The array is mapped to X = index 0, Y = index 1, Z = index 2
;"Angle":
;   Return Type: Var[] (use Utility.VarToVarArray) with 3 Floats
;   Details: The XYZ rotation of the scene. The array is mapped to X = index 0, Y = index 1, Z = index 2
Var Function GetSceneProperty(SceneId akScene, String sProperty) Native Global

;Checks if the provided scene parameters have any potential issues that would cause a scene to not start.
;If any problems were found, returns a string with a message describing the problem.
;Otherwise, returns an empty string.
String Function ValidateSceneParams(Actor[] akActors, SceneSettings akSettings = None) Native Global

;If the player's input is currently disabled by a NAF scene, this function force-enables it.
Function ReEnablePlayerInput() Native Global

;|--------------------------|
;| Body Animation Functions |
;|--------------------------|

;Plays a .nanim animation on an actor. Any animation created with the NAF Animation Studio is a .nanim animation.
;akActor - The actor to play the animation on.
;sFilePath - The file path of the .nanim file, starting from the Data/NAF folder. If an animation is at Data/NAF/myAnim.nanim,
;you would just put myAnim.nanim
;sAnimID - The animation ID to play from the .nanim file. For animations baked from the NAF Animation Studio, this will always
;be "default".
;fTransitionTime - The duration of the transition between the current animation and the new animation, in seconds.
;Returns false if the actor is invalid or their race has no configured GraphInfo, otherwise true.
Bool Function PlayNANIM(Actor akActor, String sFilePath, String sAnimID = "default", Float fTransitionTime = 1.0) Native Global

;Stops any .nanim animation currently playing on an actor.
;akActor - The actor to stop any animations on.
;fTransitionTime - The duration of the transition between the current animation and the new animation, in seconds.
;Returns false if there is no animation currently playing on the actor, otherwise true.
Bool Function StopNANIM(Actor akActor, Float fTransitionTime = 1.0) Native Global

;Enables an IK chain on an actor and sets the effector to the current position and rotation of akTarget.
;akActor - The actor to enable the chain on.
;sChainName - The ID of the IK chain, as configured in a GraphInfo XML. i.e., "RArm" for right arm on humans.
;akTarget - The object reference to use as the target.
Function SetIKChainTarget(Actor akActor, String sChainName, ObjectReference akTarget) Native Global

;Enables or disables an IK chain on an actor. Disabling will clear any target set on the chain.
;akActor - The actor to enable/disable the chain on.
;sChainName - The ID of the IK chain to enable/disable, as configured in a GraphInfo XML. i.e., "RArm" for right arm on humans.
;bEnabled - Whether to enable or disable the chain.
Function SetIKChainEnabled(Actor akActor, String sChainName, Bool bEnabled) Native Global

;|--------------------------|
;| Face Animation Functions |
;|--------------------------|

;Overrides the eye coordinates of the corresponding actor. fEyeX = left and right, fEyeY = up and down.
;1.0 is all the way in one direction, -1.0 is all the way in the other direction.
;The eyes will stay in this position until SetEyeCoordOverride is called again with different values, or
;ClearEyeCoordOverride is called.
Function SetEyeCoordOverride(Actor akActor, Float fEyeX, Float fEyeY) Native Global

;Clears the eye coordinate override for the corresponding actor, returning control to the face animation system.
;If the actor has no eye coordinate override active, nothing will happen.
Function ClearEyeCoordOverride(Actor akActor) Native Global

;Plays akAnimationId face animation on akActor. Returns false if no such animation could be found or the actor is None, otherwise true.
;
;If bLoop is set to True, the face animation will loop until stopped by StopFaceAnimation.
;
;If bBodySync is set to True, NAF will attempt to sync the face animation to the havok animation currently playing on the actor.
;With bBodySync set to True, the animation will not stop until StopFaceAnimation is called.
;
;Note: If the actor has an active eye coord override, that will override any eye coord keyframes in the animation.
Bool Function PlayFaceAnimation(Actor akActor, String akAnimationId, Bool bLoop = False, Bool bBodySync = False) Native Global

;Stops any NAF face animation currently playing on akActor, returning control to the game's animation system.
;Note: If the actor has an active eye coord override, this function will not clear that.
Function StopFaceAnimation(Actor akActor) Native Global

;|---------------|
;| HUD Functions |
;|---------------|
;These functions can draw & manipulate elements on the NAF HUD,
;which is visible whenever the main game HUD is visible.
;The NAF HUD is 1080 pixels in height and 1920 pixels in width. Coordinates & sizes are in pixels.
;Colors are hexadecimal, i.e. 0xFFFFFF for pure white and 0x000000 for pure black.
;White is replaced with the user-set HUD color. (Green by default.)
;By default, elements that take up the same screen space will be layered based on the order they were originally drawn.

;Draws a rectangle to the NAF HUD and returns a handle that can be used to manipulate the rectangle.
Int Function DrawRectangle(Float fX, Float fY, Float fWidth, Float fHeight, Int iColor = 0xFFFFFF, Float fAlpha = 1.0, Float fBorderSize = 0.0, Int iBorderColor = 0xFFFFFF, Float fBorderAlpha = 0.0) Native Global

;Draws text to the NAF HUD and returns a handle that can be used to manipulate the text.
Int Function DrawText(String akText, Float fX, Float fY, Float fTxtSize = 18.0, Int iColor = 0xFFFFFF, Float fAlpha = 1.0, Bool bBold = False, Bool bItalic = False, Bool bUnderline = False) Native Global

;Draws a line to the NAF HUD and returns a handle that can be used to manipulate the line.
Int Function DrawLine(Float fStartX, Float fStartY, Float fEndX, Float fEndY, Float fThickness = 1.0, Int iColor = 0xFFFFFF, Float fAlpha = 1.0) Native Global

;Sets an element's position using its handle.
Function SetElementPosition(Int iHandle, Float fX, Float fY) Native Global

;Translates an element from its current position to the end position over fSeconds.
;Note: An element's position can not be set while it is being translated.
Function TranslateElementTo(Int iHandle, Float fEndX, Float fEndY, Float fSeconds) Native Global

;Stops any active translation on an element by its handle.
;If the element is currently translating, it will immediately be popped to the end position of the translation.
Function StopElementTranslation(Int iHandle) Native Global

;Removes an element from the NAF HUD. The element's handle can no longer be used after it has been removed.
Function RemoveElement(Int iHandle) Native Global

;Gets the full width of an element in pixels. (Can be used to measure the width of text.)
;If the handle is invalid, 0 will be returned.
Float Function GetElementWidth(Int iHandle) Native Global

;Gets the full height of an element in pixels. (Can be used to measure the height of text.)
;If the handle is invalid, 0 will be returned.
Float Function GetElementHeight(Int iHandle) Native Global

;Moves an element to the top layer by its handle.
Function MoveElementToFront(Int iHandle) Native Global

;Moves an element to the bottom layer by its handle.
Function MoveElementToBack(Int iHandle) Native Global

;Sets iMaskHandle element as the mask for iHandle element. This will make iMaskHandle element invisible,
;and make it so that iHandle element is only visible where iMaskHandle was previously visible.
;If iMaskHandle is an invalid handle OR bRemove is set to True, iMaskHandle will be ignored
;and the current mask for iHandle will be removed.
Function SetElementMask(Int iHandle, Int iMaskHandle, Bool bRemove = False) Native Global

;Attaches iHandle element to iTargetHandle element.
;Once attached, iHandle element will move with iTargetHandle element.
;Useful for translating or changing the position of many elements at once,
;while keeping the current layout.
;
;Note: Attached elements can be chained. Meaning if A is attached to B, and B is attached to C,
;A & B will both move when C moves. However, there is a chain depth limit of 100 elements to
;maintain performance.
;If iTargetHandle is an invalid handle OR bDetach is set to true, iTargetHandle will be ignored
;and iHandle element will be detached from whatever element it's currently attached to.
Function AttachElementTo(Int iHandle, Int iTargetHandle, Bool bDetach = False) Native Global

;|-------------------|
;| Utility Functions |
;|-------------------|

;Returns the local rotation and position of akObject, relative to akParent.
;First 3 floats in the array are rotation, and last 3 floats are position.
;If either object reference is None, or either object reference's 3d is not loaded, all zeros will be returned.
Float[] Function GetLocalTransform(ObjectReference akObject, ObjectReference akParent) Native Global

;Synchronizes the animation times of all provided actors.
;For example, if the first actor is 2 seconds into playing their animation,
;and the second actor is 3 seconds into playing their animation, this function will instantly set them both to 2 seconds.
;Works for both normal game animations & NANIM animations.
;
;Returns true if successful, or false if there was an error with one or more of the actors.
Bool Function SynchronizeAnimations(Actor[] akActors) Native Global

;|--------------|
;| Scene Events |
;|--------------|

;Event for when a scene is started.
;Parameters: (NAF:SceneId akScene)
;Register for this event using RegisterForExternalEvent(NAF.SceneStartEvent(), YourFunctionName)
String Function SceneStartEvent() Global
    Return "NAF::SceneStarted"
EndFunction

;Event for when a scene fails to start. (Useful for checking if the scene actually started when using WalkToAndStartScene)
;Parameters: (NAF:SceneId akScene)
;Register for this event using RegisterForExternalEvent(NAF.SceneStartEvent(), YourFunctionName)
String Function SceneFailEvent() Global
    Return "NAF::SceneFailed"
EndFunction

;Event for when a scene is ended.
;Parameters: (NAF:SceneId akScene)
;Register for this event using RegisterForExternalEvent(NAF.SceneEndEvent(), YourFunctionName)
String Function SceneEndEvent() Global
    Return "NAF::SceneEnded"
EndFunction

;Same as SceneEndEvent, but passes data about the ended scene as extra parameters.
;Parameters: (NAF:SceneId akScene, Actor[] akActors)
;Register for this event using RegisterForExternalEvent(NAF.SceneEndDataEvent(), YourFunctionName)
String Function SceneEndDataEvent() Global
    Return "NAF::SceneEndedData"
EndFunction

;Event for when a scene's position is changed.
;Parameters: (NAF:SceneId akScene, String akNewPosition)
;Register for this event using RegisterForExternalEvent(NAF.ScenePositionChangeEvent(), YourFunctionName)
String Function ScenePositionChangeEvent() Global
    Return "NAF::ScenePositionChanged"
EndFunction

;Event for when a position tree's sub-position is changed.
;Parameters: (NAF:SceneId akScene, String akNewPosition, String akTreePosition)
;Register for this event using RegisterForExternalEvent(NAF.TreePositionChangeEvent(), YourFunctionName)
String Function TreePositionChangeEvent() Global
    Return "NAF::TreePositionChanged"
EndFunction