package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.text.TextFieldAutoSize;
	
	
	public class DynamicInfo extends ISizeNotifier {
		
		public var mainText_tf:TextField;
		
		public function DynamicInfo(p:DynamicPanel) {
			super(p);
			mainText_tf.autoSize = TextFieldAutoSize.LEFT;
			mainText_tf.wordWrap = false;
			mainText_tf.selectable = false;
		}

		public override function SetElementText(s:String){
			mainText_tf.text = s;
			parentContainer.onSizeChanged(this);
		}
		
		public override function NotifyMaxWidth(w:Number){
			mainText_tf.x = ((w / 2) - (mainText_tf.width / 2));
		}
	}
	
}
