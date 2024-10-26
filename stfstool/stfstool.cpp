#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>

#include <stdio.h>
#include <Stfs/StfsPackage.h>

#define VERSION_STR "v0.1"

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
	std::wstring path;
	std::string nameInPackage;
} FileInjectInfo;

// From https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

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

void PrintBanner()
{
	printf("STFSTool " VERSION_STR " by carrot_c4k3\n");
	printf("based on Velocity by Hetelek and Experiment5X\n");
}

void PrintUsage()
{
	printf("\n\tUsage: stfstool [--create]|[--open] [filename] <options>\n\n");
	printf("For detailed information on all options pass --help\n\n");
}


int wmain(int argc, wchar_t** argv)
{
	StfsOpenMode openMode = Unknown;
	std::string packagePath;
	std::string kvPath;

	// package metadata
	std::wstring titleName;
	std::wstring displayName;
	DWORD titleId = 0xFFFE07D1; // Xbox 360 Dashboard title id by default
	BYTE profileId[8] = { 0 };
	std::vector<FileInjectInfo> filesToInject;

	PrintBanner();
	if (argc < 2)
	{
		PrintUsage();
		return 0;
	}

	for (int i = 0; i < argc; i++)
	{
		std::wstring cur_arg(argv[i]);

		// open mode
		if (cur_arg == L"-c" || cur_arg == L"--create")
		{
			openMode = Create;

			NEXT_ARG("--create");
			std::wstring wpackagePath(argv[i]);
			packagePath = ws2s(wpackagePath);
		}
		else if (cur_arg == L"-o" || cur_arg == L"--open")
		{
			openMode = OpenExisting;

			NEXT_ARG("--open");
			std::wstring wpackagePath(argv[i]);
			packagePath = ws2s(wpackagePath);
		}
		else if (cur_arg == L"-kv" || cur_arg == L"--keyvault")
		{
			NEXT_ARG("--keyvault");
			std::wstring wkvPath(argv[i]);
			kvPath = ws2s(wkvPath);
		}
		else if (cur_arg == L"-if" || cur_arg == L"--inject-file")
		{
			FileInjectInfo injectInfo;

			NEXT_ARG("--inject_file");
			injectInfo.path = std::wstring(argv[i]);

			NEXT_ARG("--inject_file");
			std::wstring nameInPackage(argv[i]);
			injectInfo.nameInPackage = ws2s(nameInPackage);

			filesToInject.push_back(injectInfo);
		}
		else if (cur_arg == L"-pid" || cur_arg == L"--profile-id")
		{
			NEXT_ARG("--profile-id");
			std::wstring profileIdStr(argv[i]);

			if (profileIdStr.length() != 0x10)
			{
				printf("Error: Profile ID should be 16 characters long, "
					"provided %i (--profile-id)\n", profileIdStr.length());

				return 0;
			}

			swscanf_s(profileIdStr.c_str(),
				L"%02X%02X%02X%02X%02X%02X%02X%02X",
				&profileId[0], &profileId[1], &profileId[2], &profileId[3],
				&profileId[4], &profileId[5], &profileId[6], &profileId[7]);
		}
		else if (cur_arg == L"-tid" || cur_arg == L"--title-id")
		{
			NEXT_ARG("--title-id");
			std::wstring titleIdStr(argv[i]);

			if (titleIdStr.length() != 8)
			{
				printf("Error: Title ID should be 8 characters long, "
					"provided %i (--profile-id)\n", titleIdStr.length());

				return 0;
			}

			swscanf_s(titleIdStr.c_str(), L"%08X", &titleId);
		}
		else if (cur_arg == L"-tn" || cur_arg == L"--title-name")
		{
			NEXT_ARG("--title-name");
			titleName = std::wstring(argv[i]);
		}
		else if (cur_arg == L"-dn" || cur_arg == L"--display-name")
		{
			NEXT_ARG("--display-name");
			displayName = std::wstring(argv[i]);
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
	package.metaData->titleName = titleName;
	package.metaData->displayName = displayName;
	package.metaData->contentType = SavedGame;
	package.metaData->titleID = titleId;
	package.metaData->mediaID = 0x15E8CF88;
	package.metaData->version = 1;
	package.metaData->baseVersion = 1;
	package.metaData->discNumber = 1;
	package.metaData->discInSet = 1;

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