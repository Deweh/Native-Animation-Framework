package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	
	
	public class DynamicTicker extends ISizeNotifier {

		public var lBtn_mc:TickerButton;
		public var rBtn_mc:TickerButton;
		public var mBtn_mc:TickerSelectionButton;

		public function DynamicTicker(p:DynamicPanel) {
			super(p);
			rBtn_mc.SetButtonText(">>");
			lBtn_mc.SetButtonText("<<");

			lBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onLeftTicker);
			rBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onRightTicker);
			mBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onMainTicker);
		}

		public override function SetElementText(s:String){
			mBtn_mc.SetButtonText(s);
		}
		
		public override function NotifyMaxWidth(w:Number){
			var totalWidth:Number = (rBtn_mc.x + rBtn_mc.width) - lBtn_mc.x;
			var targetOffset:Number = ((w / 2) - (totalWidth / 2)) - lBtn_mc.x;
			
			lBtn_mc.x += targetOffset;
			rBtn_mc.x += targetOffset;
			mBtn_mc.x += targetOffset;
		}

		public function onLeftTicker(e:MouseEvent):void {
			if(!parentContainer.disableInput && NAFMenu.CodeObjAvailable){
				NAFMenu.CodeObj.PlaySound("UIMenuOK");
				NAFMenu.CodeObj.TickerDynamicSelected(index, 0);
			}
		}

		public function onRightTicker(e:MouseEvent):void {
			if(!parentContainer.disableInput && NAFMenu.CodeObjAvailable){
				NAFMenu.CodeObj.PlaySound("UIMenuOK");
				NAFMenu.CodeObj.TickerDynamicSelected(index, 1);
			}
		}

		public function onMainTicker(e:MouseEvent):void {
			if(!parentContainer.disableInput && NAFMenu.CodeObjAvailable){
				NAFMenu.CodeObj.PlaySound("UIMenuOK");
				NAFMenu.CodeObj.TickerDynamicSelected(index, 2);
			}
		}
	}
	
}
