#pragma once


class Redirector {
public:
    Redirector() {
        mCache["x"] = "y";
    }
    std::string convert(const std::string& hostname) {
        auto it = mCache.find(hostname);
        return it != mCache.end() ? it->second : hostname;
    }

private:
    std::map<std::string, std::string> mCache;
};
