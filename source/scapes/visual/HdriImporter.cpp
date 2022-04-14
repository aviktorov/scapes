#include <impl/HdriImporter.h>

namespace scapes::visual
{
	HdriImporter *HdriImporter::create(const HdriImporter::CreateOptions &options)
	{
		return new impl::HdriImporter(options);
	}

	void HdriImporter::destroy(HdriImporter *importer)
	{
		delete importer;
	}
}
