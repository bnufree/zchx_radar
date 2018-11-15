ZCHX_3RD_PATH = $${PWD}/3rdparty
DEFINES *= ZCHX_API_BUILD
# to resolve windows problem
# https://stackoverflow.com/questions/404774/why-library-name-gets-an-additional-0-in-its-name/42269750#42269750
# https://stackoverflow.com/questions/14636397/qt-dont-append-major-version-number-to-the-end-of-executable-library-name
win32|mingw{
TARGET_EXT = .dll
CONFIG *= skip_target_version_ext
}
