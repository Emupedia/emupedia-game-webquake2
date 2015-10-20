#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils.h"


int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage %s PAK-file targetdirectory\n", argv[0]);
		exit(0);
	}

	const char *pakFile = argv[1];

	auto buf = readFile(pakFile);
	auto filesize = buf.size();
	printf("read \"%s\", %u bytes\n", pakFile, static_cast<unsigned int>(filesize));

	PackHeader header;
	memcpy(&header, &buf[0], sizeof(header));

	if (memcmp(&header.magic, "PACK", 4) != 0) {
		printf("bad magic\n");
		exit(1);
	}

	if (header.dirOffset > filesize) {
		printf("dir offset past end of file\n");
		exit(1);
	}

	if ((header.dirLength % 64) != 0) {
		printf("dir length not divisible by 64\n");
		exit(1);
	}

	if (header.dirOffset + header.dirLength > filesize) {
		printf("dir end past end of file\n");
		exit(1);
	}

	auto numEntries = header.dirLength / sizeof(DirEntry);
	std::vector<DirEntry> directory(numEntries);
	memcpy(&directory[0], &buf[header.dirOffset], header.dirLength);

	std::string outPath(argv[2]);
	// remove trailing slashes so we don't have to deal with them below
	while (outPath.back() == '/') {
		outPath.pop_back();
	}

	{
		struct stat statbuf;
		memset(&statbuf, 0, sizeof(statbuf));
		int retval = stat(outPath.c_str(), &statbuf);
		if (retval == 0) {
			// success means failure
			printf("Error: \"%s\" already exists, cowardly refusing to overwrite\n", outPath.c_str());
			exit(1);
		}
	}

	std::unordered_set<std::string> alreadyCreated;
	makeDir(outPath, alreadyCreated);

	for (unsigned int i = 0; i < numEntries; i++) {
		const auto &entry = directory[i];
		std::string fullname(&entry.name[0], 56);

		std::string path;
		std::string name;
		std::tie(path, name) = splitPath(fullname);

		printf("\"%s\" \"%s\" at %u len %u\n", path.c_str(), name.c_str(), entry.filePos, entry.length);

		if (entry.filePos > filesize) {
			printf(" Error: filepos start past end of buf\n");
			exit(1);
		}

		if (entry.filePos  + entry.length> filesize) {
			printf(" Error: filepos end past end of buf\n");
			exit(1);
		}

		makeDir(outPath + "/" + path, alreadyCreated);

		std::string outFilename = outPath + "/" + fullname;
		FILE *outFile = fopen(outFilename.c_str(), "wb");
		if (outFile == NULL) {
			printf("Failed to open output file \"%s\" %s (%u)\n", outFilename.c_str(), strerror(errno), errno);
			exit(1);
		}

		auto retval = fwrite(&buf[entry.filePos], 1, entry.length, outFile);
		if (retval != entry.length) {
			printf(" Error: partial write (%u bytes) %s (%u)\n", static_cast<unsigned int>(retval), strerror(errno), errno);
			exit(1);
		}

		fclose(outFile);
	}

	return 0;
}
