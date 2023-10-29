package  {
	
	import flash.display.MovieClip;
	import flash.display.Shape;
	import flash.events.Event;
	import flash.events.MouseEvent;
	
	public class Timeline extends MovieClip {

		public var iScrollIndex:uint = 0;
		public var iKeysDisplayed:uint = 120;
		
		public var vKeyHolders:Vector.<KeyHolder> = new Vector.<KeyHolder>();
		public var fKeyWidth:Number = 0;
		public var linesShape:Shape = new Shape();
		
		public function Timeline() {
			addEventListener(MouseEvent.MOUSE_DOWN, this.onMouseDown);
			addEventListener(Event.ADDED_TO_STAGE, this.onAddedToStage);
			addChild(linesShape);
			linesShape.x = 0;
			linesShape.y = 0;
		}
		
		public function SetKeySelected(idx:uint, s:Boolean){
			if(idx < vKeyHolders.length && vKeyHolders[idx].kObj != null){
				vKeyHolders[idx].kObj.SetSelected(s);
			}
		}
		
		public function KeyToPos(p:Number) : Number {
			return p * fKeyWidth;
		}
		
		public function GetKeyNumber(p:Number) : Number{
			return p / fKeyWidth;
		}
		
		public function OnKeyDragEnd(k:Keyframe){
			var kObj:Keyframe = null;
			var oldSlot:uint = 0;
			
			for(var i:uint = 0; i < vKeyHolders.length; i++){
				if(vKeyHolders[i].kObj == k){
					kObj = vKeyHolders[i].kObj;
					vKeyHolders[i].kObj = null;
					oldSlot = i;
					break;
				}
			}
			
			if(kObj == null){
				return;
			}
			
			var destSlot:Number = Math.round(kObj.x / fKeyWidth);

			if(destSlot == oldSlot){
				vKeyHolders[oldSlot].kObj = kObj;
				NAFStudio.NotifyKeySelected(oldSlot);
				return;
			}
			
			var swapSlot:Number = destSlot;
			
			while(vKeyHolders[swapSlot].kObj != null){
				if(swapSlot == 0){
					swapSlot = vKeyHolders.length - 1;
				} else {
					swapSlot = swapSlot - 1;
				}
			}
			
			if(vKeyHolders[destSlot].kObj != null){
				vKeyHolders[swapSlot].kObj = vKeyHolders[destSlot].kObj;
				NAFStudio.NotifyKeyMoved(destSlot as uint, swapSlot as uint);
			}
			
			vKeyHolders[destSlot].kObj = kObj;
			NAFStudio.NotifyKeyMoved(oldSlot as uint, destSlot as uint);
			DoUpdate();
		}
		
		public function ClearKeyHolders(){
			for(var i:uint = 0; i < vKeyHolders.length; i++){
				if(vKeyHolders[i].kObj != null){
					removeChild(vKeyHolders[i].kObj);
					vKeyHolders[i].kObj = null;
				}
			}
		}
		
		public function SetKeyData(d:Array){
			ClearKeyHolders();
			if(d.length != vKeyHolders.length){
				NAFStudio.Log("Key data length does not match specified size!");
				return;
			}
			for(var i:uint = 0; i < vKeyHolders.length; i++){
				if(d[i] == true){
					vKeyHolders[i].kObj = new Keyframe();
				}
			}
			DoUpdate();
		}
		
		public function SetDataLength(dataLength:uint){
			ClearKeyHolders();
			vKeyHolders.length = 0;
			for(var i:uint = 0; i < dataLength; i++){
				vKeyHolders.push(new KeyHolder());
			}
			iKeysDisplayed = dataLength;
		}
		
		public function DoUpdate(){
			if(vKeyHolders.length <= iKeysDisplayed){
				iScrollIndex = 0;
			}
			
			if(iScrollIndex > vKeyHolders.length - iKeysDisplayed){
				iScrollIndex = vKeyHolders.length - iKeysDisplayed;
			} else if(iScrollIndex < 0){
				iScrollIndex = 0;
			}
			
			UpdateKeysDisplayed();
		}
		
		public function UpdateKeysDisplayed(){
			fKeyWidth = 1920 / iKeysDisplayed;
			
			linesShape.graphics.clear();			
			linesShape.graphics.lineStyle(1, 0x404040, 0.5);
			
			for(var i:uint = 0; i < iKeysDisplayed; i++){
				linesShape.graphics.moveTo(i * fKeyWidth, 25);
				linesShape.graphics.lineTo(i * fKeyWidth, 0);
			}
			
			for(i = 0; i < vKeyHolders.length; i++){
				
				if(vKeyHolders[i].kObj != null){
					if(i >= iScrollIndex && i < iScrollIndex + iKeysDisplayed){
						var kObj = vKeyHolders[i].kObj;
						kObj.tlParent = this;
						kObj.y = 0;
						kObj.x = ((i - iScrollIndex) * fKeyWidth) + 3.5;
						kObj.height = 25;
						kObj.width = fKeyWidth - 6
						addChild(kObj);
						kObj.UpdateDragBounds();
					} else {
						removeChild(kObj);
					}
				}
			}
		}
		
		public function onMouseDown(e:MouseEvent){
			//iKeysDisplayed = 10;
			//UpdateKeysDisplayed();
		}
		
		public function onAddedToStage(e:Event){
			DoUpdate();
		}
	}
	
}
