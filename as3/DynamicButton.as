package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.events.MouseEvent;
	import flash.text.TextFieldAutoSize;
	
	
	public class DynamicButton extends ISizeNotifier {
		
		public var btnText_tf:TextField;
		public var btnBg_mc:MovieClip;
		public var btnOutline_mc:MovieClip;
		public var isHovered:Boolean = false;
		
		public function DynamicButton(p:DynamicPanel) {
			super(p);
			btnText_tf.autoSize = TextFieldAutoSize.LEFT;
			btnText_tf.wordWrap = false;
			btnText_tf.selectable = false;
			btnBg_mc.visible = true;
			btnBg_mc.alpha = 0.01;
			
			addEventListener(MouseEvent.MOUSE_OVER, this.onMouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.onMouseOut);
			addEventListener(MouseEvent.MOUSE_UP, this.onMouseClick);
		}
		
		public function onMouseOver(e:MouseEvent):void {
			setHovered(true);
		}
		
		public function onMouseOut(e:MouseEvent):void {
			setHovered(false);
		}
		
		public function onMouseClick(e:MouseEvent):void {
			if(NAFMenu.CodeObjAvailable && !parentContainer.disableInput){
				NAFMenu.CodeObj.PlaySound("UIMenuOK");
				NAFMenu.CodeObj.ButtonClicked(index);
			}
		}

		public override function SetElementText(s:String){
			btnText_tf.text = s;
			btnBg_mc.width = btnText_tf.width;
			btnOutline_mc.width = btnText_tf.width;
			parentContainer.onSizeChanged(this);
		}
		
		public override function NotifyMaxWidth(w:Number){
			btnBg_mc.width = w;
			btnOutline_mc.width = w;
			btnText_tf.x = (btnBg_mc.width / 2) - (btnText_tf.width / 2);
		}
		
		public function setHovered(hovered:Boolean):void {
			isHovered = hovered;
			
			if(hovered){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuFocus");
				}
				btnText_tf.textColor = 0x000000;
				btnBg_mc.alpha = 1.00;
			} else {
				btnText_tf.textColor = 0xffffff;
				btnBg_mc.alpha = 0.01;
			}
		}
	}
	
}
