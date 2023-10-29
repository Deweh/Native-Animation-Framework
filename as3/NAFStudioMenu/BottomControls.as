package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	
	
	public class BottomControls extends MovieClip {
		
		public var Scrubber_mc:Scrubber;
		public var Timeline_mc:Timeline;
		public var PlayPause_mc:PlayPauseButton;
		
		public function BottomControls() {
			Scrubber_mc.bcParent = this;
		}
		
		public function SetScrubberPosition(p:Number){
			Scrubber_mc.SetPositionNormal(Timeline_mc.KeyToPos(p));
		}
		
		public function ScrubberPositionChanged(p:Number){
			NAFStudio.NotifyScrubberPosition(Timeline_mc.GetKeyNumber(p));
		}
	}
	
}
