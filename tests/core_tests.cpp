#include "../src/tasks.hpp"
#include <iostream>
int main(){try{std::vector<Task> t{{1,1,false,"Задача, \"кавычки\"\nстрока"},{4,3,true,"Done"}};auto r=decode(encode(t));if(r.size()!=2||r[0].title!=t[0].title||!r[1].done||nextId(r)!=5)throw std::runtime_error("Round trip");for(auto bad:{"0 1 0 \"bad\"","1 4 0 \"bad\"","1 1 0 \"\"","broken","1 1 0 \"a\"\n1 1 0 \"b\""}){bool rejected=false;try{decode(bad);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Accepted invalid data");}return 0;}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
