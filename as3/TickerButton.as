package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.events.MouseEvent;
	
	public class TickerButton extends MovieClip {
		
		public var mainBG_mc:MovieClip;
		public var mainText_tf:TextField;
		public var forceTextBlack:Boolean = false;
		public var isHovered:Boolean = false;
		
		public function TickerButton() {
			addEventListener(MouseEvent.MOUSE_OVER, this.onMouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, this.onMouseOut);
			mainText_tf.selectable = false;
			mainBG_mc.visible = false;
		}

		public function SetButtonText(s:String){
			mainText_tf.text = s;
		}

		public function SetTextBlack(b:Boolean){
			forceTextBlack = b;
			if(forceTextBlack){
				mainText_tf.textColor = 0x000000;
			} else if (!forceTextBlack && !isHovered) {
				mainText_tf.textColor = 0xffffff
			}
		}

		public function onMouseOver(e:MouseEvent):void {
			isHovered = true;
			mainText_tf.textColor = 0x000000;
			mainBG_mc.visible = true;
		}
		
		public function onMouseOut(e:MouseEvent):void {
			isHovered = false;
			if(!forceTextBlack){
				mainText_tf.textColor = 0xffffff;
			}
			mainBG_mc.visible = false;
		}
	}
}
