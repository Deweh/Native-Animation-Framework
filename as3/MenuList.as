package  {
	
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.events.MouseEvent;
	import flash.events.KeyboardEvent;
	
	
	public class MenuList extends MovieClip {

		public var hoveredDataItm:int = -1;
		public var hoveredItem:int = -1;
		public var dataItems:Array = new Array();
		public var scrollPos:int = 0;
		public var displayItems:Vector.<MenuListItem>;
		public var itemsContainer_mc:MovieClip;
		public var title_tf:TextField;
		public var disableInput:Boolean = false;
		public var displayItemsLength:int;
		public var scrollMessage_tf:TextField;
		
		public var brLeft:HorizontalBracket;
		public var brRight:HorizontalBracket;
		
		public function MenuList(numDisplayItems:uint) {
			scrollMessage_tf.visible = false;
			brLeft = new HorizontalBracket();
			brRight = new HorizontalBracket();
			addChild(brLeft);
			addChild(brRight);
			brLeft.setBrLeft();
			brLeft.setBrHeight(970);
			brRight.setBrRight();
			brRight.setBrHeight(970);
			brRight.x = 502;
			
			focusRect = false;
			title_tf.selectable = false;
			
			displayItemsLength = numDisplayItems;
			displayItems = new Vector.<MenuListItem>(numDisplayItems);
			var spacing:uint = 900 / numDisplayItems;
			
			for(var i:uint = 0; i < numDisplayItems; i++){
				var itm:MenuListItem = new MenuListItem(this);
				itemsContainer_mc.addChild(itm);
				itm.x = 0;
				itm.y = i * spacing;
				itm.index = i;
				itm.item_tf.selectable = false;
				itm.visible = false;
				
				displayItems[i] = itm;
			}
			
			addEventListener(MouseEvent.MOUSE_WHEEL, this.onMouseScroll);
		}
		
		public function scrollMenu(gotoPos:int){
			scrollPos = gotoPos;
			updateList();
			if(hoveredItem > -1){
				itemHover(hoveredItem, true);
			}
		}
		
		public function itemSelect(index:int):void {
			if(!disableInput){
				if(!displayItems[index].isTicker){
					if(NAFMenu.CodeObjAvailable){
						NAFMenu.CodeObj.PlaySound("UIMenuOK");
						NAFMenu.CodeObj.ItemSelected(scrollPos + index);
					}
				}
			}
		}

		public function itemRightClick(index:int):void {
			if(!disableInput){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuOK");
					NAFMenu.CodeObj.ItemRightClicked(scrollPos + index);
				}
			}
		}
		
		public function itemHover(index:int, isHovered:Boolean):void {
			if(!disableInput){
				if(hoveredDataItm > -1){
					if(NAFMenu.CodeObjAvailable){
						NAFMenu.CodeObj.ItemHoverChanged(hoveredDataItm, false);
					}
					hoveredDataItm = -1;
				}
				
				if(index > dataItems.length){
					index = dataItems.length - 1;
				}
				
				if(isHovered){
					if(hoveredItem >= 0){
						displayItems[hoveredItem].setHovered(false);
					}
					displayItems[index].setHovered(true);
					hoveredItem = index;
					hoveredDataItm = scrollPos + index;
					
					if(NAFMenu.CodeObjAvailable){
						NAFMenu.CodeObj.ItemHoverChanged(hoveredDataItm, true);
					}
				} else {
					displayItems[index].setHovered(false);
					if(hoveredItem == index){
						hoveredItem = -1
					}
				}
			}
		}

		public function leftTick(index:int):void {
			if(!disableInput){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuOK");
					NAFMenu.CodeObj.TickerSelected(scrollPos + index, 0);
				}
			}
		}

		public function rightTick(index:int):void {
			if(!disableInput){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuOK");
					NAFMenu.CodeObj.TickerSelected(scrollPos + index, 1);
				}
			}
		}

		public function selectTick(index:int):void {
			if(!disableInput){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuOK");
					NAFMenu.CodeObj.TickerSelected(scrollPos + index, 2);
				}
			}
		}

		public function setTickerText(s:String, index:int):void {
			dataItems[index].value = s;
			displayItems[index - scrollPos].mainTickerBtn_mc.SetButtonText(s);
		}
		
		public function keyNav(up:Boolean, isSide:Boolean = false):void {
			if(!disableInput){
				if(hoveredItem < 0){
					itemHover(0, true);
					return;
				}
				
				if(!isSide){
					if(up){
						if(hoveredItem > 0){
							itemHover(hoveredItem - 1, true);
						} else if(scrollPos > 0){
							scrollMenu(scrollPos - 1);
						} else {
							scrollMenu(MaxScrollPos());
							itemHover(displayItemsLength - 1, true);
						}
					} else {
						if(hoveredItem < (displayItemsLength - 1) && dataItems.length > (hoveredItem + 1)){
							itemHover(hoveredItem + 1, true)
						} else if(scrollPos < MaxScrollPos()){
							scrollMenu(scrollPos + 1);
						} else {
							scrollMenu(0);
							itemHover(0, true);
						}
					}
				} else {
					if(hoveredItem > -1 && displayItems[hoveredItem].isTicker){
						if(up){
							leftTick(hoveredItem);
						} else {
							rightTick(hoveredItem);
						}
					}
				}
			}
		}
		
		public function keySelect():void {
			if(!disableInput && hoveredItem >= 0){
				itemSelect(hoveredItem);
			}
		}
		
		public function onMouseScroll(e:MouseEvent):void {
			if(!disableInput){
				if(NAFMenu.CodeObjAvailable){
					NAFMenu.CodeObj.PlaySound("UIMenuFocus");
				}
				
				scrollMenu(scrollPos - e.delta);
			}
		}
		
		public function updateList():void {
			if(scrollPos < 0){
				scrollPos = 0;
			} else if(scrollPos > MaxScrollPos()){
				scrollPos = MaxScrollPos();
			}
			
			for(var i:int = 0; i < displayItemsLength; i++){
				if((scrollPos + i) < dataItems.length){
					displayItems[i].visible = true;
					var dataIndex:int = scrollPos + i;
					displayItems[i].item_tf.text = dataItems[dataIndex].label;
					
					if((dataItems[dataIndex].bold && !displayItems[i].isBold) || (!dataItems[dataIndex].bold && displayItems[i].isBold)){
						displayItems[i].setBold(dataItems[dataIndex].bold);
					}
					if((dataItems[dataIndex].ticker && !displayItems[i].isTicker) || (!dataItems[dataIndex].ticker && displayItems[i].isTicker)){
						displayItems[i].setTicker(dataItems[dataIndex].ticker);
					}

					displayItems[i].mainTickerBtn_mc.SetButtonText(dataItems[dataIndex].value);
				} else {
					displayItems[i].visible = false;
				}
			}

			scrollMessage_tf.visible = (displayItemsLength < dataItems.length);
		}
		
		public function MaxScrollPos():int {
			var res:int = dataItems.length - displayItemsLength;
			if(res < 0){
				return 0;
			} else {
				return res;
			}
		}
	}
	
}
