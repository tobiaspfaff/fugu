#pragma once
#include <memory>
#include <string>

class Window 
{
public:
	Window(const std::string& name, int w, int h);
	~Window();
	void* getInstance();
	void* getHandle();

	int width, height;
private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};