package  {
	
	import flash.display.MovieClip;
	import flash.display.Shape;
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.display.BlendMode;
	
	
	public class NAFHUD extends MovieClip {
		
		public var managedElements:Object = new Object() 
		
		public function NAFHUD() {
			this.blendMode = BlendMode.LAYER;
		}
		
		public function AddRectangle(handle:uint, nColor:uint, nAlpha:Number, nBorderSize:Number, nBorderColor:uint, nBorderAlpha:Number, nX:Number, nY:Number, nWidth:Number, nHeight:Number)
		{
			var newRect:Shape = new Shape();
			newRect.graphics.beginFill(nColor, nAlpha);
			newRect.graphics.lineStyle(nBorderSize, nBorderColor, nBorderAlpha);
			newRect.graphics.drawRect(0, 0, nWidth, nHeight);
			newRect.graphics.endFill();
			addChild(newRect);
			newRect.x = nX;
			newRect.y = nY;
			managedElements[handle] = { "type":"shape", "object":newRect };
		}
		
		public function AddText(handle:uint, nText:String, nSize:Number, nX:Number, nY:Number, nColor:uint, nAlpha:Number, nBold:Boolean, nItalic:Boolean, nUnderline:Boolean)
		{
			var newText:TextField = new TextField();
			newText.autoSize = "left";
			var newTxtFmt:TextFormat = new TextFormat("$MAIN_Font", nSize, nColor, nBold, nItalic, nUnderline, null, null, "left", 0, 0, 0, 0);
			newText.text = nText;
			newText.setTextFormat(newTxtFmt);
			addChild(newText);
			newText.x = nX;
			newText.y = nY;
			managedElements[handle] = { "type":"text", "object":newText };
		}
		
		public function AddLine(handle:uint, nColor:uint, nAlpha:Number, nThickness:Number, nSX:Number, nSY:Number, nEX:Number, nEY:Number){
			var newLine:Shape = new Shape();
			newLine.graphics.lineStyle(nThickness, nColor, nAlpha);
			newLine.graphics.lineTo(nEX - nSX, nEY - nSY);
			addChild(newLine);
			newLine.x = nSX;
			newLine.y = nSY;
			managedElements[handle] = { "type":"shape", "object":newLine };
		}
		
		public function SetAlpha(a:Number){
			this.alpha = a;
		}
		
		public function SetElementPosition(handle:uint, nX:Number, nY:Number){
			if(managedElements.hasOwnProperty(handle)){
				managedElements[handle]["object"].x = nX;
				managedElements[handle]["object"].y = nY;
			}
		}
		
		public function RemoveElement(handle:uint){
			if(managedElements.hasOwnProperty(handle)){
				removeChild(managedElements[handle]["object"]);
				delete managedElements[handle];
			}
			this.blendMode = BlendMode.ADD;
			this.blendMode = BlendMode.LAYER;
		}
		
		public function MoveElementToBack(handle:uint){
			if(managedElements.hasOwnProperty(handle)){
				setChildIndex(managedElements[handle]["object"], 0);
			}
		}
		
		public function MoveElementToFront(handle:uint){
			if(managedElements.hasOwnProperty(handle)){
				addChild(managedElements[handle]["object"]);
			}
		}
		
		public function GetElementWidth(handle:uint):Number
		{
			if(managedElements.hasOwnProperty(handle)){
				return managedElements[handle]["object"].width;
			} else {
				return 0;
			}
		}
		
		public function GetElementHeight(handle:uint):Number
		{
			if(managedElements.hasOwnProperty(handle)){
				return managedElements[handle]["object"].height;
			} else {
				return 0;
			}
		}
		
		public function SetElementMask(handle:uint, mskHandle:uint){
			if(managedElements.hasOwnProperty(handle) && managedElements.hasOwnProperty(mskHandle)){
				managedElements[handle]["object"].mask = managedElements[mskHandle]["object"];
			} else if(managedElements.hasOwnProperty(handle)) {
				managedElements[handle]["object"].mask = null;
			}
		}
		
		public function ClearElements(){
			for(var i:String in managedElements){
				removeChild(managedElements[i]["object"]);
			}
			
			managedElements = new Object();
		}
	}
	
}
