#include <iostream>
#include <fstream>

#include <stdio.h>
#include <Stfs/StfsPackage.h>

#define NEXT_ARG(arg_name) { \
			i++;\
			if (i >= argc)\
			{\
				printf("Error: %s expects another argument\n", arg_name);\
				return 0;\
			}\
		}

typedef enum _StfsOpenMode {
	Unknown,
	Create,
	OpenExisting
} StfsOpenMode;

typedef struct _FileInjectInfo {
	std::string path;
	std::string nameInPackage;
} FileInjectInfo;

bool ReadEntireFile(std::string path, BYTE** contents, size_t* size)
{
	std::fstream file;
	file.open(path, std::ios::in | std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	file.seekp(0, std::ios::end);
	size_t data_size = file.tellp();
	file.seekp(0, std::ios::beg);

	BYTE* data = new BYTE[data_size];
	file.read((char*)data, data_size);
	file.close();

	*contents = data;
	*size = data_size;
}


int main(int argc, char** argv)
{
	StfsOpenMode openMode = Unknown;
	std::string packagePath;
	std::string kvPath;
	std::vector<FileInjectInfo> filesToInject;

	for (int i = 0; i < argc; i++)
	{
		std::string cur_arg(argv[i]);

		// open mode
		if (cur_arg == "-c" || cur_arg == "--create")
		{
			openMode = Create;

			NEXT_ARG("--create");
			packagePath = std::string(argv[i]);
		}
		else if (cur_arg == "-o" || cur_arg == "--open")
		{
			openMode = OpenExisting;

			NEXT_ARG("--open");
			packagePath = std::string(argv[i]);
		}
		else if (cur_arg == "-kv" || cur_arg == "--keyvault")
		{
			NEXT_ARG("--keyvault");
			kvPath = std::string(argv[i]);
		}
		else if (cur_arg == "-if" || cur_arg == "--inject-file")
		{
			FileInjectInfo injectInfo;

			NEXT_ARG("--inject_file");
			injectInfo.path = std::string(argv[i]);

			NEXT_ARG("--inject_file");
			injectInfo.nameInPackage = std::string(argv[i]);

			filesToInject.push_back(injectInfo);
		}
	}

	if (openMode == Unknown)
	{
		printf("Error: No open mode specified, provide either --create or --open\n");
		return 0;
	}

	if (packagePath.empty())
	{
		printf("Error: Provide a package path (--open/--create)\n");
		return 0;
	}

	if (kvPath.empty())
	{
		printf("Error: Provide a keyvault path (--keyvault)\n");
		return 0;
	}


	size_t thumbnail_size = 0;
	BYTE* thumbnail_data = 0;
	if (ReadEntireFile("thumbnail.png", &thumbnail_data, &thumbnail_size) == false)
	{
		printf("Error: Failed to read thumbnail\n");
		return 0;
	}


	StfsPackage package(packagePath, StfsPackageCreate);
	package.metaData->titleName = L"Call of Duty 4";
	package.metaData->displayName = L"Call of Duty 4 Save Game";
	package.metaData->contentType = SavedGame;
	package.metaData->titleID = 0x415607E6;
	package.metaData->mediaID = 0x15E8CF88;
	package.metaData->version = 1;
	package.metaData->baseVersion = 1;
	package.metaData->discNumber = 1;
	package.metaData->discInSet = 1;

	BYTE profileId[] = { 0xE0, 0x00, 0x04, 0xFD, 0xAE, 0xE4, 0x01, 0x7C };
	memcpy(package.metaData->profileID, profileId, sizeof(profileId));

	package.metaData->thumbnailImage = thumbnail_data;
	package.metaData->thumbnailImageSize = thumbnail_size;

	package.metaData->titleThumbnailImage = thumbnail_data;
	package.metaData->titleThumbnailImageSize = thumbnail_size;

	package.metaData->WriteMetaData();
	
	package.InjectFile("exploit.bin", "savegame.svg");

	package.Resign(kvPath);
	package.Close();

	return 0;
}