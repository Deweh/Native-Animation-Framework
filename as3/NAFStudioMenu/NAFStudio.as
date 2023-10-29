package  {
	
	import flash.display.MovieClip;
	import flash.display.DisplayObject;
	
	
	public class NAFStudio extends MovieClip {
		
		public var BGSCodeObj:Object;
		public static var CodeObjAvailable:Boolean = false;
		public static var CodeObj:Object;
		
		public var BottomControls_mc:BottomControls;
		public var Gizmo_mc:GizmoControls;
		
		public static function CallNative(f:String, ... args){
			if(CodeObjAvailable && CodeObj.hasOwnProperty(f)){
				CodeObj[f].apply(args);
			}
		}
		
		public static function NotifyClicked(t:String, src:DisplayObject){
			if(CodeObjAvailable){
				CodeObj.NotifyClicked(t, src);
			}
		}
		
		public static function NotifyScrubberPosition(p:Number){
			if(CodeObjAvailable){
				CodeObj.NotifyScrubberPosition(p);
			}
		}
		
		public static function NotifyGizmoMovement(gID:uint, fX:Number, fY:Number){
			if(CodeObjAvailable){
				CodeObj.NotifyGizmoMovement(gID, fX, fY);
			}
		}
		
		public static function NotifyKeyMoved(curPos:uint, newPos:uint){
			if(CodeObjAvailable){
				CodeObj.NotifyKeyMoved(curPos, newPos);
			}
		}
		
		public static function NotifyKeySelected(keyIdx:uint){
			if(CodeObjAvailable){
				CodeObj.NotifyKeySelected(keyIdx);
			}
		}
		
		public static function Log(s:String){
			if(CodeObjAvailable){
				CodeObj.Log(s);
			}
		}
		
		public function NAFStudio() {
			stage.stageFocusRect = false;
			BGSCodeObj = new Object();
			Gizmo_mc.visible = false;
		}
		
		public function SetKeySelected(idx:uint, s:Boolean){
			BottomControls_mc.Timeline_mc.SetKeySelected(idx, s);
		}
		
		public function SetPlayState(p:Boolean){
			BottomControls_mc.PlayPause_mc.SetIsPlay(p);
		}
		
		public function SetScrubberPosition(p:Number){
			BottomControls_mc.SetScrubberPosition(p);
		}
		
		public function SetTimelineSize(s:uint){
			BottomControls_mc.Timeline_mc.SetDataLength(s);
		}
		
		public function SetTimelineKeys(k:Array){
			BottomControls_mc.Timeline_mc.SetKeyData(k);
		}
		
		public function GetStageWidth() : Number{
			return this.stage.stageWidth;
		}
		
		public function GetStageHeight() : Number{
			return this.stage.stageHeight;
		}
		
		public function onCodeObjCreate():* {
			CodeObj = BGSCodeObj;
			CodeObjAvailable = true;
			CallNative("NotifyLoaded");
			Log("NAFStudio Loaded.");
        }
		
		public function CreateNodeMarker(iIndex:uint) : DisplayObject {
			var newMarker:MovieClip = new NodeMarker();
			this.addChildAt(newMarker, 0);
			newMarker.iIndex = iIndex;
			setChildIndex(BottomControls_mc, numChildren - 1);
			return newMarker;
		}
		
		public function DestroyNodeMarker(marker:DisplayObject) : void {
			this.removeChild(marker);
		}
	}
	
}
