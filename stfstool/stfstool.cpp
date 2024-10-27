#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <map>
#include <algorithm>
#include <cwctype>

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

std::map<std::wstring, ContentType> ContentTypeMap = {
	{L"arcadegame", ArcadeGame},
	{L"avatarassetpack", AvatarAssetPack},
	{L"avataritem", AvatarItem},
	{L"cachefile", CacheFile},
	{L"communitygame", CommunityGame},
	{L"gamedemo", GameDemo},
	{L"gameondemand", GameOnDemand},
	{L"gamerpicture", GamerPicture},
	{L"gamertitle", GamerTitle},
	{L"gametrailer", GameTrailer},
	{L"gamevideo", GameVideo},
	{L"installedgame", InstalledGame},
	{L"installer", Installer},
	{L"iptvpausebuffer", IPTVPauseBuffer},
	{L"licensestore", LicenseStore},
	{L"marketplacecontent", MarketPlaceContent},
	{L"movie", Movie},
	{L"musicvideo", MusicVideo},
	{L"podcastvideo", PodcastVideo},
	{L"profile", Profile},
	{L"publisher", Publisher},
	{L"savedgame", SavedGame},
	{L"storagedownload", StorageDownload},
	{L"theme", Theme},
	{L"video", Video},
	{L"viralvideo", ViralVideo},
	{L"xboxdownload", XboxDownload},
	{L"xboxoriginalgame", XboxOriginalGame},
	{L"xboxsavedgame", XboxSavedGame},
	{L"xbox360title", Xbox360Title},
	{L"xna", XNA}
};

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

bool ReadEntireFile(std::wstring path, BYTE** contents, size_t* size)
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

	size_t thumbnailSize = 0;
	BYTE* thumbnailData = 0;
	std::wstring thumbnailPath;
	size_t titleThumbnailSize = 0;
	BYTE* titleThumbnailData = 0;
	std::wstring titleThumbnailPath;
	DWORD version = 0;
	DWORD baseVersion = 0;
	DWORD titleId = 0xFFFE07D1; // Xbox 360 Dashboard title id by default
	DWORD mediaId = 0;
	BYTE discNumber = 0;
	BYTE discInSet = 0;
	BYTE profileId[8] = { 0 };
	BYTE deviceId[20] = { 0 };
	ContentType contentType = SavedGame;

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

			if (profileIdStr.length() != sizeof(profileId) * 2)
			{
				printf("Error: Profile ID should be %i characters long, "
					"provided %i (--profile-id)\n", sizeof(profileId) * 2,
					profileIdStr.length());

				return 0;
			}

			swscanf_s(profileIdStr.c_str(),
				L"%02X%02X%02X%02X%02X%02X%02X%02X",
				&profileId[0], &profileId[1], &profileId[2], &profileId[3],
				&profileId[4], &profileId[5], &profileId[6], &profileId[7]);
		}
		else if (cur_arg == L"-did" || cur_arg == L"--device-id")
		{
			NEXT_ARG("--device-id");
			std::wstring deviceIdStr(argv[i]);

			if (deviceIdStr.length() != sizeof(deviceId) * 2)
			{
				printf("Error: Profile ID should be %i characters long, "
					"provided %i (--device-id)\n", sizeof(deviceId) * 2,
					deviceIdStr.length());

				return 0;
			}

			swscanf_s(deviceIdStr.c_str(),
				L"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
				 "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
				&deviceId[0], &deviceId[1], &deviceId[2], &deviceId[3],
				&deviceId[4], &deviceId[5], &deviceId[6], &deviceId[7],
				&deviceId[8], &deviceId[9], &deviceId[10], &deviceId[11],
				&deviceId[12], &deviceId[13], &deviceId[14], &deviceId[15],
				&deviceId[16], &deviceId[17], &deviceId[18], &deviceId[19],
				&deviceId[20]);
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
		else if (cur_arg == L"-mid" || cur_arg == L"--media-id")
		{
			NEXT_ARG("--media-id");
			std::wstring mediaIdStr(argv[i]);

			if (mediaIdStr.length() != 8)
			{
				printf("Error: Media ID should be 8 characters long, "
					"provided %i (--profile-id)\n", mediaIdStr.length());

				return 0;
			}

			swscanf_s(mediaIdStr.c_str(), L"%08X", &mediaId);
		}
		else if (cur_arg == L"-v" || cur_arg == L"--version")
		{
			NEXT_ARG("--version");
			std::wstring versionStr(argv[i]);

			swscanf_s(versionStr.c_str(), L"%i", &version);
		}
		else if (cur_arg == L"-bv" || cur_arg == L"--base-version")
		{
			NEXT_ARG("--base-version");
			std::wstring baseVersionStr(argv[i]);

			swscanf_s(baseVersionStr.c_str(), L"%i", &baseVersion);
		}
		else if (cur_arg == L"-dn" || cur_arg == L"--disc-number")
		{
			NEXT_ARG("--disc-number");
			std::wstring discNumberStr(argv[i]);

			// can't scanf for a single byte so read as int and cast down
			int discNumInt = 0;
			swscanf_s(discNumberStr.c_str(), L"%i", &discNumInt);
			discNumber = discNumInt;
		}
		else if (cur_arg == L"-dis" || cur_arg == L"--disc-in-set")
		{
			NEXT_ARG("--disc-in-set");
			std::wstring discInSetStr(argv[i]);

			// can't scanf for a single byte so read as int and cast down
			int discInSetInt = 0;
			swscanf_s(discInSetStr.c_str(), L"%i", &discInSetInt);
			discInSet = discInSetInt;
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
		else if (cur_arg == L"-thumb" || cur_arg == L"--thumbnail")
		{
			NEXT_ARG("--thumbnail");
			thumbnailPath = std::wstring(argv[i]);
		}
		else if (cur_arg == L"-tthumb" || cur_arg == L"--title-thumbnail")
		{
			NEXT_ARG("--title-thumbnail");
			titleThumbnailPath = std::wstring(argv[i]);
		}
		else if (cur_arg == L"-ct" || cur_arg == L"--content-type")
		{
			NEXT_ARG("--content-type");
			std::wstring contentTypeStr(argv[i]);
			std::transform(contentTypeStr.begin(), contentTypeStr.end(),
				contentTypeStr.begin(), [](wchar_t c) 
				{ return std::towlower(c); });


			if (!ContentTypeMap.contains(contentTypeStr))
			{
				printf("Error: Invalid content type! (--content-type)\n");
				return 0;
			}

			contentType = ContentTypeMap[contentTypeStr];
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


	// read the thumbnails
	if (!thumbnailPath.empty())
	{
		if (ReadEntireFile(thumbnailPath, &thumbnailData, 
							&thumbnailSize) == false)
		{
			printf("Error: Failed to read thumbnail\n");
			return 0;
		}
	}
	if (!titleThumbnailPath.empty())
	{
		if (ReadEntireFile(titleThumbnailPath, &titleThumbnailData,
							&titleThumbnailSize) == false)
		{
			printf("Error: Failed to read title thumbnail\n");
			return 0;
		}
	}


	StfsPackage package(packagePath, StfsPackageCreate);
	package.metaData->titleName = titleName;
	package.metaData->displayName = displayName;
	package.metaData->contentType = contentType;
	package.metaData->titleID = titleId;
	package.metaData->mediaID = mediaId;
	package.metaData->version = version;
	package.metaData->baseVersion = baseVersion;
	package.metaData->discNumber = discNumber;
	package.metaData->discInSet = discInSet;

	memcpy(package.metaData->profileID, profileId, sizeof(profileId));
	memcpy(package.metaData->deviceID, deviceId, sizeof(deviceId));

	package.metaData->thumbnailImage = thumbnailData;
	package.metaData->thumbnailImageSize = thumbnailSize;

	package.metaData->titleThumbnailImage = titleThumbnailData;
	package.metaData->titleThumbnailImageSize = titleThumbnailSize;

	package.metaData->WriteMetaData();

	package.InjectFile("exploit.bin", "savegame.svg");

	package.Resign(kvPath);
	package.Close();

	return 0;
}