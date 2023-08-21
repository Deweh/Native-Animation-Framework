package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.text.TextFieldAutoSize;
	import flash.events.TimerEvent;
	import flash.utils.Timer;
	
	
	public class DynamicText extends flash.display.MovieClip {
		
		public var txt_tf:TextField;
		public var txtBg_mc:MovieClip;
		
		public var notificationTimer:Timer = new Timer(25)
		public var timeRemaining:Number = 0
		
		public function DynamicText(){
			txt_tf.autoSize = TextFieldAutoSize.LEFT;
			txt_tf.wordWrap = false;
			notificationTimer.addEventListener("timer", OnTimer);
		}
		
		public function ShowNotification(s:String, time:Number){
			notificationTimer.reset();
			notificationTimer.stop();
			this.visible = true;
			txt_tf.text = s;
			if(txt_tf.width + 30 > 270){
				txtBg_mc.width = (txt_tf.width + 30);
			} else {
				txtBg_mc.width = 270;
			}
			this.alpha = 1.0;
			timeRemaining = time * 40;
			notificationTimer.start();
		}
		
		public function OnTimer(event:TimerEvent) : void
		{
			if(timeRemaining > 0){
				timeRemaining -= 1;
			} else {
				if(this.alpha > 0){
					this.alpha -= 0.025;
				} else {
					notificationTimer.reset();
					notificationTimer.stop();
					this.visible = false;
				}
			}
		}
	}
	
}
