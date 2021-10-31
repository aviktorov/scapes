#pragma once

#include "IO.h"

#include <algorithm>
#include <cassert>

/*
 */
ApplicationStream::ApplicationStream(FILE *file) : file(file)
{
	assert(file);
}

ApplicationStream::~ApplicationStream()
{
	assert(file);

	fclose(file);
	file = nullptr;
}

size_t ApplicationStream::read(void *data, size_t element_size, size_t element_count)
{
	assert(file);
	return fread(data, element_size, element_count, file);
}

size_t ApplicationStream::write(const void *data, size_t element_size, size_t element_count)
{
	assert(file);
	return fwrite(data, element_size, element_count, file);
}

bool ApplicationStream::seek(uint64_t offset, scapes::foundation::io::SeekOrigin origin)
{
	assert(file);
	return (fseek(file, static_cast<long>(offset), static_cast<int>(origin)) == 0);
}

uint64_t ApplicationStream::tell() const
{
	assert(file);
	return static_cast<uint64_t>(ftell(file));
}

uint64_t ApplicationStream::size() const
{
	assert(file);
	long position = ftell(file);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, position, SEEK_SET);

	return static_cast<uint64_t>(size);
}


/*
 */
ApplicationFileSystem::ApplicationFileSystem(const char *root) : root_path(root)
{
	std::replace(root_path.begin(), root_path.end(), '\\', '/');

	size_t pos = root_path.rfind('/');
	if (pos == std::string::npos)
		root_path += '/';
}

ApplicationFileSystem::~ApplicationFileSystem()
{

}

scapes::foundation::io::Stream *ApplicationFileSystem::open(const scapes::foundation::io::URI &uri, const char *mode)
{
	const char *offset = strstr(uri, root_path.c_str());
	std::string resolved_path = (offset != uri) ? root_path + uri : uri;

	FILE *file = nullptr;
	fopen_s(&file, resolved_path.c_str(), mode);
	if (!file)
		return nullptr;

	return new ApplicationStream(file);
}

bool ApplicationFileSystem::close(scapes::foundation::io::Stream *stream)
{
	assert(stream);
	delete stream;

	return true;
}
