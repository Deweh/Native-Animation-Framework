@echo off
copy %~dp0\f4se-plugin\build\Release\NAF.dll "C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\F4SE\Plugins\"
copy %~dp0\f4se-plugin\build\Release\NAF.pdb "C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\F4SE\Plugins\"
copy %~dp0\as3\build\NAFMenu.swf "C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\Interface\"
copy %~dp0\as3\build\NAFHUDMenu.swf "C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\Interface\"
copy %~dp0\as3\build\NAFStudioMenu.swf "C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\Interface\"
