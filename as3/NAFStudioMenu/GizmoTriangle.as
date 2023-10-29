package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	
	
	public class GizmoTriangle extends MovieClip {
		public var bMouseOver:Boolean = false;
		public var bSelected:Boolean = false;
		public var iGizmoID:uint = 0;
		
		public var mParent:NodeMarker = null;
		
		public function GizmoTriangle() {
			alpha = 0.6;
			addEventListener(MouseEvent.MOUSE_OVER, this.MouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.MouseOut);
			addEventListener(MouseEvent.MOUSE_DOWN, this.MouseDown);
			addEventListener(Event.ADDED_TO_STAGE, this.AddedToStage);
		}
		
		public function AddedToStage(e:Event){
			stage.addEventListener(MouseEvent.MOUSE_UP, this.MouseUp);
		}
		
		public function MouseOver(e:MouseEvent){
			bMouseOver = true;
			if(!bSelected){
				alpha = 0.75;
			}
		}
		
		public function MouseOut(e:MouseEvent){
			bMouseOver = false;
			if(!bSelected){
				alpha = 0.6;
			}
		}
		
		public function MouseDown(e:MouseEvent){
			bSelected = true;
			alpha = 0.9;
			mParent.GizmoStart(iGizmoID, e.stageX, e.stageY);
		}
		
		public function MouseUp(e:MouseEvent){
			if(bSelected){
				if(bMouseOver){
					alpha = 0.75;
				} else {
					alpha = 0.6;
				}
				
				bSelected = false;
				mParent.GizmoStop();
			}
		}
	}
	
}
