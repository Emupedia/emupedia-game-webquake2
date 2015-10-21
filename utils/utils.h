#ifndef UTILS_H
#define UTILS_H


#include <dirent.h>
#include <sys/stat.h>

#include <cassert>


struct DIRwrapper {
	DIR *dir;

	// this can be used transparently as DIR*
	operator DIR *() {
		return dir;
	}


	DIRwrapper(const char *dirname) {
		assert(dirname != nullptr);
		dir = opendir(dirname);

		if (dir == nullptr) {
            printf("failed to opendir \"%s\"\n", dirname);
			exit(1);
		}
	}


	// take care of closing dir when destroyed
	~DIRwrapper() {
		closedir(dir);
	}
};


struct FILEDeleter {
	void operator()(FILE *f) { fclose(f); }
};


static std::vector<char> readFile(std::string filename) {
	std::unique_ptr<FILE, FILEDeleter> file(fopen(filename.c_str(), "rb"));

	if (!file) {
		printf("file not found %s\n", filename.c_str());
		exit(1);
	}

	int fd = fileno(file.get());
	if (fd < 0) {
		printf("no fd\n");
		exit(1);
	}

	struct stat statbuf;
	memset(&statbuf, 0, sizeof(struct stat));
	int retval = fstat(fd, &statbuf);
	if (retval < 0) {
		printf("fstat failed\n");
		exit(1);
	}

	uint64_t filesize = static_cast<uint64_t>(statbuf.st_size);
	std::vector<char> buf(filesize, '\0');

	size_t ret = fread(&buf[0], 1, filesize, file.get());
	if (ret != filesize)
	{
		printf("fread failed\n");
		exit(1);
	}

	return buf;
}


// split path at last slash
static std::pair<std::string, std::string> splitPath(std::string filename) {
	std::string path;
	std::string name;
	auto lastSlash = filename.rfind('/');
	if (lastSlash != std::string::npos) {
		path = filename.substr(0, lastSlash);
		name = filename.substr(lastSlash + 1);
	} else {
		name = filename;
	}

	return std::make_pair(path, name);
}


// create dir if and only if necessary
static inline void makeDir(std::string directory, std::unordered_set<std::string> &alreadyCreated) {
	if (alreadyCreated.find(directory) != alreadyCreated.end()) {
		// should already have been created
		return;
	}

	std::string path;
	std::string name;
	std::tie(path, name) = splitPath(directory);
	if (!path.empty()) {
		// recurse
		makeDir(path, alreadyCreated);
	}

	int retval = mkdir(directory.c_str(), 0775);
	if (retval < 0) {
		if (errno == EEXIST) {
			// already exists, not an error
			return;
		}

		printf("mkdir error when creating \"%s\": %s (%d)\n", directory.c_str(), strerror(errno), errno);
		exit(1);
	}

	alreadyCreated.emplace(std::move(directory));
}


struct PackHeader {
	uint32_t magic;
	uint32_t dirOffset;
	uint32_t dirLength;
} __attribute__((packed));


struct DirEntry {
	char name[56];
	uint32_t filePos;
	uint32_t length;
};


static_assert(sizeof(PackHeader) == 12, "FAIL");
static_assert(sizeof(DirEntry) == 64, "FAIL");


#endif  // UTILS_H
