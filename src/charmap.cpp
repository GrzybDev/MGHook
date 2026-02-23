#include "pch.hpp"
#include "charmap.hpp"

void LoadCharSubstitutionMap(const std::wstring& path)
{
	std::ifstream file(path);

	if (!file.is_open())
	{
		Logger::Log(
			"[CharMap] No charmap loaded (file missing or doesn't contain charmap). (Tried path: %s)",
			WideToUtf8(path).c_str());
		return;
	}

	std::string line;
	bool firstLine = true;
	int count = 0;

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

		charMap.push_back({.from = std::move(fromPart), .to = std::move(toPart)});
		++count;
	}

	Logger::Log("[CharMap] Loaded %d char substitution(s).", count);
}

std::string ApplyCharMap(const std::string& utf8Text)
{
	if (charMap.empty())
		return utf8Text;

	std::string result = utf8Text;

	for (const auto& [from, to] : charMap)
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
