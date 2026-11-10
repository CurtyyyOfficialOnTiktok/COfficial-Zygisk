#include <jni.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

struct DeviceProfile {
    const char* brand;
    const char* manufacturer;
    const char* model;
    const char* device;
};

static const std::unordered_map<std::string, DeviceProfile> DEVICES = {
    {"S26_ULTRA",    {"samsung", "samsung", "SM-S948B", "e3q"}},
    {"IPHONE_18_PM", {"Apple", "Apple", "iPhone18,2", "iPhone18ProMax"}},
    {"ZFOLD_8_U",    {"samsung", "samsung", "SM-F976B", "q8q"}},
    {"ROG_9_PRO",    {"asus", "ASUSTeK COMPUTER INC.", "ASUS_AI2501", "AI2501"}},
    {"RM_10_PRO",    {"nubia", "nubia", "NX789J", "NX789J"}},
    {"RM_11S_PRO",   {"nubia", "nubia", "NX799J", "NX799J"}}
};

class COfficialModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *raw_pkg = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!raw_pkg) return;
        std::string pkg(raw_pkg);
        env->ReleaseStringUTFChars(args->nice_name, raw_pkg);

        std::string conf_path = "/data/adb/cofficial/apps/" + pkg + ".conf";
        std::ifstream file(conf_path);
        if (!file.is_open()) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        std::string line, spoof_key = "DEFAULT";
        bool enabled = false;
        while (std::getline(file, line)) {
            if (line.rfind("ENABLED=1", 0) == 0) enabled = true;
            if (line.rfind("SPOOF=", 0) == 0) spoof_key = line.substr(6);
        }

        if (!enabled || spoof_key == "DEFAULT" || DEVICES.find(spoof_key) == DEVICES.end()) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const auto& dev = DEVICES.at(spoof_key);
        jclass build_cls = env->FindClass("android/os/Build");
        if (build_cls) {
            auto set_field = [&](const char* name, const char* val) {
                jfieldID fid = env->GetStaticFieldID(build_cls, name, "Ljava/lang/String;");
                if (fid) env->SetStaticObjectField(build_cls, fid, env->NewStringUTF(val));
            };
            set_field("BRAND", dev.brand);
            set_field("MANUFACTURER", dev.manufacturer);
            set_field("MODEL", dev.model);
            set_field("DEVICE", dev.device);
        }
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(COfficialModule)
