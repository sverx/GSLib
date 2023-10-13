@echo off 
::echo Build assets.c and assets.h from assets folder 
::folder2c assets assets 
::folder2c assets2 assets2 2
::folder2c assets3 assets3 3
::sdcc -c -mz80 assets.c 
::sdcc -c -mz80 --constseg BANK2 assets2.c 
::sdcc -c -mz80 --constseg BANK3 assets3.c 
assets2banks assets --compile
::if %errorlevel% NEQ 0 goto :EOF 
echo Build Main 
sdcc -c -mz80 main.c 
if %errorlevel% NEQ 0 goto :EOF 
echo Linking 
  ::sdcc -o output.ihx -mz80 --no-std-crt0 --data-loc 0xC000 -Wl-b_BANK2=0x8000 -Wl-b_BANK3=0x8000 crt0_sms.rel main.rel SMSlib.lib 
  ::assets.rel assets2.rel assets3.rel
  sdcc -o output.ihx -mz80 --no-std-crt0 --data-loc 0xC000 -Wl-b_BANK2=0x4000 -Wl-b_BANK3=0x4000 libs\crt0_sms.rel main.rel SMSlib.lib bank2.rel bank3.rel

if %errorlevel% NEQ 0 goto :EOF 
ihx2sms output.ihx GSLib_Demo.sms 


echo clean up files
::del *.rel
del *.lst
del *.sym
del *.lk
del *.map
del *.noi
del *.rel
del *.ixh
del bank2.h
del bank3.h
del main.asm
