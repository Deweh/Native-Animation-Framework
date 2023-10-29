package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.geom.Rectangle;
	
	
	public class Scrubber extends DynamicClip {
		
		public var bcParent:BottomControls = null;

		public function Scrubber() {
			bDraggable = true;
			bDragCenter = true;
			rDragBounds = new Rectangle(0, -28, 1920, 0);
		}
		
		override public function DragBegin(){
			NotifyParentOfPos();
		}
		
		override public function DragEnd(){
			NotifyParentOfPos();
		}
		
		override public function DragUpdate(e:MouseEvent){
			NotifyParentOfPos();
		}
		
		public function NotifyParentOfPos(){
			if(bcParent != null){
				bcParent.ScrubberPositionChanged(GetPositionNormal());
			}
		}
		
		public function SetPositionNormal(p:Number){
			x = p;
			/*
			x = p * 1920;
			*/
		}
		
		public function GetPositionNormal():Number{
			/*
			var res:Number = 0;
			if(x > 0){
				res = x / 1920.0;
			}
			return res;
			*/
			return x;
		}
	}
	
}
