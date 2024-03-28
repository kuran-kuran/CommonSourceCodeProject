@echo off
setlocal enabledelayedexpansion

:: �������擾���A�啶���ɕϊ�
set variant=%1
set variant_uppercase=%variant%
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    call set variant_uppercase=%%variant_uppercase:%%i=%%i%%
)

:: Release��t���Ċ��S�ȃr���h�^�C�v���쐬
set buildType=!variant_uppercase!Release

cd ..

:: �A�Z���u���ƃC���X�g�[��
call .\gradlew assemble!buildType!
call .\gradlew install!buildType!

:: �A�v���P�[�V�����̋N��
adb shell am start -n jp.matrix.shikarunochi.emulator.!variant!/jp.matrix.shikarunochi.emulator.EmulatorActivity

cd buildbatch

:: buildbatch�Ɉړ����ē��t�t�H���_���쐬
cd buildbatch
set DATESTR=%date:~-10,4%%date:~-5,2%%date:~-2,2%
set FOLDERNAME=v%DATESTR%_release_apk

if not exist "%FOLDERNAME%" (
    mkdir "%FOLDERNAME%"
)

:: APK�t�@�C����V�����t�H���_�ɃR�s�[
:: �ȉ��̃p�X�̓v���W�F�N�g�̍\���ɂ��K�X����
copy "..\app\build\outputs\apk\%VARIANT%\release\*.apk" "%FOLDERNAME%"

endlocal
