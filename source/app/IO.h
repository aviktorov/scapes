#pragma once

#include <common/IO.h>
#include <string>

class ApplicationStream : public io::IStream
{
public:
	ApplicationStream(FILE *file);
	~ApplicationStream() final;

	size_t read(void *data, size_t element_size, size_t element_count) final;
	size_t write(const void *data, size_t element_size, size_t element_count) final;

	bool seek(uint64_t offset, io::SeekOrigin origin) final;
	uint64_t tell() const final;
	uint64_t size() const final;

private:
	FILE *file {nullptr};
};

class ApplicationFileSystem : public io::IFileSystem
{
public:
	ApplicationFileSystem(const char *root);
	~ApplicationFileSystem() final;

	io::IStream *open(const char *path, const char *mode) final;
	bool close(io::IStream *stream) final;

private:
	std::string root_path;
};
