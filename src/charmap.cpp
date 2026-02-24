#include "pch.hpp"
#include "charmap.hpp"

static std::vector<CharMapEntry> ParseCharMapFile(const std::wstring& path)
{
	std::vector<CharMapEntry> entries;
	std::ifstream file(path);

	if (!file.is_open())
		return entries;

	std::string line;
	bool firstLine = true;

	while (std::getline(file, line))
	{
		if (firstLine)
		{
			firstLine = false;

			if (line.size() >= 3 &&
				line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF')
				line = line.substr(3);
		}

		line = Trim(line);

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		// Strip inline comment  (;  or  #  after the value)
		// We must be careful: the '=' separates key from value,
		// and the first ';' or '#' after the value is a comment.
		const auto sep = line.find('=');
		if (sep == std::string::npos)
			continue;

		std::string fromPart = Trim(line.substr(0, sep));
		std::string rest = line.substr(sep + 1);

		// Strip inline comment from the value part
		for (size_t i = 0; i < rest.size(); ++i)
		{
			if (rest[i] == ';' || rest[i] == '#')
			{
				rest = rest.substr(0, i);
				break;
			}
		}
		std::string toPart = Trim(rest);

		if (fromPart.empty() || toPart.empty()) continue;

		// Validate: both sides must be exactly one UTF-8 character
		const int wlenFrom = MultiByteToWideChar(CP_UTF8, 0, fromPart.c_str(),
		                                         static_cast<int>(fromPart.size()),
		                                         nullptr, 0);
		const int wlenTo = MultiByteToWideChar(CP_UTF8, 0, toPart.c_str(),
		                                       static_cast<int>(toPart.size()),
		                                       nullptr, 0);

		if (wlenFrom != 1 || wlenTo != 1)
			continue;

		entries.push_back({.from = std::move(fromPart), .to = std::move(toPart)});
	}

	return entries;
}

void LoadCharSubstitutionMap(const std::wstring& path)
{
	auto entries = ParseCharMapFile(path);

	if (entries.empty())
	{
		Logger::Log(
			"[CharMap] No global charmap loaded (file missing or empty). (Tried path: %s)",
			WideToUtf8(path).c_str());
		return;
	}

	charMap = std::move(entries);
	Logger::Log("[CharMap] Loaded %d global char substitution(s).", static_cast<int>(charMap.size()));
}

void LoadRegionCharSubstitutionMap(const std::wstring& path, int regionIndex)
{
	auto entries = ParseCharMapFile(path);

	if (entries.empty())
		return;

	Logger::Log("[CharMap] Loaded %d char substitution(s) for region %s.",
	            static_cast<int>(entries.size()),
	            STRING_REGIONS[regionIndex].name);

	regionCharMaps[regionIndex] = std::move(entries);
}

static std::string ApplyCharMapEntries(const std::string& utf8Text, const std::vector<CharMapEntry>& entries)
{
	if (entries.empty())
		return utf8Text;

	std::string result = utf8Text;

	for (const auto& [from, to] : entries)
	{
		std::string out;
		size_t pos = 0;
		const size_t fromLen = from.size();

		while (pos < result.size())
		{
			const size_t found = result.find(from, pos);

			if (found == std::string::npos)
			{
				out.append(result, pos, result.size() - pos);
				break;
			}

			out.append(result, pos, found - pos);
			out.append(to);
			pos = found + fromLen;
		}

		result = std::move(out);
	}

	return result;
}

std::string ApplyCharMap(const std::string& utf8Text, int regionIndex)
{
	std::string result = ApplyCharMapEntries(utf8Text, charMap);

	if (regionIndex >= 0)
	{
		const auto it = regionCharMaps.find(regionIndex);
		if (it != regionCharMaps.end())
			result = ApplyCharMapEntries(result, it->second);
	}

	return result;
}
