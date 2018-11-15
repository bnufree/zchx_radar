INCLUDEPATH += $$PWD/include
CONFIG(release,debug|release){

	LIBS += -L$$PWD/lib/Release -llibradar_analysis
	Ais_install.files += $$PWD/bin/Release/radar_analysis.dll
}
else{

	LIBS += -L$$PWD/lib/Debug -llibradar_analysis
	Ais_install.files += $$PWD/bin/Debug/radar_analysis.dll
}

#Ais_install.path = ../build/bin/
#INSTALLS += Ais_install
