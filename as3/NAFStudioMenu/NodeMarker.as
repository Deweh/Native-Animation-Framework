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
		
		public function SetGizmoVisible(vis:Boolean){
			Tri1_mc.visible = vis;
			Tri2_mc.visible = vis;
			Tri3_mc.visible = vis;
			bGizmoVisible = vis;
			
			if(vis){
				alpha = 1.0;
			} else {
				SetAdjustIcon(0);
				alpha = fDefaultAlpha;
			}
		}
		
		public function onAddedToStage(e:Event){
			stage.addEventListener(MouseEvent.MOUSE_MOVE, this.MouseMove);
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
	}
	
}
