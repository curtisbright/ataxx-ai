compile:
	g++ ataxx-explorer.cpp `wx-config --cppflags` `wx-config --libs` -o ataxx-explorer
clean:
	rm -f ataxx-explorer