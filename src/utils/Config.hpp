#pragma once

#include "oatpp/core/Types.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>

#include "jwt/json/json.hpp"
using njson = nlohmann::json;

const long PAGE_LIMIT=10;

class Config{
public:
    int dataThread,ioThread,timerThread;
    v_uint16 hostPort,dbPool,dbTtl;
    oatpp::String hostAddress,secretKey,dbUrl;
    oatpp::String staticFilePath,upAndDownPath;
    bool publicMode;

    void parseConfig(const std::string& content){
        auto j = njson::parse(content);

        auto threadCfg = j["thread"];
        if(threadCfg.is_null()){
            OATPP_LOGE("Config", "read THREAD config error!");
            exit(1);
        }
        dataThread = threadCfg["dataThread"].get<int>();
        ioThread = threadCfg["ioThread"].get<int>();
        timerThread = threadCfg["timerThread"].get<int>();

        auto server = j["server"];
        if(server.is_null()){
            OATPP_LOGE("Config", "read SERVER config error!");
            exit(1);
        }
        hostAddress = oatpp::String(server["address"].get<std::string>().c_str());
        hostPort = (v_uint16)server["port"].get<int>();
        secretKey = oatpp::String(server["secretKey"].get<std::string>().c_str());
        publicMode = server["publicMode"].get<bool>();
        staticFilePath = oatpp::String(server["staticFilePath"].get<std::string>().c_str());
        upAndDownPath = oatpp::String(server["upAndDownPath"].get<std::string>().c_str());

        auto db = j["postgres"];
        if(db.is_null()){
            OATPP_LOGE("Config", "read DB config error!");
            exit(1);
        }
        dbUrl = oatpp::String(db["url"].get<std::string>().c_str());
        dbPool = (v_uint16)db["pool"].get<int>();
        dbTtl = (v_uint16)db["ttl"].get<int>();
    }
    Config(const char *filename){
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            OATPP_LOGE("Config", "read config file error: %s", filename);
            exit(1);
        }

        std::string content((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());

        try {
            parseConfig(content);
        } catch (const std::exception& e){
            std::cerr << "Config parse error: " << e.what() << std::endl;
            exit(1);
        } catch (...){
            std::cerr << "Something wrong with config file." << std::endl;
            exit(1);
        }
    }
};
