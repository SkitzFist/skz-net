#ifndef SKZ_NET_GREETER_H_
#define SKZ_NET_GREETER_H_

#include <string>

class Greeter{
public:
  Greeter(const std::string& str) : m_str(str){}
  const std::string& greet() {return m_str; }
private:
  std::string m_str;
};

#endif