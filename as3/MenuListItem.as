package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.events.MouseEvent;
	import flash.events.TimerEvent;
	import flash.ui.Keyboard;
	import flash.events.KeyboardEvent;
	import flash.utils.Timer;
	
	
	public class MenuListItem extends MovieClip {
		
		public var item_tf:TextField;
		public var background_mc:MovieClip;
		public var index:int = 0;
		public var parentMenu:MenuList;
		public var isBold:Boolean = false;
		public var isHovered:Boolean = false;
		public var isTicker:Boolean = false;

		public var leftTickerBtn_mc:TickerButton;
		public var rightTickerBtn_mc:TickerButton;
		public var mainTickerBtn_mc:TickerSelectionButton;
		
		public var inputTimer:Timer = new Timer(650)
		public var inputFirst:Boolean = true;
		public var inputType:int = -1;
		
		public function MenuListItem(prntMenu:MenuList) {
			this.parentMenu = prntMenu;
			
			addEventListener(MouseEvent.MOUSE_OVER, this.onMouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.onMouseOut);
			addEventListener(MouseEvent.MOUSE_UP, this.onMouseClick);
			rightTickerBtn_mc.SetButtonText(">>");
			leftTickerBtn_mc.SetButtonText("<<");
			leftTickerBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onLeftTicker);
			rightTickerBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onRightTicker);
			leftTickerBtn_mc.addEventListener(MouseEvent.MOUSE_DOWN, this.onLeftTicker);
			rightTickerBtn_mc.addEventListener(MouseEvent.MOUSE_DOWN, this.onRightTicker);
			mainTickerBtn_mc.addEventListener(MouseEvent.MOUSE_UP, this.onMainTicker);
			
			inputTimer.addEventListener("timer", OnInputTimer);
		}
		
		public function setBold(doBold:Boolean):void {
			isBold = doBold;
			
			if(doBold){
				item_tf.textColor = 0x000000;
				background_mc.visible = true;
				background_mc.alpha = 1.0;
				leftTickerBtn_mc.SetTextBlack(true);
				rightTickerBtn_mc.SetTextBlack(true);
				mainTickerBtn_mc.SetTextBlack(true);
			} else {
				if(isHovered){
					item_tf.textColor = 0x000000;
					background_mc.visible = true;
					background_mc.alpha = 0.8;
				} else {
					leftTickerBtn_mc.SetTextBlack(false);
					rightTickerBtn_mc.SetTextBlack(false);
					mainTickerBtn_mc.SetTextBlack(false);
					item_tf.textColor = 0xffffff;
					background_mc.visible = false;
				}
			}
		}
		
		public function setTicker(doTicker:Boolean){
			isTicker = doTicker;

			leftTickerBtn_mc.visible = isTicker;
			rightTickerBtn_mc.visible = isTicker;
			mainTickerBtn_mc.visible = isTicker;
		}
		
		public function onMouseOver(e:MouseEvent):void {
			parentMenu.itemHover(index, true);
		}
		
		public function onMouseOut(e:MouseEvent):void {
			parentMenu.itemHover(index, false);
		}
		
		public function setHovered(hovered:Boolean):void {
			isHovered = hovered;
			
			if(isBold){
				return;
			}

			if(isTicker){
				leftTickerBtn_mc.SetTextBlack(hovered);
				rightTickerBtn_mc.SetTextBlack(hovered);
				mainTickerBtn_mc.SetTextBlack(hovered);
			}
			
			if(hovered){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuFocus");
				}
				item_tf.textColor = 0x000000;
				background_mc.visible = true;
				background_mc.alpha = 0.8;
			} else {
				item_tf.textColor = 0xffffff;
				background_mc.visible = false;
			}
		}
		
		public function onMouseClick(e:MouseEvent):void {
			parentMenu.itemSelect(index);
		}

		public function onLeftTicker(e:MouseEvent):void {
			if(e.buttonDown){
				activateLeftTicker();
				startInputTimer(0);
			} else {
				stopInputTimer();
			}
		}
		
		public function onRightTicker(e:MouseEvent):void {
			if(e.buttonDown){
				activateRightTicker();
				startInputTimer(1);
			} else {
				stopInputTimer();
			}
		}
		
		public function startInputTimer(ty:int):void {
			inputTimer.reset();
			inputTimer.stop();
			inputTimer.delay = 650;
			inputFirst = true;
			inputType = ty;
			inputTimer.start();
		}
		
		public function stopInputTimer():void {
			inputTimer.reset();
			inputTimer.stop();
			inputType = -1;
		}
		
		public function OnInputTimer(event:TimerEvent) : void
		{
			if(inputFirst){
				inputTimer.reset();
				inputTimer.stop();
				inputTimer.delay = 100;
				inputFirst = false;
				inputTimer.start();
			} else {
				if(inputType == 0){
					activateLeftTicker();
				} else if(inputType == 1) {
					activateRightTicker();
				}
			}
		}
		
		public function activateLeftTicker():void {
			if(isTicker){
				parentMenu.leftTick(index);
			}
		}
		
		public function activateRightTicker():void {
			if(isTicker){
				parentMenu.rightTick(index);
			}
		}

		public function onMainTicker(e:MouseEvent):void {
			if(isTicker){
				parentMenu.selectTick(index);
			}
		}
	}
	
}
