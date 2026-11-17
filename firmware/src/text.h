// Small text helpers for file names coming from the SD card.
#pragma once
#include <ctype.h>
#include <string>

namespace zpod {

// macOS often writes accented file names decomposed ("a" + combining tilde). The display
// font only has the precomposed letters, so join the pairs Portuguese needs.
inline std::string composeAccents(const std::string& in) {
  std::string out;
  for (size_t i = 0; i < in.size(); ++i) {
    unsigned char c = in[i];
    // Combining marks U+0300..U+033F are encoded as 0xCC 0x80..0xBF.
    if (c == 0xCC && i + 1 < in.size() && !out.empty() && isalpha((unsigned char)out.back())) {
      unsigned char mark = in[i + 1];
      char base = out.back();
      char lower = (char)tolower((unsigned char)base);
      int col = -1;   // grave, acute, circumflex, tilde, diaeresis
      switch (mark) {
        case 0x80: col = 0; break;
        case 0x81: col = 1; break;
        case 0x82: col = 2; break;
        case 0x83: col = 3; break;
        case 0x88: col = 4; break;
        default: break;
      }
      int cp = 0;
      static const int a[] = {0xE0, 0xE1, 0xE2, 0xE3, 0xE4}, e[] = {0xE8, 0xE9, 0xEA, 0, 0xEB},
                       ii[] = {0xEC, 0xED, 0xEE, 0, 0xEF}, o[] = {0xF2, 0xF3, 0xF4, 0xF5, 0xF6},
                       u[] = {0xF9, 0xFA, 0xFB, 0, 0xFC};
      if (col >= 0) {
        if (lower == 'a') cp = a[col];
        else if (lower == 'e') cp = e[col];
        else if (lower == 'i') cp = ii[col];
        else if (lower == 'o') cp = o[col];
        else if (lower == 'u') cp = u[col];
        else if (lower == 'n' && col == 3) cp = 0xF1;
      } else if (mark == 0xA7 && lower == 'c') {
        cp = 0xE7;   // cedilla
      }
      if (cp) {
        if (base != lower) cp -= 0x20;   // upper case
        out.pop_back();
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
        ++i;
        continue;
      }
    }
    out += (char)c;
  }
  return out;
}

// "03 - Borboletinha.mp3" -> "Borboletinha"
inline std::string titleFromFileName(const std::string& file) {
  std::string s = file;
  size_t dot = s.rfind('.');
  if (dot != std::string::npos) s.erase(dot);
  size_t i = 0;
  while (i < s.size() && isdigit((unsigned char)s[i])) ++i;
  if (i > 0 && i < s.size()) {
    size_t j = i;
    while (j < s.size() && (s[j] == ' ' || s[j] == '-' || s[j] == '.' || s[j] == '_')) ++j;
    if (j > i && j < s.size()) s.erase(0, j);
  }
  for (auto& ch : s)
    if (ch == '_') ch = ' ';
  return composeAccents(s);
}

}  // namespace zpod
