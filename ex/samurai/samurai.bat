@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
MODE 64, 32
(CHCP 65001)>NUL

REM Reads converted sprites from file and loops them with a rain animation

IF not exist sprite.txt (
    ECHO Run the .c file first to generate sprites
    PAUSE
    EXIT /B
)

SET /A "sprite[num]=sprite[cur]=0"
FOR /F "tokens=*" %%? in (sprite.txt) DO (
    SET "samurai[!sprite[num]!]=%%?"
    SET /A "sprite[num]+=1"
)
FOR /L %%G in (1, 1, 64) DO (
    SET "ground=!ground!_"
)

ECHO [?25l
FOR /L %%? in () DO (
    FOR /L %%G in (1, 1, 10) DO (
        SET /A "rand[y]=!RANDOM! %% 32", "rand[x]=!RANDOM! %% 64"
        IF !rand[y]! GTR 20 (
            SET /A "rand[splat]=!RANDOM! %% 6"
            IF "!rand[splat]!" == "0" (
                SET "rain=!rain![!rand[y]!;!rand[x]!H|"
            ) else (
                SET "rain=!rain![!rand[y]!;!rand[x]!H[D\|/"
            )
        ) else (
            SET "rain=!rain![!rand[y]!;!rand[x]!H|"
        )
    )

    FOR %%C in (!sprite[cur]!) DO (
        ECHO [2J[19;1H[38;2;255;255;255m%ground%[38;2;120;181;207m!rain![1;1H!samurai[%%C]!
    )

    SET /A "sprite[cur]=(sprite[cur] + 1) %% sprite[num]"
    SET "rain="
    FOR /L %%J in (1,30,1000000) DO REM
)

EXIT /B