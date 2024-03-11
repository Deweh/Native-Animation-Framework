#pragma once

#define SETTINGS_INI_PATH "Data\\F4SE\\Plugins\\NAF.ini"
#define USERDATA_DIR "Data\\NAF\\"s

#define PEVENT_SCENE_START "NAF::SceneStarted"
#define PEVENT_SCENE_FAIL "NAF::SceneFailed"
#define PEVENT_SCENE_END "NAF::SceneEnded"
#define PEVENT_SCENE_END_DATA "NAF::SceneEndedData"
#define PEVENT_SCENE_POS_CHANGE "NAF::ScenePositionChanged"
#define PEVENT_TREE_POS_CHANGE "NAF::TreePositionChanged"

#define LOOKSMENU_1_6_18_CHECKSUM 0x57B3298
#define LOOKSMENU_1_6_20_CHECKSUM 0x59F7081

#define HUD_AddRectangle "AddRectangle"
#define HUD_AddText "AddText"
#define HUD_AddLine "AddLine"
#define HUD_SetAlpha "SetAlpha"
#define HUD_SetElementPosition "SetElementPosition"
#define HUD_RemoveElement "RemoveElement"
#define HUD_MoveElementToBack "MoveElementToBack"
#define HUD_MoveElementToFront "MoveElementToFront"
#define HUD_GetElementWidth "GetElementWidth"
#define HUD_GetElementHeight "GetElementHeight"
#define HUD_SetElementMask "SetElementMask"
#define HUD_ClearElements "ClearElements"

#define IDLE_DELIMITER "|~|"s
#define IDLE_DELIMITER_FORMAT "{}|~|{}"

enum ActorGender : int32_t
{
	Any = -1,
	Male = 0,
	Female = 1
};
