package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.geom.Rectangle;
	
	
	public class Keyframe extends DynamicClip {
		
		public var tlParent:Timeline = null;
		public var Glow_mc:MovieClip;
		
		public function Keyframe() {
			this.fBaseAlpha = 0.65;
			this.fHoverAlpha = 0.9;
			this.fSelectedAlpha = 1.0;
			this.bDraggable = true;
			Glow_mc.visible = false;
			UpdateDragBounds();
		}
		
		public function UpdateDragBounds(){
			this.rDragBounds = new Rectangle(0, 0, 1920 - this.width, 0);
		}
		
		public function SetSelected(s:Boolean){
			Glow_mc.visible = s;
		}
		
		override public function DragEnd(){
			tlParent.OnKeyDragEnd(this);
		}
	}
	
}
