ataxx-explorer: ataxx-explorer.cpp
	g++ ataxx-explorer.cpp `~/wxWidgets-2.9.5/wx-config --cppflags` `~/wxWidgets-2.9.5/wx-config --libs` -o ataxx-explorer
clean:
	rm -f ataxx-explorer
