#include "ncbind.hpp"
#include <string>


#define NCB_MODULE_NAME TJS_W("steam_api.dll")

class steam_api {
public:
    std::string vserion = "1.0.0";
};

NCB_REGISTER_CLASS(steam_api) { Constructor(); }