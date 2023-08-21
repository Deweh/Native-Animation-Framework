package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.ui.Keyboard;
	import flash.events.KeyboardEvent;
	
	public class EntryBox extends MovieClip {
		
		public var mainText_tf:TextField
		public var mainList:MenuList;
		public var dataPanel:DynamicPanel = null;
		
		public function EntryBox() {
			mainText_tf.addEventListener(KeyboardEvent.KEY_DOWN, handleKeyDown);
		}

		public function handleKeyDown(event:KeyboardEvent){
			if(this.visible == true){
				if(event.keyCode == Keyboard.TAB){
					this.visible = false;
					stage.focus = mainList;
					mainList.disableInput = false;
					mainList.visible = true;
					if(dataPanel != null){
						dataPanel.visible = true;
					}
					if(NAFMenu.CodeObjAvailable){
						NAFMenu.CodeObj.SetTextEntry(false);
						NAFMenu.CodeObj.PlaySound("UIMenuCancel")
						NAFMenu.CodeObj.TextEntryCompleted(false, "");
					}
				} else if (event.keyCode == Keyboard.ENTER){
					this.visible = false;
					stage.focus = mainList;
					mainList.disableInput = false;
					mainList.visible = true;
					if(dataPanel != null){
						dataPanel.visible = true;
					}
					if(NAFMenu.CodeObjAvailable){
						NAFMenu.CodeObj.SetTextEntry(false);
						NAFMenu.CodeObj.PlaySound("UIMenuOK")
						NAFMenu.CodeObj.TextEntryCompleted(true, mainText_tf.text);
					}
				}
			}
		}
	}
	
}
