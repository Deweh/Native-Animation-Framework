package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.display.DisplayObject;
	import flash.events.Event;
	
	
	public class NodeMarker extends MovieClip {
		public var fLastX:Number = 0;
		public var fLastY:Number = 0;		
		
		public var bMarkedVisible:Boolean = true;
		public var bEnabled:Boolean = true;
		public var fDefaultAlpha:Number = 0.7;
		public var iIndex:uint = 0;
		
		public var Tri1_mc:GizmoTriangle;
		public var Tri2_mc:GizmoTriangle;
		public var Tri3_mc:GizmoTriangle;
		
		public var bDraggingGizmo:Boolean = false;
		public var iGizmoID:uint = 0;
		public var bGizmoVisible = false;
		public var sClickType:String = "NodeMarker";
		
		public var NodeIco_mc:MovieClip;
		public var EffectorIco_mc:MovieClip;
		public var PoleIco_mc:MovieClip;
		
		public var RotateIco_mc:MovieClip;
		public var MoveIco_mc:MovieClip;
		
		public var iIconType:int = 0;
		
		public var fGizmoXX:Number = 0;
		public var fGizmoXY:Number = 0;
		public var fGizmoYX:Number = 0;
		public var fGizmoYY:Number = 0;
		public var fGizmoZX:Number = 0;
		public var fGizmoZY:Number = 0;
		
		public var fGizmoThickness:Number = 28;
		public var fGizmoAlpha:Number = 0.75;
		
		public function NodeMarker() {
			EffectorIco_mc.visible = false;
			PoleIco_mc.visible = false;
			this.alpha = fDefaultAlpha;
			Tri1_mc.visible = false;
			Tri2_mc.visible = false;
			Tri3_mc.visible = false;
			Tri1_mc.iGizmoID = 0;
			Tri2_mc.iGizmoID = 1;
			Tri3_mc.iGizmoID = 2;
			Tri1_mc.mParent = this;
			Tri2_mc.mParent = this;
			Tri3_mc.mParent = this;
			
			addEventListener(MouseEvent.MOUSE_OVER, this.onMouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.onMouseOut);
			addEventListener(MouseEvent.MOUSE_UP, this.onMouseClick);
			addEventListener(MouseEvent.MOUSE_DOWN, this.onMouseDown);
			addEventListener(Event.ADDED_TO_STAGE, this.onAddedToStage);
			
			RotateIco_mc.visible = false;
			MoveIco_mc.visible = false;
		}
		
		public function SetAdjustIcon(t:int){
			RotateIco_mc.visible = false;
			MoveIco_mc.visible = false;
			
			if(t == 1){
				RotateIco_mc.visible = true;
			} else if (t == 2){
				MoveIco_mc.visible = true;
			}
		}
		
		public function SetIconType(t:int){
			EffectorIco_mc.visible = false;
			PoleIco_mc.visible = false;
			NodeIco_mc.visible = false;
			iIconType = t;
			
			if(t == 1){
				PoleIco_mc.visible = true;
			} else if (t == 2){
				EffectorIco_mc.visible = true;
				
				//Effectors should always be on top.
				parent.setChildIndex(this, parent.numChildren - 1);
			} else {
				NodeIco_mc.visible = true;
			}
		}
		
		public function SetGizmoPoints(
		pXx:Number, pXy:Number,
		pYx:Number, pYy:Number,
		pZx:Number, pZy:Number)
		{
			if(bGizmoVisible){
				fGizmoXX = pXx;
				fGizmoXY = pXy;
				fGizmoYX = pYx;
				fGizmoYY = pYy;
				fGizmoZX = pZx;
				fGizmoZY = pZy;
				graphics.clear();
				
				if(bDraggingGizmo && iGizmoID == 0){
					graphics.lineStyle(fGizmoThickness, 0xFFFFFF);
				} else {
					graphics.lineStyle(fGizmoThickness, 0x858585, fGizmoAlpha);
				}
				graphics.moveTo(0, 0);
				graphics.lineTo(pXx, pXy);
				
				if(bDraggingGizmo && iGizmoID == 1){
					graphics.lineStyle(fGizmoThickness, 0xFFFFFF);
				} else {
					graphics.lineStyle(fGizmoThickness, 0x969696, fGizmoAlpha);
				}
				graphics.moveTo(0, 0);
				graphics.lineTo(pYx, pYy);
				
				if(bDraggingGizmo && iGizmoID == 2){
					graphics.lineStyle(fGizmoThickness, 0xFFFFFF);
				} else {
					graphics.lineStyle(fGizmoThickness, 0xADADAD, fGizmoAlpha);
				}
				graphics.moveTo(0, 0);
				graphics.lineTo(pZx, pZy);
				
				graphics.lineStyle(0, 0, 0);
				graphics.beginFill(0, 0.0);
				graphics.drawRect(-1000, -1000, 2000, 2000);
			}
		}
		
		public function SetGizmoVisible(vis:Boolean){
			//Tri1_mc.visible = vis;
			//Tri2_mc.visible = vis;
			//Tri3_mc.visible = vis;
			bGizmoVisible = vis;
			
			if(vis){
				EffectorIco_mc.visible = false;
				PoleIco_mc.visible = false;
				NodeIco_mc.visible = false;
				alpha = 1.0;
			} else {
				SetIconType(iIconType);
				SetAdjustIcon(0);
				alpha = fDefaultAlpha;
			}
			graphics.clear();
		}
		
		public function onAddedToStage(e:Event){
			stage.addEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
			stage.addEventListener(MouseEvent.MOUSE_UP, this.onMouseUpGlobal);
		}
		
		public function GizmoStart(gID:uint, fX:Number, fY:Number){
			iGizmoID = gID;
			bDraggingGizmo = true;
			fLastX = fX;
			fLastY = fY;
			NAFStudio.CallNative("StartGizmoCursor");
		}
		
		public function GizmoStop(){
			bDraggingGizmo = false;
			NAFStudio.CallNative("StopGizmoCursor");
		}
		
		public function MouseMove(e:MouseEvent){
			if(bDraggingGizmo){
				var difX:Number = e.stageX - fLastX;
				var difY:Number = e.stageY - fLastY;
				fLastX = e.stageX;
				fLastY = e.stageY;
				NAFStudio.NotifyGizmoMovement(iGizmoID, difX, difY);
			}
		}
		
		public function onMouseOver(e:MouseEvent):void {
			if(bEnabled && !bGizmoVisible){
				this.alpha = 1.0;
				var tObj:DisplayObject = this;
				NAFStudio.CallNative("NotifyHoverChanged", sClickType, true, tObj);
			}
		}
		
		public function onMouseOut(e:MouseEvent):void {
			if(!bGizmoVisible){
				this.alpha = fDefaultAlpha;
				var tObj:DisplayObject = this;
				NAFStudio.CallNative("NotifyHoverChanged", sClickType, false, tObj);
			}
		}
		
		public function onMouseClick(e:MouseEvent):void {
			NAFStudio.NotifyClicked(sClickType, this);
		}
		
		public function onMouseDown(e:MouseEvent):void {
			if(bGizmoVisible){
				if(isPointInLine(0, 0, fGizmoXX, fGizmoXY, fGizmoThickness * 2, e.localX, e.localY)){
					GizmoStart(0, e.stageX, e.stageY);
				} else if(isPointInLine(0, 0, fGizmoYX, fGizmoYY, fGizmoThickness * 2, e.localX, e.localY)){
					GizmoStart(1, e.stageX, e.stageY);
				} else if(isPointInLine(0, 0, fGizmoZX, fGizmoZY, fGizmoThickness * 2, e.localX, e.localY)){
					GizmoStart(2, e.stageX, e.stageY);
				}
			}
		}
		
		public function onMouseUpGlobal(e:MouseEvent):void {
			if(bDraggingGizmo){
				GizmoStop();
			}
		}
		
		function isPointInLine(x1:Number, y1:Number, x2:Number, y2:Number, thickness:Number, testX:Number, testY:Number):Boolean {
			var distance:Number = pointToLineDistance(x1, y1, x2, y2, testX, testY);
			return distance <= thickness / 2;
		}

		function pointToLineDistance(x1:Number, y1:Number, x2:Number, y2:Number, testX:Number, testY:Number):Number {
			var A:Number = testX - x1;
			var B:Number = testY - y1;
			var C:Number = x2 - x1;
			var D:Number = y2 - y1;

			var dot:Number = A * C + B * D;
			var lenSq:Number = C * C + D * D;
			var param:Number = dot / lenSq;

			var closestX:Number, closestY:Number;

			if (param < 0 || (x1 == x2 && y1 == y2)) {
				closestX = x1;
				closestY = y1;
			} else if (param > 1) {
				closestX = x2;
				closestY = y2;
			} else {
				closestX = x1 + param * C;
				closestY = y1 + param * D;
			}

			var dx:Number = testX - closestX;
			var dy:Number = testY - closestY;
			return Math.sqrt(dx * dx + dy * dy);
		}
	}
	
}
