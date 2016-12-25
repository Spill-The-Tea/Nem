set PATH=%PATH%;"C:\Program Files\7-Zip"
set RELEASE_VERSION=1
IF "%APPVEYOR_BUILD_NUMBER%"=="" set APPVEYOR_BUILD_NUMBER=01
set ARCHIVE_DIR="Nemorino_last"
RD /S /Q %ARCHIVE_DIR%
MKDIR %ARCHIVE_DIR%
copy .\x64\Release\nemorino.exe .\%ARCHIVE_DIR%\Nemorino_%RELEASE_VERSION%.%APPVEYOR_BUILD_NUMBER%_win64.exe
copy ".\x64\Release no Popcount\nemorino.exe" .\%ARCHIVE_DIR%\Nemorino_%RELEASE_VERSION%.%APPVEYOR_BUILD_NUMBER%_win64-no-popcount.exe
copy license.txt .\%ARCHIVE_DIR%\license.txt
mkdir %ARCHIVE_DIR%\src
copy Nelson\*.cpp %ARCHIVE_DIR%\src\*.cpp
copy Nelson\*.h %ARCHIVE_DIR%\src\*.h
mkdir %ARCHIVE_DIR%\src\syzygy
copy Nelson\syzygy\*.* %ARCHIVE_DIR%\src\syzygy\*.*
copy readme.html %ARCHIVE_DIR%\readme.html
7z a %ARCHIVE_DIR%.zip .\%ARCHIVE_DIR%\*
RD /S /Q %ARCHIVE_DIR%
