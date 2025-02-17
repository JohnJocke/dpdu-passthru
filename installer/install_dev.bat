@echo off

pushd %~dp0

if not exist "C:\Program Files (x86)\dpdu_j2534" mkdir "C:\Program Files (x86)\dpdu_j2534"
if not exist "C:\Program Files (x86)\D-PDU API" mkdir "C:\Program Files (x86)\D-PDU API"

echo "Copying files"
xcopy /s/y "pdu_api_root.xml" "C:\Program Files (x86)\D-PDU API"
if errorlevel 1 (
	echo Error copying DPDPU XML file
	pause
	exit
)

xcopy /s/y "..\driver\Release\driver.dll" "C:\Program Files (x86)\dpdu_j2534"
if errorlevel 1 (
	echo Error copying driver.dll
	pause
	exit
)

echo Install complete!
pause