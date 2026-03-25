#pragma once
#include <map>
#include <string>
#include <vector>

// Un ScopeTable est une liste de dictionnaires.
// Chaque dictionnaire fait le lien entre le "vrai" nom (a) et le "nom unique" (a_1) pour un bloc donné.
using ScopeTable = std::vector<std::map<std::string, std::string>>;