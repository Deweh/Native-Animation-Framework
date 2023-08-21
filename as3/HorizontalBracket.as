package  {
	
	import flash.display.MovieClip;
	
	
	public class HorizontalBracket extends MovieClip {
		
		public var brLength_mc:MovieClip;
		public var brTopCap_mc:MovieClip;
		public var brBtmCap_mc:MovieClip;
		private var brHeight:Number;
		
		public function HorizontalBracket() {
			
		}
		
		public function setBrRight(){
			brTopCap_mc.x = -35;
			brBtmCap_mc.x = -35;
		}
		
		public function setBrLeft(){
			brTopCap_mc.x = 0;
			brBtmCap_mc.x = 0;
		}
		
		public function setBrHeight(ht:Number){
			brLength_mc.height = ht;
			brBtmCap_mc.y = ht - 3;
		}
		
	}
	
}
