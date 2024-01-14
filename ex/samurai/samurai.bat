@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
(CHCP 65001)>NUL

REM Reads converted sprites from file and runs them in a loop

SET /A "sprite[num]=sprite[cur]=0"
FOR /F "tokens=*" %%? in (sprite.txt) DO (
    SET "samurai[!sprite[num]!]=%%?"
    SET /A "sprite[num]+=1"
)

ECHO [?25l
FOR /L %%? in () DO (
    FOR %%C in (!sprite[cur]!) DO (
        ECHO [2J[1;1H!samurai[%%C]!
    )
    SET /A "sprite[cur]=(sprite[cur] + 1) %% sprite[num]"
    FOR /L %%J in (1,30,1000000) DO REM
)

EXIT /B