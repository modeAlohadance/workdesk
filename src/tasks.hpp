#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
struct Task{int id;int priority;bool done;std::string title;};
inline std::string encode(const std::vector<Task>& ts){std::ostringstream o;for(auto& t:ts)o<<t.id<<' '<<t.priority<<' '<<t.done<<' '<<std::quoted(t.title)<<'\n';return o.str();}
inline std::vector<Task> decode(const std::string& s){std::istringstream in(s);std::vector<Task> out;Task t;while(in>>std::ws&&!in.eof()){if(!(in>>t.id>>t.priority>>t.done>>std::quoted(t.title))||t.id<1||t.priority<1||t.priority>3||t.title.empty())throw std::runtime_error("Invalid tasks file");for(auto& a:out)if(a.id==t.id)throw std::runtime_error("Duplicate task id");out.push_back(t);}return out;}
inline int nextId(const std::vector<Task>& ts){int n=1;for(auto& t:ts)n=std::max(n,t.id+1);return n;}
