#pragma once

std::vector<TranslationEntry> LoadTranslations(const std::wstring& path);
TranslationResult ApplyTranslations(const std::vector<TranslationEntry>& translations, const AddressTuple& addresses);
