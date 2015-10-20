#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils.h"


static void recursiveAddFiles(std::string path, std::vector<char> &outBuf, std::vector<DirEntry> &directory) {
	DIRwrapper dir(path.c_str());

	struct dirent *curr = nullptr;

	std::vector<std::string> subdirs;

	do {
		curr = readdir(dir);
		if (!curr) {
			if (errno != 0 && errno != ENOENT) {
				printf("error in readdir \"%s\": %s (%u)\n", path.c_str(), strerror(errno), errno);
				exit(1);
			}
			break;
		}

		// skip hidden files
		// and special "." and ".." directories
		if (curr->d_name[0] != '.') {
			std::string fullPathAndName = path + "/" + curr->d_name;

			bool isDir = (curr->d_type & DT_DIR) != 0;
			bool isNormal = (curr->d_type & DT_REG) != 0;

			if (isDir) {
				subdirs.push_back(curr->d_name);
			} else if (isNormal) {
				// drop first component
				auto firstSlash = fullPathAndName.find('/');
				// can't be npos since we added a slash before
				// though it doesn't make sense for there not to be several
				std::string pakName = fullPathAndName.substr(firstSlash + 1);

				printf("pakName: \"%s\"\n", pakName.c_str());

				if (pakName.size() > 56) {
					printf(" Error: filename too long\n");
					exit(1);
				}
				auto buf = readFile(fullPathAndName);

				DirEntry entry;
				memset(&entry, 0, sizeof(entry));
				strncpy(entry.name, pakName.c_str(), pakName.size());
				entry.filePos = outBuf.size();
				entry.length = buf.size();

				outBuf.insert(outBuf.end(), buf.begin(), buf.end());
				directory.emplace_back(std::move(entry));
			}
		}
	} while (curr != nullptr);

	// recurse into subdirectories
	for (const std::string &subdirName : subdirs) {
		std::string fullPathAndName = path + "/" + subdirName;
		recursiveAddFiles(fullPathAndName, outBuf, directory);
	}
}


int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage %s sourcedirectory PAK-file\n", argv[0]);
		exit(0);
	}

	const char *pakFile = argv[2];
	const char *sourceDir = argv[1];

	{
		struct stat statbuf;
		memset(&statbuf, 0, sizeof(statbuf));
		int retval = stat(pakFile, &statbuf);
		if (retval == 0) {
			// success means failure
			printf("Error: \"%s\" already exists, cowardly refusing to overwrite\n", pakFile);
			exit(1);
		}
	}

	// TODO: reserve sizes?
	std::vector<char> outBuf;
	std::vector<DirEntry> directory;

	PackHeader header;
	memset(&header, 0, sizeof(header));
	memcpy(&header.magic, "PACK", 4);
	// insert initial header
	outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&header), reinterpret_cast<char *>(&header) + sizeof(header));

	recursiveAddFiles(sourceDir, outBuf, directory);

	printf("directory.size(): %u\n", static_cast<unsigned int>(directory.size()));

	// fix up header
	header.dirOffset = outBuf.size();
	header.dirLength = directory.size() * sizeof(DirEntry);
	memcpy(&outBuf[0], &header, sizeof(header));

	outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&directory[0]), reinterpret_cast<char *>(&directory[0]) + header.dirLength);
	printf("outBuf.size(): %u\n", static_cast<unsigned int>(outBuf.size()));

	FILE *outFile = fopen(pakFile, "wb");
	if (!outFile) {
		printf("Failed to open output file \"%s\" %s (%u)\n", pakFile, strerror(errno), errno);
		exit(1);
	}

	auto retval = fwrite(&outBuf[0], 1, outBuf.size(), outFile);
	if (retval != outBuf.size()) {
		printf("Error: partial write (%u bytes) %s (%u)\n", static_cast<unsigned int>(retval), strerror(errno), errno);
		exit(1);
	}

	fclose(outFile);

	return 0;
}
