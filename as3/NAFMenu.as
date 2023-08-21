package  {
	
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	
	public class NAFMenu extends MovieClip {

		public var BGSCodeObj:Object;
		public var mainList:MenuList;
		public var dataPanel:DynamicPanel = null;
		public var notify_tf:DynamicText;
		public var mainEntry:EntryBox;
		public static var CodeObjAvailable:Boolean = false;
		public static var CodeObj;
		public var nextHandle:uint = 0;
		
		public function NAFMenu() {
			BGSCodeObj = new Object();
			
			mainList = new MenuList(20);
			addChild(mainList);
			mainList.x = 1392;
			mainList.y = 47;
			
			addEventListener(MouseEvent.MOUSE_OVER, this.onMainListOver);
		}
		
		public function GetNextHandle():uint{
			return nextHandle++;
		}
		
		public function onCodeObjCreate():* {
            BGSCodeObj.PlaySound("UIMenuOK");
			CodeObj = BGSCodeObj;
			CodeObjAvailable = true;
			BGSCodeObj.NotifyLoaded();
        }

		public function ShowTextEntry() {
			mainList.disableInput = true;
			mainEntry.mainText_tf.text = "";
			mainEntry.mainList = mainList;
			mainEntry.dataPanel = dataPanel;
			mainEntry.visible = true;
			
			mainList.visible = false;
			if(dataPanel != null){
				dataPanel.visible = false;
			}
			BGSCodeObj.SetTextEntry(true);
			stage.focus = mainEntry.mainText_tf;
		}

		public function SetTickerText(s:String, index:int){
			mainList.setTickerText(s, index);
		}
		
		public function ShowNotification(s:String, time:Number) {
			notify_tf.ShowNotification(s, time);
		}

		public function SetMenuTitle(s:String){
			mainList.title_tf.text = s;
		}
		
		public function SetDataItems(arr:Array, resetScroll:Boolean) {
			if(resetScroll){
				mainList.scrollPos = 0;
			}
			
			mainList.dataItems = arr;
			mainList.updateList();
			
			if(mainList.hoveredItem > -1){
				mainList.hoveredDataItm = -1;
				mainList.itemHover(mainList.hoveredItem, true);
			}
		}
		
		public function SetSingleDataItem(itmIndex:int, itmLabel:String, itmBold:Boolean) {
			mainList.dataItems[itmIndex].label = itmLabel;
			mainList.dataItems[itmIndex].bold = itmBold;
			mainList.updateList();
		}
		
		public function SetPanelShown(s:Boolean, leftAlign:Boolean){
			if(s && dataPanel == null){
				dataPanel = new DynamicPanel();
				addChild(dataPanel);
				if(leftAlign){
					dataPanel.x = 0;
				} else {
					dataPanel.isRightAnchor = true;
					dataPanel.rightAnchorX = 1382
				}
				dataPanel.y = 47;
				
				dataPanel.updatePanel();
			} else if(!s && dataPanel != null){
				removeChild(dataPanel);
				dataPanel.cleanup();
				dataPanel = null;
				nextHandle = 0;
			}
		}

		public function SetPanelElements(arr:Array){
			if(dataPanel == null){
				return;
			}

			var eles:Vector.<ISizeNotifier> = new Vector.<ISizeNotifier>()
			var newEle:ISizeNotifier;
			for(var i:int = 0; i < arr.length; i++){
				if(arr[i].itemType == 0){
					newEle = new DynamicButton(dataPanel);
				} else if(arr[i].itemType == 1) {
					newEle = new DynamicTicker(dataPanel);
				} else if(arr[i].itemType == 2) {
					newEle = new DynamicInfo(dataPanel);
				} else {
					continue;
				}
				newEle.SetElementText(arr[i].label);
				eles.push(newEle);
			}

			dataPanel.SetElements(eles);
		}

		public function SetElementText(s:String, index:int){
			if(dataPanel == null){
				return;
			}

			if(dataPanel.elements.length <= index){
				return;
			}

			dataPanel.elements[index].SetElementText(s);
		}
		
		public function ProcessButtonEvent(e:String) {
			switch(e){
				case "Accept":
					mainList.keySelect();
					break;
				case "Up":
					mainList.keyNav(true);
					break;
				case "Down":
					mainList.keyNav(false);
					break;
				case "Left":
					mainList.keyNav(true, true);
					break;
				case "Right":
					mainList.keyNav(false, true);
					break;
				case "Delete":
					if(mainList.hoveredItem > -1){
						mainList.itemRightClick(mainList.hoveredItem);
					}
					break;
			}
		}
		
		public function onMainListOver(e:MouseEvent):void {
			if(stage.focus != mainList && !mainList.disableInput){
				stage.focus = mainList;
			}
		}
	}
}
