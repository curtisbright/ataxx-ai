ataxx-ai: ataxx-ai.cpp
	g++ ataxx-ai.cpp `wx-config --cppflags` `wx-config --libs` -o ataxx-ai
clean:
	rm -f ataxx-ai
