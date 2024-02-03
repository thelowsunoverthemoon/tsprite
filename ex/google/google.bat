@ECHO OFF

ECHO [4mGoogle image split into 4 different parts, Truecolour vs Preset[24m
CALL :PRINT_IMAGE "Top Left" "-a 0 -a 0 -a 20 -a 10"
CALL :PRINT_IMAGE "Top Right" "-a 20 -a 0 -a 20 -a 10"
CALL :PRINT_IMAGE "Bottom Left" "-a 0 -a 10 -a 20 -a 10"
CALL :PRINT_IMAGE "Bottom Right" "-a 20 -a 10 -a 20 -a 10"

PAUSE>NUL
EXIT /B

:PRINT_IMAGE <text> <area>
CALL :GET_IMAGE_VAR "tsprite -f google.jpg --ax 40 --ay 20 --cs %~2" image[normal]
CALL :GET_IMAGE_VAR "tsprite -f google.jpg --ax 40 --ay 20 --cs --ea %~2" image[equiv]
ECHO %~1[B[G%image[normal]%[9A[20G%image[equiv]%]10B[0m
GOTO :EOF

:GET_IMAGE_VAR <arg> <var>
FOR /F "tokens=*" %%A in ('%~1') DO (
    SET "%~2=%%A"
)
GOTO :EOF