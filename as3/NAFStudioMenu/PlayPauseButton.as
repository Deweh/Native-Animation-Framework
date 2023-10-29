package  {
	
	import flash.display.MovieClip;
	
	
	public class PlayPauseButton extends DynamicClip {
		
		public var Play_mc:MovieClip = new PlayIcon();
		public var Pause_mc:MovieClip = new PauseIcon();
		
		public var bIsPlay = true;
		
		public function PlayPauseButton() {
			this.fBaseAlpha = 0.65;
			this.fHoverAlpha = 0.75;
			addChild(Play_mc);
		}
		
		public function SetIsPlay(i:Boolean){
			if(bIsPlay != i){
				if(i){
					removeChild(Pause_mc);
					addChild(Play_mc);
				} else {
					removeChild(Play_mc);
					addChild(Pause_mc);
				}
				bIsPlay = i;
			}
		}
		
		override public function Selected(){
			SetIsPlay(!bIsPlay);
			if(bIsPlay){
				NAFStudio.NotifyClicked("Play", this);
			} else {
				NAFStudio.NotifyClicked("Pause", this);
			}
		}
	}
	
}
