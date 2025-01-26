@echo off 
echo Build bank2.rel 
assets2banks assets --compile
if %errorlevel% NEQ 0 goto :EOF 
echo Build Main 
sdcc -c -mz80 main.c 
if %errorlevel% NEQ 0 goto :EOF 
echo Linking
sdcc -o output.ihx -mz80 --no-std-crt0 --data-loc 0xC000 crt0_sms.rel SMSlib.lib PSGlib.lib GSLib.rel main.rel bank2.rel
if %errorlevel% NEQ 0 goto :EOF 
ihx2sms output.ihx GSLib_Demo.sms 

