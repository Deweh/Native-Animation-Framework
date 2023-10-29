package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.geom.Rectangle;
	
	
	public class GizmoMover extends DynamicClip {

		public var fLastX:Number = 0;
		public var fLastY:Number = 0;
		public var sGizmoID:String = "Default";
		
		public function GizmoMover() {
			NAFStudio.Log("Constructing GizmoMover...");
			fBaseAlpha = 0.40;
			NAFStudio.Log("Set fBaseAlpha to 0.40");
		}

		override public function DownImpl(){
			NAFStudio.Log("GizmoMover DownImpl");
			if(bEnabled && !bDragging){
				NAFStudio.Log("GizmoMover DownImpl Enter");
				this.stage.addEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
				NAFStudio.Log("GizmoMover DownImpl Registered");
				bDragging = true;
				NAFStudio.Log("GizmoMover DownImpl Dragging");
				NAFStudio.CallNative("StartGizmoCursor");
			}
		}
		
		override public function UpImpl(){
			NAFStudio.Log("GizmoMover UpImpl");
			if(bEnabled && bDragging){
				NAFStudio.Log("GizmoMover UpImpl Enter");
				this.stage.removeEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
				NAFStudio.Log("GizmoMover UpImpl Remove");
				bDragging = false;
				NAFStudio.Log("GizmoMover UpImpl Not Dragging");
				NAFStudio.CallNative("StopGizmoCursor");
			}
		}
		
		override public function DragUpdate(e:MouseEvent){
			var difX:Number = e.localX - fLastX;
			var difY:Number = e.localY - fLastY;
			fLastX = e.localX;
			fLastY = e.localY;
		}
	}
	
}
