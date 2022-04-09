#pragma once

#include "IO.h"

#include <algorithm>
#include <chrono>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>

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
	root_path = std::filesystem::u8path(root);
}

ApplicationFileSystem::~ApplicationFileSystem()
{

}

scapes::foundation::io::Stream *ApplicationFileSystem::open(const scapes::foundation::io::URI &uri, const char *mode)
{
	std::filesystem::path path = std::filesystem::u8path(uri.c_str());

	if (!path.is_absolute())
		path = root_path / path;

	FILE *file = nullptr;
	fopen_s(&file, path.u8string().c_str(), mode);
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

void *ApplicationFileSystem::map(const scapes::foundation::io::URI &uri, size_t &size)
{
	size = 0;

	scapes::foundation::io::Stream *stream = open(uri, "rb");
	if (!stream)
		return nullptr;

	size = static_cast<size_t>(stream->size());

	uint8_t *data = new uint8_t[size];
	size_t bytes_read = stream->read(data, sizeof(uint8_t), size);
	assert(bytes_read == size);

	bool success = close(stream);
	assert(success);

	return data;
}

bool ApplicationFileSystem::unmap(void *data)
{
	assert(data);
	delete[] data;

	return true;
}

uint64_t ApplicationFileSystem::mtime(const scapes::foundation::io::URI &uri)
{
	std::filesystem::path path = std::filesystem::u8path(uri.c_str());

	if (!path.is_absolute())
		path = root_path / path;

	struct stat info;
	if (stat(path.u8string().c_str(), &info) != 0)
		return 0;

	return static_cast<uint64_t>(info.st_mtime);
}
