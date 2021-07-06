#pragma once

class Log
{
public:
	static void message(const char *format, ...);
	static void warning(const char *format, ...);
	static void error(const char *format, ...);
	static void fatal(const char *format, ...);
};

