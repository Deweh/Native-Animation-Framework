package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.geom.Rectangle;
	
	
	public class DynamicClip extends MovieClip {
		public var bEnabled:Boolean = true;
		public var bDraggable:Boolean = false;
		public var bDragCenter:Boolean = false;
		public var rDragBounds:Rectangle;

		public var fSelectedAlpha:Number = 1.0;
		public var fHoverAlpha:Number = 0.92;
		public var fBaseAlpha:Number = 0.85;
		public var fDisabledAlpha:Number = 0.65;

		public var bMouseOver:Boolean = false;
		public var bDragging:Boolean = false;
		
		public function DynamicClip() {
			addEventListener(Event.ADDED_TO_STAGE, this.AddedToStage);
			alpha = fBaseAlpha;
		}
		
		public function AddedToStage(e:Event){
			alpha = fBaseAlpha;
			addEventListener(MouseEvent.MOUSE_OVER, this.MouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.MouseOut);
			addEventListener(MouseEvent.MOUSE_DOWN, this.MouseDown);
			this.stage.addEventListener(MouseEvent.MOUSE_UP, this.MouseUp);
		}
		
		public function MouseOver(e:MouseEvent){
			bMouseOver = true;

			if(bEnabled && !bDragging){
				alpha = fHoverAlpha;
			}
		}
		
		public function MouseOut(e:MouseEvent){
			bMouseOver = false;

			if(bEnabled && !bDragging){
				alpha = fBaseAlpha;
			}
		}
		
		public function MouseDown(e:MouseEvent){
			if(bEnabled && bDraggable && !bDragging){
				this.startDrag(bDragCenter, rDragBounds);
				bDragging = true;
				this.stage.addEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
				DragBegin();
			}

			if(bEnabled){
				alpha = fSelectedAlpha;
			}

			DownImpl();
		}
		
		public function MouseUp(e:MouseEvent){
			if(bEnabled){
				if(bMouseOver){
					alpha = fHoverAlpha;
				} else {
					alpha = fBaseAlpha;
				}
			}

			if(bDraggable && bDragging){
				this.stopDrag();
				bDragging = false;
				this.stage.removeEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
				DragEnd();
			} else if(bEnabled && bMouseOver) {
				Selected();
			}

			UpImpl();
		}

		public function MouseMove(e:MouseEvent){
			if(bDragging){
				DragUpdate(e);
			}
		}

		public function DownImpl(){}
		public function UpImpl(){}

		public function Selected(){}
		public function DragBegin(){}
		public function DragEnd(){}
		public function DragUpdate(e:MouseEvent){}
	}
	
}
