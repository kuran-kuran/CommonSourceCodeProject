@echo off
setlocal enabledelayedexpansion

:: �������擾���A�啶���ɕϊ�
set variant=%1
set variant_uppercase=%variant%
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    call set variant_uppercase=%%variant_uppercase:%%i=%%i%%
)

:: Debug��t���Ċ��S�ȃr���h�^�C�v���쐬
set buildType=!variant_uppercase!Debug

cd ..

:: �C���X�g�[��
call .\gradlew install!buildType!

cd buildbatch

endlocal
