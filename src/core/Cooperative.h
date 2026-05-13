#pragma once
#include <string>

class Cooperative {
public:
    Cooperative(int id, const std::string& nom);
    int get_id() const;
    std::string get_nom() const;
private:
    int m_id;
    std::string m_nom;
};