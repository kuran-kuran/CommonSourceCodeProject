@echo off
setlocal enabledelayedexpansion

set "csvFile=models\models.csv"
set "buildType=subBatch\rbuild.bat"  ; �f�t�H���g�̃r���h�^�C�v

:parseArgs
if "%~1"=="" goto :execute
if "%~1"=="-i" (
    set "csvFile=%~2"
    shift & shift
    goto :parseArgs
)
if "%~1"=="-m" (
    set "buildType="
    if "%~2"=="ReleaseBuild" set "buildType=subBatch\rbuild.bat"
    if "%~2"=="ReleaseInstall" set "buildType=subBatch\rinstall.bat"
    if "%~2"=="ReleaseBuildExecute" set "buildType=subBatch\rbuildexec.bat"
    if "%~2"=="DebugBuild" set "buildType=subBatch\dbuild.bat"
    if "%~2"=="DebugInstall" set "buildType=subBatch\dinstall.bat"
    if "%~2"=="DebugBuildExecute" set "buildType=subBatch\dbuildexec.bat"
    if "%~2"=="UnInstall" set "buildType=subBatch\uninstall.bat"

    if "!buildType!"=="" (
        echo �x��: �w�肳�ꂽ���[�h�u%~2�v�ɑΉ�����o�b�`�t�@�C��������܂���B
        exit /b 1
    )
    shift & shift
    goto :parseArgs
)
shift
goto :parseArgs

:execute
if not exist "%csvFile%" (
    echo �w�肳�ꂽ�t�@�C����������܂���: %csvFile%
    exit /b 1
)

for /f "tokens=1,2 delims=," %%a in (%csvFile%) do (
    echo ********** %%a [!buildType!] **********
    call !buildType! %%b
)

echo �r���h�v���Z�X���������܂����B

endlocal
