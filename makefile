default: ataxx-ai ataxx-ai-console
ataxx-ai: ataxx-ai.cpp
	g++ ataxx-ai.cpp `wx-config --cppflags` `wx-config --libs` -Ofast -o ataxx-ai
ataxx-ai-console: ataxx-ai-console.c
	gcc ataxx-ai-console.c -Ofast -o ataxx-ai-console
clean:
	rm -f ataxx-ai
	rm -f ataxx-ai-console
