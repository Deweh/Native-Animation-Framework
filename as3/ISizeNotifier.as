package  {
	
	import flash.display.MovieClip;
	
	
	public class ISizeNotifier extends MovieClip {

		public var parentContainer:DynamicPanel;
		public var index:int;
		
		public function ISizeNotifier(prnt:DynamicPanel) {
			parentContainer = prnt;
		}

		public function SetElementText(s:String){
		}

		public function NotifyMaxWidth(w:Number){
		}
	}
	
}
