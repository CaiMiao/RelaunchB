/*-----------------------------------------------------------------
 Not Copyright (ɔ) 2019 - 2021
    FlameKat53
    Pk11
    RocketRobz
    StackZ
------------------------------------------------------------------*/
#include "includes.h"
#include <set>
#include <unordered_map>

// std::string fSlash = "/";
std::string strFatRoot = "fat:/";
std::string strSdRoot = "sd:/";
std::set<std::string> setMp{
	strFatRoot, strSdRoot
};

bool nameEndsWith(const std::string& name, const std::vector<std::string>& extensionList) {
	if(name.substr(0, 2) == "._") return false;

	if(name.size() == 0) return false;

	if(extensionList.size() == 0) return true;

	for(int i = 0; i <(int)extensionList.size(); i++) {
		const std::string ext = extensionList.at(i);
		if(strcasecmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0) return true;
	}
	return false;
}

bool dirEntryPredicate (const DirEntry& lhs, const DirEntry& rhs) {
	if (!lhs.isDirectory && rhs.isDirectory) {
		return false;
	}
	if (lhs.isDirectory && !rhs.isDirectory) {
		return true;
	}
	return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
}

void findNdsFiles(std::vector<DirEntry>& dirContents) {
	std::vector<std::string> extensionList = {"nds", "dsi", "srl", "srldr"};
	findFiles(dirContents, extensionList);
}

std::unordered_map<std::string, std::set<std::string>> mapIgnoredPath{
	{
		"", {
			"_nds",
			"_gba",
			"_dstwo",
			"_dsone",
			"__aio",
			// "__akaio",
			"__rpg",
			"_wfwd",
			"gm9i",
			"DCIM",
			"private",
			"3ds",
			"gm9",
			"luma",
			"Nintendo 3DS",
			"switch",
			"atmosphere",
			"Nintendo",
			"retroarch",
			"roms",
			"games" // todo?: user configurable (bounty: $0 :)
		}
	},
	{
		"_nds/", {
			"nds-bootstrap",
			"TWiLightMenu"
		}
	},
};

bool bStyleDirNameComparsion (std::string path, std::string name) {
	for (auto its = setMp.rbegin();  its != setMp.rend(); its++)
	{
		if (path.substr(0,sizeof(*its)).compare(*its) == 0)
			for (auto itm = mapIgnoredPath.begin(); itm != mapIgnoredPath.end(); itm++)
			{
				if (path.compare(*its + itm->first) == 0)
					if (itm->second.find(name) != itm->second.end())
						return false; // ignored path and name
			}
	}
	return true;
}

void findFiles(std::vector<DirEntry>& dirContents, std::vector<std::string> extensionList) {
	struct stat st;
	DIR *pdir = opendir(".");

	if (pdir == NULL) {
		printf("Internal error, unable to open the directory.");
		for(int i=0;i<120;i++)
			swiWaitForVBlank();
	} else {
		char path[PATH_MAX];
		getcwd(path, PATH_MAX);
		std::string strPath{path};
		while (true) {
			DirEntry dirEntry;

			struct dirent* pent = readdir(pdir);
			if (pent == NULL) break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;

			// Ensure there's a path separator
			if (strPath.back() != '/') strPath += '/';

			dirEntry.fullPath = strPath + dirEntry.name;
			dirEntry.isDirectory = (st.st_mode & S_IFDIR) ? true : false;
			if(!(dirEntry.isDirectory) && dirEntry.name.length() >= 3) {
				if (nameEndsWith(dirEntry.name, extensionList)) {
					dirContents.push_back(dirEntry);
				}
			} else if (dirEntry.isDirectory
				&& dirEntry.name.compare(".") != 0
				&& dirEntry.name.compare("..") != 0
				// && dirEntry.name.compare("_nds/nds-bootstrap") != 0
				// && dirEntry.name.compare("_nds/TWiLightMenu") != 0
				// && dirEntry.name.compare("_gba") != 0
				// && dirEntry.name.compare("3ds") != 0
				// && dirEntry.name.compare("DCIM") != 0
				// && dirEntry.name.compare("gm9") != 0
				// && dirEntry.name.compare("luma") != 0
				// && dirEntry.name.compare("Nintendo 3DS") != 0
				// && dirEntry.name.compare("private") != 0
				// && dirEntry.name.compare("retroarch") != 0
				// && dirEntry.name.compare("roms/sms") != 0
				// && dirEntry.name.compare("roms/sg1000") != 0
				// && dirEntry.name.compare("roms/gen") != 0
				// && dirEntry.name.compare("roms/snes") != 0
				// && dirEntry.name.compare("roms/nes") != 0
				// && dirEntry.name.compare("roms/gb") != 0
				// && dirEntry.name.compare("roms/gbc") != 0
				// && dirEntry.name.compare("roms/gba") != 0
				// && dirEntry.name.compare("roms/gg") != 0
				// && dirEntry.name.compare("roms/nds/saves") != 0) {
				&& bStyleDirNameComparsion(strPath, dirEntry.name)) {
				chdir(dirEntry.name.c_str());
				findNdsFiles(dirContents);
				chdir("..");
			}
		}
		closedir(pdir);
	}
}
