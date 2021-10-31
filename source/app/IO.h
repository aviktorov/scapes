#pragma once

#include <scapes/foundation/io/FileSystem.h>
#include <string>

class ApplicationStream : public scapes::foundation::io::Stream
{
public:
	ApplicationStream(FILE *file);
	~ApplicationStream() final;

	size_t read(void *data, size_t element_size, size_t element_count) final;
	size_t write(const void *data, size_t element_size, size_t element_count) final;

	bool seek(uint64_t offset, scapes::foundation::io::SeekOrigin origin) final;
	uint64_t tell() const final;
	uint64_t size() const final;

private:
	FILE *file {nullptr};
};

class ApplicationFileSystem : public scapes::foundation::io::FileSystem
{
public:
	ApplicationFileSystem(const char *root);
	~ApplicationFileSystem() final;

	scapes::foundation::io::Stream *open(const scapes::foundation::io::URI &uri, const char *mode) final;
	bool close(scapes::foundation::io::Stream *stream) final;

private:
	std::string root_path;
};
