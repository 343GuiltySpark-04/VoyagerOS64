@echo off
color 0a

:start
echo.
echo press any key to launch qemu...
pause > nul
goto launch

:launch
cls
color 07
qemu-system-x86_64 -d int -serial file:serial.log -gdb tcp::6000 -cdrom VoyagerOS.iso