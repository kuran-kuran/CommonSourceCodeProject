@echo off

setlocal enabledelayedexpansion

set "csvFile=models\\models.csv"
set "buildType=subBatch\\rbuild.bat"
set "hasInputFile=false"
set "hasBuildType=false"
set "directModelName="

:parseArgs
if "%~1"=="" goto :executeOrShowHelp

if "%~1"=="-i" (
    set "csvFile=%~2"
    set "hasInputFile=true"
    shift & shift
    goto :parseArgs
)

if "%~1"=="-m" (
    set "buildType="
    if "%~2"=="ReleaseBuild" set "buildType=subBatch\\rbuild.bat"
    if "%~2"=="ReleaseInstall" set "buildType=subBatch\\rinstall.bat"
    if "%~2"=="ReleaseBuildExecute" set "buildType=subBatch\\rbuildexec.bat"
    if "%~2"=="DebugBuild" set "buildType=subBatch\\dbuild.bat"
    if "%~2"=="DebugInstall" set "buildType=subBatch\\dinstall.bat"
    if "%~2"=="DebugBuildExecute" set "buildType=subBatch\\dbuildexec.bat"
    if "%~2"=="UnInstall" set "buildType=subBatch\\uninstall.bat"
    if "!buildType!"=="" (
        echo �x��: �w�肳�ꂽ���[�h�u%~2�v�ɑΉ�����o�b�`�t�@�C��������܂���B
        exit /b 1
    )
    set "hasBuildType=true"
    shift & shift
    goto :parseArgs
)

if "%~1"=="-d" (
    set "directModelName=%~2"
    set "hasInputFile=true"
    shift & shift
    goto :parseArgs
)

shift
goto :parseArgs

:executeOrShowHelp
if %hasInputFile%==true if %hasBuildType%==true goto :execute
goto :showHelp

:showHelp
echo ���̃o�b�`�t�@�C���͈ȉ��̈��������܂�:
echo   -i csvFile     ���f����񂪋L�ڂ��ꂽCSV�t�@�C���̃p�X (�f�t�H���g: models\models.csv)
echo   -m buildType   ���s����r���h�̎�� (�f�t�H���g: ReleaseBuild)
echo                  ReleaseBuild, ReleaseInstall, ReleaseBuildExecute,
echo                  DebugBuild, DebugInstall, DebugBuildExecute, UnInstall
echo   -d modelName   �_�C���N�g�Ƀr���h�o�b�`�ɓn���@�햼
exit /b 0

:execute
if not "%directModelName%"=="" (
    echo *********** Direct Model [%directModelName%] [!buildType!] ***********
    call !buildType! %directModelName%
    goto :endExecution
)

if not exist "%csvFile%" (
    echo �w�肳�ꂽ�t�@�C����������܂���: %csvFile%
    exit /b 1
)

for /f "tokens=1,2 delims=," %%a in (%csvFile%) do (
    echo ***********  %%a [!buildType!] ***********
    call !buildType! %%b
)

:endExecution
echo �r���h�v���Z�X���������܂����B

endlocal
