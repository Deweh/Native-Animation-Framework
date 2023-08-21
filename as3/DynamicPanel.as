package  {
	
	import flash.display.MovieClip;
	
	
	public class DynamicPanel extends MovieClip {
		
		public var elements:Vector.<ISizeNotifier> = new Vector.<ISizeNotifier>();
		public var dynamicWidth:Number = 300;
		public var dynamicHeight:Number = 150;
		public var horizontalPadding:Number = 10;
		public var verticalPadding:Number = 10;
		
		public var brLeft:HorizontalBracket;
		public var brRight:HorizontalBracket;
		public var bgPanel_mc:MovieClip;
		
		public var isRightAnchor:Boolean = false;
		public var rightAnchorX:Number = 0;

		public var disableInput:Boolean = false;
		
		public function DynamicPanel() {
			brLeft = new HorizontalBracket();
			brRight = new HorizontalBracket();
			addChild(brLeft);
			addChild(brRight);
			brLeft.setBrLeft();
			brRight.setBrRight();
			updatePanel();
		}
		
		public function cleanup(){
			for(var i:int = 0; i < elements.length; i++){
				elements[i].parentContainer = null;
				removeChild(elements[i]);
			}
			
			elements = null;
		}
		
		public function SetElements(eles:Vector.<ISizeNotifier>){
			for(var i:int = 0; i < elements.length; i++){
				removeChild(elements[i]);
			}

			elements = eles;
			
			for(i = 0; i < elements.length; i++){
				addChild(elements[i]);
				elements[i].index = i;
			}
			
			updatePanel();
		}
		
		public function AddElement(ele:ISizeNotifier){
			addChild(ele);
			elements.push(ele);
			updatePanel();
		}
		
		public function onSizeChanged(ele:ISizeNotifier){
			updatePanel();
		}
		
		public function updatePanel(){
			var curHeight:Number = verticalPadding;
			var curWidth:Number = horizontalPadding * 2;
			
			for(var i:int = 0; i < elements.length; i++){
				elements[i].y = curHeight;
				elements[i].x = horizontalPadding;
				curHeight += elements[i].height + verticalPadding;
				
				if(curWidth < (elements[i].width + (horizontalPadding * 2))){
					curWidth = (elements[i].width + (horizontalPadding * 2));
				}
			}

			dynamicWidth = curWidth;
			dynamicHeight = curHeight;

			bgPanel_mc.width = dynamicWidth;
			bgPanel_mc.height = dynamicHeight;
			
			brLeft.setBrHeight(dynamicHeight);
			brRight.setBrHeight(dynamicHeight);
			brRight.x = dynamicWidth - 3;
			
			if(isRightAnchor){
				this.x = (rightAnchorX - this.width);
			}
			
			var widthToNotify:Number = (dynamicWidth - (horizontalPadding * 2));
			for(var j:int = 0; j < elements.length; j++){
				elements[j].NotifyMaxWidth(widthToNotify);
			}
		}
	}
	
}
