#pragma once
#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <stdexcept>

class CommandLineParser
{
public:
	struct Option
	{
		std::string strName;
		std::string strLongName;
		std::string strDescription;
		bool bTakesValue;

		bool bProvided;
		std::string strValue;
	};

	struct Argument
	{
		std::string strName;
		std::string strDescription;

		std::string strValue;
	};

	CommandLineParser(int nArgc, char** ppszArgv)
	{
		for (int i = 0; i < nArgc; i++)
		{
			m_vecProvidedArguments.push_back(ppszArgv[i]);
		}

		m_vecProvidedArguments.erase(m_vecProvidedArguments.begin());
	};

	bool Parse()
	{
		ParseOptions(m_vecProvidedArguments);
		return ParseArguments(m_vecProvidedArguments);
	}

	void AddOption(Option  opt)
	{
		m_vecOptions.push_back(opt);
	};

	void AddOption(std::string strName, std::string strLongName, std::string strDescription, bool bTakesValue = false)
	{
		Option opt = { strName, strLongName, strDescription, bTakesValue };
		m_vecOptions.push_back(opt);
	};

	void AddArgument(Argument arg)
	{
		m_vecArguments.push_back(arg);
	}

	void AddArgument(std::string strName, std::string strDescription)
	{
		Argument arg = { strName, strDescription };
		m_vecArguments.push_back(arg);
	}

	bool OptionExists(std::string strKey)
	{
		for (const auto& opt : m_vecOptions)
		{
			if (strKey == opt.strName || strKey == opt.strLongName)
			{
				return opt.bProvided;
			}
		}
		return false;
	}

	std::string GetOptionValue(std::string strKey)
	{
		for (const auto& opt : m_vecOptions)
		{
			if ((strKey == opt.strName || strKey == opt.strLongName) && opt.bProvided)
			{
				return opt.strValue;
			}
		}
		return "";
	}

	std::string GetArgumentValue(std::string strKey)
	{
		for (const auto& arg : m_vecArguments)
		{
			if (strKey == arg.strName && !arg.strValue.empty())
			{
				return arg.strValue;
			}
		}
		return "";
	}

	void PrintHelp()
	{
		std::cout << "Options:" << std::endl;
		for (const auto& opt : m_vecOptions)
		{
			std::cout << "  " << std::left << std::setw(5) << opt.strName;
			if (!opt.strLongName.empty())
			{
				std::cout << std::setw(15) << opt.strLongName;
			}
			else
			{
				std::cout << std::setw(20) << "";
			}
			std::cout << opt.strDescription;
			if (opt.bTakesValue)
			{
				std::cout << " (Takes a value)";
			}
			std::cout << std::endl;
		}

		std::cout << "Arguments:" << std::endl;
		for (const auto& arg : m_vecArguments)
		{
			std::cout << "  " << std::left << std::setw(20) << arg.strName;
			std::cout << arg.strDescription << std::endl;
		}
	}

	void PrintAll()
	{
		std::cout << "Parsed Options:" << std::endl;
		for (const auto& opt : m_vecOptions)
		{
			if (!opt.bProvided)
				continue;

			std::cout << "  " << std::left << std::setw(5) << opt.strName;
			if (!opt.strLongName.empty())
			{
				std::cout << std::setw(15) << opt.strLongName;
			}
			else
			{
				std::cout << std::setw(20) << "";
			}
			if (opt.bTakesValue && !opt.strValue.empty())
			{
				std::cout << " (Value: " << opt.strValue << ")";
			}
			std::cout << std::endl;
		}

		std::cout << "Parsed Arguments:" << std::endl;
		for (const auto& arg : m_vecArguments)
		{
			std::cout << "  " << std::left << std::setw(20) << arg.strName;
			std::cout << arg.strDescription;
			if (!arg.strValue.empty())
			{
				std::cout << " (Value: " << arg.strValue << ")";
			}
			std::cout << std::endl;
		}
	}

	void Debug()
	{
		for (std::string arg : m_vecProvidedArguments)
		{
			std::cout << arg << std::endl;
		}
	}

private:
	std::vector<std::string> m_vecProvidedArguments;
	std::vector<Option> m_vecOptions;
	std::vector<Argument> m_vecArguments;

	void ParseOptions(std::vector<std::string>& vecProvidedArguments)
	{
		auto ArgsIterator = vecProvidedArguments.begin();
		while (ArgsIterator != vecProvidedArguments.end())
		{
			std::string strOpt = *ArgsIterator;
			if (!(strOpt.size() >= 1 && (strOpt.substr(0, 1) == "-" || strOpt.substr(0, 2) == "--")))
			{
				ArgsIterator++;
				continue;
			}

			bool bFound = false;
			for (auto OptionsIterator = m_vecOptions.begin(); OptionsIterator != m_vecOptions.end(); OptionsIterator++)
			{
				Option* pOpt = &(*OptionsIterator);
				if (pOpt->strName != strOpt && pOpt->strLongName != strOpt)
					continue;

				pOpt->bProvided = true;

				if (pOpt->bTakesValue)
				{
					auto ValueIterator = ArgsIterator;

					try
					{
						ValueIterator++;
						auto It = vecProvidedArguments.end();

						if (ValueIterator >= It)
							continue;

						pOpt->strValue = *ValueIterator;
						vecProvidedArguments.erase(ValueIterator);
					}
					catch (std::exception& e)
					{
						std::cout << e.what();
					}

				}

				bFound = true;
				vecProvidedArguments.erase(ArgsIterator);
				ArgsIterator = vecProvidedArguments.begin();
				break;
			}

			if (!bFound)
			{
				vecProvidedArguments.erase(ArgsIterator); // This is an option but not a valid one so remove it
				ArgsIterator = vecProvidedArguments.begin();
			}
		}
	}

	bool ParseArguments(std::vector<std::string>& vecProvidedArguments)
	{
		if (m_vecArguments.size() > vecProvidedArguments.size())
		{
			std::cout << "Not enough arguments passed to fill the required arguments" << std::endl;
			return false;
		}

		for (auto ArgsIterator = m_vecArguments.begin(); ArgsIterator != m_vecArguments.end(); ArgsIterator++)
		{
			auto ProvidedArgsIterator = vecProvidedArguments.begin();
			(*ArgsIterator).strValue = (*ProvidedArgsIterator);
			vecProvidedArguments.erase(ProvidedArgsIterator);
		}

		vecProvidedArguments.clear();
		return true;
	}
};

#endif