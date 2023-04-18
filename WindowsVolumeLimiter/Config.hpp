#include <filesystem>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

class Config
{
private:
	float maxVolume;
	float setVolume;

	std::vector<std::string> split(const std::string& input, const std::string& regex)
	{
		std::regex re(regex);
		std::sregex_token_iterator
			first{ input.begin(), input.end(), re, -1 },
			last;
		return { first, last };
	}

	void initConfigFile()
	{
		std::cout << "init config.cfg file" << std::endl;
		std::ofstream  configFile(getCfgPath());
		configFile.clear();
		configFile << "maxVolume:0.7" << std::endl;
		configFile << "setVolume:0.1" << std::endl;
		configFile.close();
	}

	void initBatFile()
	{
		std::cout << "init start.bat file" << std::endl;
		std::ofstream  batFile(getBatPath());
		batFile.clear();
		batFile << "start \"\" " << getExePath() << std::endl;
		batFile.close();
	}

	void writeConfigFile()
	{
		std::cout << "Write config to config.cfg" << std::endl;
		std::ofstream  configFile(getCfgPath());
		configFile.clear();
		configFile << "maxVolume:" << this->maxVolume << std::endl;
		configFile << "setVolume:" << this->setVolume << std::endl;
		configFile.close();
	}

	void processConfigString(std::string& str)
	{
		auto config = split(str, ":");
		if (config.size() != 2)
		{
			std::cout << "config format invalid: " << str << std::endl;
			return;
		}
		if (config[0].compare("maxVolume") == 0)
		{
			std::cout << "config: " << config[0] << " => " << config[1] << std::endl;
			this->maxVolume = std::stof(config[1]);
		}
		else if (config[0].compare("setVolume") == 0)
		{
			std::cout << "config: " << config[0] << " => " << config[1] << std::endl;
			this->setVolume = std::stof(config[1]);
		}
		else
		{
			std::cout << "Unknown Config: " << config[0] << std::endl;
		}
	}

	void readConfigFile()
	{
		std::cout << "read config.cfg file" << std::endl;
		std::string buf;
		std::ifstream configFile(getCfgPath());
		while (std::getline(configFile, buf))
		{
			processConfigString(buf);
		}
		configFile.close();
	}

public:

	fs::path getExePath()
	{
		TCHAR currPath[MAX_PATH];
		GetModuleFileName(NULL, currPath, MAX_PATH);
		auto currFSPath = fs::path(currPath);
		return currFSPath;
	}

	fs::path getCfgPath()
	{
		TCHAR currPath[MAX_PATH];
		GetModuleFileName(NULL, currPath, MAX_PATH);
		auto currFSPath = fs::path(currPath);
		return currFSPath.parent_path() / fs::path("config.cfg");
	}

	fs::path getBatPath()
	{
		TCHAR currPath[MAX_PATH];
		GetModuleFileName(NULL, currPath, MAX_PATH);
		auto currFSPath = fs::path(currPath);
		return currFSPath.parent_path() / fs::path("start.bat");
	}

	float getMaxVolume()
	{
		return this->maxVolume;
	}

	float getSetVolume()
	{
		return this->setVolume;
	}

	void setMaxVolume(float vol)
	{
		if (vol <= .0 || vol >= 1.)
		{
			std::cout << "maxVolume value invalid: " << vol << std::endl;
			return;
		}
		this->maxVolume = vol;
		writeConfigFile();
	}

	void setSetVolume(float vol)
	{
		if (vol <= .0 || vol >= 1.)
		{
			std::cout << "setVolume value invalid: " << vol << std::endl;
			return;
		}
		this->setVolume = vol;
		writeConfigFile();
	}

	Config()
	{
		if (fs::exists(getCfgPath()))
		{
			std::cout << "config.cfg found" << std::endl;
			readConfigFile();
		}
		else
		{
			std::cout << "config.cfg not found" << std::endl;
			initConfigFile();
		}
		if (!fs::exists(getBatPath()))
		{
			std::cout << "start.bat found" << std::endl;
			initBatFile();
		}
	}
};