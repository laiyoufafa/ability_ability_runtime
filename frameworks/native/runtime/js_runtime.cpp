/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "js_runtime.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <sys/epoll.h>

#include "native_engine/impl/ark/ark_native_engine.h"
#ifdef SUPPORT_GRAPHICS
#include "core/common/container_scope.h"
#include "declarative_module_preloader.h"
#endif
#include "event_handler.h"
#include "hilog_wrapper.h"
#include "js_console_log.h"
#include "js_module_searcher.h"
#include "js_runtime_utils.h"

#ifdef ENABLE_HITRACE
#include "hitrace/trace.h"
#endif
#include "systemcapability.h"
#include "parameters.h"

namespace OHOS {
namespace AbilityRuntime {
namespace {
constexpr uint8_t SYSCAP_MAX_SIZE = 64;
constexpr int64_t DEFAULT_GC_POOL_SIZE = 0x10000000; // 256MB
constexpr int64_t ASSET_FILE_MAX_SIZE = 20 * 1024 * 1024;
#if defined(_ARM64_)
constexpr char ARK_DEBUGGER_LIB_PATH[] = "/system/lib64/libark_debugger.z.so";
#else
constexpr char ARK_DEBUGGER_LIB_PATH[] = "/system/lib/libark_debugger.z.so";
#endif

constexpr char TIMER_TASK[] = "uv_timer_task";

class ArkJsRuntime : public JsRuntime {
public:
    ArkJsRuntime()
    {
        isArkEngine_ = true;
    }

    ~ArkJsRuntime() override
    {
        Deinitialize();
        if (vm_ != nullptr) {
            panda::JSNApi::DestroyJSVM(vm_);
            vm_ = nullptr;
        }
    }

    void StartDebugMode(bool needBreakPoint, int32_t instanceId) override
    {
        if (!debugMode_) {
            HILOG_INFO("Ark VM is starting debug mode [%{public}s]", needBreakPoint ? "break" : "normal");
            panda::JSNApi::StartDebugger(ARK_DEBUGGER_LIB_PATH, vm_, needBreakPoint, instanceId);
            debugMode_ = true;
        }
    }

    bool RunScript(const std::string& path) override
    {
        static const char PANDA_MAIN_FUNCTION[] = "_GLOBAL::func_main_0";
        return vm_ != nullptr ? panda::JSNApi::Execute(vm_, path.c_str(), PANDA_MAIN_FUNCTION) : false;
    }

    NativeValue* LoadJsModule(const std::string& path) override
    {
        if (!RunScript(path)) {
            HILOG_ERROR("Failed to run script: %{public}s", path.c_str());
            return nullptr;
        }

        panda::Local<panda::ObjectRef> exportObj = panda::JSNApi::GetExportObject(vm_, path, "default");
        if (exportObj->IsNull()) {
            HILOG_ERROR("Get export object failed");
            return nullptr;
        }

        return ArkNativeEngine::ArkValueToNativeValue(
            static_cast<ArkNativeEngine*>(nativeEngine_.get()), exportObj);
    }

private:
    static int32_t PrintVmLog(int32_t id, int32_t level, const char* tag, const char* fmt, const char* message)
    {
        HILOG_INFO("ArkLog: %{public}s", message);
        return 0;
    }

    bool Initialize(const Runtime::Options& options) override
    {
        panda::RuntimeOption pandaOption;
        int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
        size_t gcThreadNum = OHOS::system::GetUintParameter<size_t>("persist.ark.gcthreads", 7);
        size_t longPauseTime = OHOS::system::GetUintParameter<size_t>("persist.ark.longpausetime", 40);
        pandaOption.SetArkProperties(arkProperties);
        pandaOption.SetGcThreadNum(gcThreadNum);
        pandaOption.SetLongPauseTime(longPauseTime);
        HILOG_INFO("ArkJSRuntime::Initialize ark properties = %{public}d", arkProperties);
        pandaOption.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
        pandaOption.SetGcPoolSize(DEFAULT_GC_POOL_SIZE);
        pandaOption.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
        pandaOption.SetLogBufPrint(PrintVmLog);
        vm_ = panda::JSNApi::CreateJSVM(pandaOption);
        if (vm_ == nullptr) {
            return false;
        }

        panda::JSNApi::SetHostResolvePathTracker(vm_, JsModuleSearcher(options.bundleName));

        nativeEngine_ = std::make_unique<ArkNativeEngine>(vm_, static_cast<JsRuntime*>(this));
        return JsRuntime::Initialize(options);
    }

    panda::ecmascript::EcmaVM* vm_ = nullptr;
};

NativeValue* SetTimeout(NativeEngine* engine, NativeCallbackInfo* info)
{
    if (engine == nullptr || info == nullptr) {
        HILOG_ERROR("Set timeout failed with engine or callback info is nullptr.");
        return nullptr;
    }

    JsRuntime& jsRuntime = *reinterpret_cast<JsRuntime*>(engine->GetJsEngine());
    return jsRuntime.SetCallbackTimer(*engine, *info, false);
}

NativeValue* CanIUse(NativeEngine* engine, NativeCallbackInfo* info)
{
    if (engine == nullptr || info == nullptr) {
        HILOG_ERROR("get syscap failed since engine or callback info is nullptr.");
        return nullptr;
    }

    if (info->argc != 1 || info->argv[0]->TypeOf() != NATIVE_STRING) {
        HILOG_ERROR("Get syscap failed with invalid parameter.");
        return engine->CreateUndefined();
    }

    char syscap[SYSCAP_MAX_SIZE] = { 0 };

    NativeString* str = ConvertNativeValueTo<NativeString>(info->argv[0]);
    if (str == nullptr) {
        HILOG_ERROR("Convert to NativeString failed.");
        return engine->CreateUndefined();
    }
    size_t bufferLen = str->GetLength();
    size_t strLen = 0;
    str->GetCString(syscap, bufferLen + 1, &strLen);

    bool ret = HasSystemCapability(syscap);
    return engine->CreateBoolean(ret);
}

NativeValue* SetInterval(NativeEngine* engine, NativeCallbackInfo* info)
{
    if (engine == nullptr || info == nullptr) {
        HILOG_ERROR("Set interval failed with engine or callback info is nullptr.");
        return nullptr;
    }

    JsRuntime& jsRuntime = *reinterpret_cast<JsRuntime*>(engine->GetJsEngine());
    return jsRuntime.SetCallbackTimer(*engine, *info, true);
}

NativeValue* ClearTimeoutOrInterval(NativeEngine* engine, NativeCallbackInfo* info)
{
    if (engine == nullptr || info == nullptr) {
        HILOG_ERROR("Clear timer failed with engine or callback info is nullptr.");
        return nullptr;
    }

    JsRuntime& jsRuntime = *reinterpret_cast<JsRuntime*>(engine->GetJsEngine());
    return jsRuntime.ClearCallbackTimer(*engine, *info);
}

void InitTimerModule(NativeEngine& engine, NativeObject& globalObject)
{
    BindNativeFunction(engine, globalObject, "setTimeout", SetTimeout);
    BindNativeFunction(engine, globalObject, "setInterval", SetInterval);
    BindNativeFunction(engine, globalObject, "clearTimeout", ClearTimeoutOrInterval);
    BindNativeFunction(engine, globalObject, "clearInterval", ClearTimeoutOrInterval);
}

void InitSyscapModule(NativeEngine& engine, NativeObject& globalObject)
{
    BindNativeFunction(engine, globalObject, "canIUse", CanIUse);
}

bool MakeFilePath(const std::string& codePath, const std::string& modulePath, std::string& fileName)
{
    std::string path(codePath);
    path.append("/").append(modulePath);
    if (path.length() > PATH_MAX) {
        HILOG_ERROR("Path length(%{public}d) longer than MAX(%{public}d)", (int32_t)path.length(), PATH_MAX);
        return false;
    }
    char resolvedPath[PATH_MAX + 1] = { 0 };
    if (realpath(path.c_str(), resolvedPath) != nullptr) {
        fileName = resolvedPath;
        return true;
    }

    auto start = path.find_last_of('/');
    auto end = path.find_last_of('.');
    if (end == std::string::npos || end == 0) {
        HILOG_ERROR("No secondary file path");
        return false;
    }

    auto pos = path.find_last_of('.', end - 1);
    if (pos == std::string::npos) {
        HILOG_ERROR("No secondary file path");
        return false;
    }

    path.erase(start + 1, pos - start);
    HILOG_INFO("Try using secondary file path: %{public}s", path.c_str());

    if (realpath(path.c_str(), resolvedPath) == nullptr) {
        HILOG_ERROR("Failed to call realpath, errno = %{public}d", errno);
        return false;
    }

    fileName = resolvedPath;
    return true;
}

void RegisterInitWorkerFunc(NativeEngine& engine)
{
    auto&& initWorkerFunc = [](NativeEngine* nativeEngine) {
        HILOG_INFO("RegisterInitWorkerFunc called");
        if (nativeEngine == nullptr) {
            HILOG_ERROR("Input nativeEngine is nullptr");
            return;
        }

        NativeObject* globalObj = ConvertNativeValueTo<NativeObject>(nativeEngine->GetGlobal());
        if (globalObj == nullptr) {
            HILOG_ERROR("Failed to get global object");
            return;
        }

        InitConsoleLogModule(*nativeEngine, *globalObj);
    };
    engine.SetInitWorkerFunc(initWorkerFunc);
}

bool GetResourceData(const std::string& filePath, std::vector<uint8_t>& content)
{
    std::ifstream stream(filePath);
    if (!stream.is_open()) {
        HILOG_ERROR("GetResourceData failed with file can't open, check uri.");
        return false;
    }

    stream.seekg(0, std::ios::end);
    auto fileLen = stream.tellg();
    if (fileLen > ASSET_FILE_MAX_SIZE) {
        HILOG_ERROR("GetResourceData failed with file too large.");
        stream.close();
        return false;
    }

    content.resize(fileLen);
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(content.data()), content.size());
    stream.close();

    return true;
}

void RegisterAssetFunc(NativeEngine& engine, const std::string& codePath)
{
    auto&& assetFunc = [codePath](const std::string& uri, std::vector<uint8_t>& content, std::string &ami) {
        if (uri.empty()) {
            HILOG_ERROR("Uri is empty.");
            return;
        }

        HILOG_INFO("RegisterAssetFunc called, uri: %{private}s", uri.c_str());
        size_t index = uri.find_last_of(".");
        if (index == std::string::npos) {
            HILOG_ERROR("Invalid uri");
            return;
        }

        std::string fileUri = uri.substr(0, index) + ".abc";
        std::string targetFile = codePath;
        targetFile += (codePath.back() == '/') ? fileUri : "/" + fileUri;
        ami = targetFile;
        HILOG_INFO("Get asset, ami: %{private}s", ami.c_str());
        if (!GetResourceData(ami, content)) {
            HILOG_ERROR("Get asset content failed.");
            return;
        }
    };
    engine.SetGetAssetFunc(assetFunc);
}

void RegisterWorker(NativeEngine& engine, const std::string& codePath)
{
    RegisterInitWorkerFunc(engine);
    RegisterAssetFunc(engine, codePath);
}

class UvLoopHandler : public AppExecFwk::FileDescriptorListener, public std::enable_shared_from_this<UvLoopHandler> {
public:
    explicit UvLoopHandler(uv_loop_t* uvLoop) : uvLoop_(uvLoop) {}

    void OnReadable(int32_t) override
    {
        HILOG_DEBUG("UvLoopHandler::OnReadable is triggered");
        OnTriggered();
    }

    void OnWritable(int32_t) override
    {
        HILOG_DEBUG("UvLoopHandler::OnWritable is triggered");
        OnTriggered();
    }

private:
    void OnTriggered()
    {
        HILOG_DEBUG("UvLoopHandler::OnTriggered is triggered");

        auto fd = uv_backend_fd(uvLoop_);
        struct epoll_event ev;
        do {
            uv_run(uvLoop_, UV_RUN_NOWAIT);
        } while (epoll_wait(fd, &ev, 1, 0) > 0);

        auto eventHandler = GetOwner();
        if (!eventHandler) {
            return;
        }

        int32_t timeout = uv_backend_timeout(uvLoop_);
        if (timeout < 0) {
            if (haveTimerTask_) {
                eventHandler->RemoveTask(TIMER_TASK);
            }
            return;
        }

        int64_t timeStamp = static_cast<int64_t>(uv_now(uvLoop_)) + timeout;
        if (timeStamp == lastTimeStamp_) {
            return;
        }

        if (haveTimerTask_) {
            eventHandler->RemoveTask(TIMER_TASK);
        }

        auto callback = [wp = weak_from_this()] {
            auto sp = wp.lock();
            if (sp) {
                // Timer task is triggered, so there is no timer task now.
                sp->haveTimerTask_ = false;
                sp->OnTriggered();
            }
        };
        eventHandler->PostTask(callback, TIMER_TASK, timeout);
        lastTimeStamp_ = timeStamp;
        haveTimerTask_ = true;
    }

    uv_loop_t* uvLoop_ = nullptr;
    int64_t lastTimeStamp_ = 0;
    bool haveTimerTask_ = false;
};
} // namespace

std::unique_ptr<Runtime> JsRuntime::Create(const Runtime::Options& options)
{
    std::unique_ptr<JsRuntime> instance = std::make_unique<ArkJsRuntime>();
    if (!instance->Initialize(options)) {
        return std::unique_ptr<Runtime>();
    }
    return instance;
}

bool JsRuntime::Initialize(const Options& options)
{
    // Create event handler for runtime
    eventHandler_ = std::make_shared<AppExecFwk::EventHandler>(options.eventRunner);

    auto uvLoop = nativeEngine_->GetUVLoop();
    auto fd = uvLoop != nullptr ? uv_backend_fd(uvLoop) : -1;
    if (fd < 0) {
        HILOG_ERROR("Failed to get backend fd from uv loop");
        return false;
    }

    // MUST run uv loop once before we listen its backend fd.
    uv_run(uvLoop, UV_RUN_NOWAIT);

    uint32_t events = AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT | AppExecFwk::FILE_DESCRIPTOR_OUTPUT_EVENT;
    eventHandler_->AddFileDescriptorListener(fd, events, std::make_shared<UvLoopHandler>(uvLoop));

    HandleScope handleScope(*this);

    NativeObject* globalObj = ConvertNativeValueTo<NativeObject>(nativeEngine_->GetGlobal());
    if (globalObj == nullptr) {
        HILOG_ERROR("Failed to get global object");
        return false;
    }

    InitConsoleLogModule(*nativeEngine_, *globalObj);
    InitTimerModule(*nativeEngine_, *globalObj);
    InitSyscapModule(*nativeEngine_, *globalObj);

    // Simple hook function 'isSystemplugin'
    BindNativeFunction(*nativeEngine_, *globalObj, "isSystemplugin",
        [](NativeEngine* engine, NativeCallbackInfo* info) -> NativeValue* {
            return engine->CreateUndefined();
        });

    methodRequireNapiRef_.reset(nativeEngine_->CreateReference(globalObj->GetProperty("requireNapi"), 1));
    if (!methodRequireNapiRef_) {
        HILOG_ERROR("Failed to create reference for global.requireNapi");
        return false;
    }
#ifdef SUPPORT_GRAPHICS
    if (options.loadAce) {
        OHOS::Ace::DeclarativeModulePreloader::Preload(*nativeEngine_);
    }
#endif
    codePath_ = options.codePath;

    auto moduleManager = NativeModuleManager::GetInstance();
    std::string packagePath = options.packagePath;
    if (moduleManager && !packagePath.empty()) {
        moduleManager->SetAppLibPath(packagePath.c_str());
    }

    RegisterWorker(*nativeEngine_, options.codePath);

    return true;
}

void JsRuntime::Deinitialize()
{
    for (auto it = modules_.begin(); it != modules_.end(); it = modules_.erase(it)) {
        delete it->second;
        it->second = nullptr;
    }

    methodRequireNapiRef_.reset();

    auto uvLoop = nativeEngine_->GetUVLoop();
    auto fd = uvLoop != nullptr ? uv_backend_fd(uvLoop) : -1;
    if (fd >= 0) {
        eventHandler_->RemoveFileDescriptorListener(fd);
    }
    RemoveTask(TIMER_TASK);

    nativeEngine_.reset();
}

NativeValue* JsRuntime::LoadJsBundle(const std::string& path)
{
    NativeObject* globalObj = ConvertNativeValueTo<NativeObject>(nativeEngine_->GetGlobal());
    NativeValue* exports = nativeEngine_->CreateObject();
    globalObj->SetProperty("exports", exports);

    if (!RunScript(path)) {
        HILOG_ERROR("Failed to run script: %{public}s", path.c_str());
        return nullptr;
    }

    NativeObject* exportsObj = ConvertNativeValueTo<NativeObject>(globalObj->GetProperty("exports"));
    if (exportsObj == nullptr) {
        HILOG_ERROR("Failed to get exports objcect: %{public}s", path.c_str());
        return nullptr;
    }

    NativeValue* exportObj = exportsObj->GetProperty("default");
    if (exportObj == nullptr) {
        HILOG_ERROR("Failed to get default objcect: %{public}s", path.c_str());
        return nullptr;
    }

    return exportObj;
}

std::unique_ptr<NativeReference> JsRuntime::LoadModule(
    const std::string& moduleName, const std::string& modulePath, bool esmodule)
{
    HILOG_INFO("JsRuntime::LoadModule(%{public}s, %{public}s, %{public}s)", moduleName.c_str(), modulePath.c_str(),
        esmodule ? "true" : "false");

    HandleScope handleScope(*this);

    NativeValue* classValue = nullptr;

    auto it = modules_.find(modulePath);
    if (it != modules_.end()) {
        classValue = it->second->Get();
    } else {
        std::string fileName;
        if (!MakeFilePath(codePath_, modulePath, fileName)) {
            HILOG_ERROR("Failed to make module file path: %{private}s", modulePath.c_str());
            return std::unique_ptr<NativeReference>();
        }

        classValue = esmodule ? LoadJsModule(fileName) : LoadJsBundle(fileName);
        if (classValue == nullptr) {
            return std::unique_ptr<NativeReference>();
        }

        modules_.emplace(modulePath, nativeEngine_->CreateReference(classValue, 1));
    }

    NativeValue* instanceValue = nativeEngine_->CreateInstance(classValue, nullptr, 0);
    if (instanceValue == nullptr) {
        HILOG_ERROR("Failed to create object instance");
        return std::unique_ptr<NativeReference>();
    }

    return std::unique_ptr<NativeReference>(nativeEngine_->CreateReference(instanceValue, 1));
}

std::unique_ptr<NativeReference> JsRuntime::LoadSystemModule(
    const std::string& moduleName, NativeValue* const* argv, size_t argc)
{
    HILOG_INFO("JsRuntime::LoadSystemModule(%{public}s)", moduleName.c_str());

    HandleScope handleScope(*this);

    NativeValue* className = nativeEngine_->CreateString(moduleName.c_str(), moduleName.length());
    NativeValue* classValue =
        nativeEngine_->CallFunction(nativeEngine_->GetGlobal(), methodRequireNapiRef_->Get(), &className, 1);
    NativeValue* instanceValue = nativeEngine_->CreateInstance(classValue, argv, argc);
    if (instanceValue == nullptr) {
        HILOG_ERROR("Failed to create object instance");
        return std::unique_ptr<NativeReference>();
    }

    return std::unique_ptr<NativeReference>(nativeEngine_->CreateReference(instanceValue, 1));
}

bool JsRuntime::RunScript(const std::string& path)
{
    return nativeEngine_->RunScript(path.c_str()) != nullptr;
}

bool JsRuntime::RunSandboxScript(const std::string& path)
{
    std::string fileName;
    if (!MakeFilePath(codePath_, path, fileName)) {
        HILOG_ERROR("Failed to make module file path: %{private}s", path.c_str());
        return false;
    }

    if (!RunScript(fileName)) {
        HILOG_ERROR("Failed to run script: %{public}s", fileName.c_str());
        return false;
    }
    return true;
}

#ifdef SUPPORT_GRAPHICS
using OHOS::Ace::ContainerScope;
#endif
class TimerTask final {
public:
    TimerTask(
        JsRuntime& jsRuntime, std::shared_ptr<NativeReference> jsFunction, const std::string &name, int64_t interval)
        : jsRuntime_(jsRuntime), jsFunction_(jsFunction), name_(name), interval_(interval)
    {
#ifdef SUPPORT_GRAPHICS
        containerScopeId_ = ContainerScope::CurrentId();
#endif
#ifdef ENABLE_HITRACE
        traceId_ = new OHOS::HiviewDFX::HiTraceId(OHOS::HiviewDFX::HiTrace::GetId());
#endif
    }

    ~TimerTask() = default;

    void operator()()
    {
        if (interval_ > 0) {
            jsRuntime_.PostTask(*this, name_, interval_);
        }
#ifdef SUPPORT_GRAPHICS
        // call js function
        ContainerScope containerScope(containerScopeId_);
#endif
        HandleScope handleScope(jsRuntime_);

        std::vector<NativeValue*> args_;
        args_.reserve(jsArgs_.size());
        for (auto arg : jsArgs_) {
            args_.emplace_back(arg->Get());
        }

        NativeEngine& engine = jsRuntime_.GetNativeEngine();
#ifdef ENABLE_HITRACE
        if (traceId_ && traceId_->IsValid()) {
            OHOS::HiviewDFX::HiTrace::SetId(*traceId_);
            engine.CallFunction(engine.CreateUndefined(), jsFunction_->Get(), args_.data(), args_.size());
            OHOS::HiviewDFX::HiTrace::ClearId();
            delete traceId_;
            traceId_ = nullptr;
            return;
        }
#endif
        engine.CallFunction(engine.CreateUndefined(), jsFunction_->Get(), args_.data(), args_.size());
    }

    void PushArgs(std::shared_ptr<NativeReference> ref)
    {
        jsArgs_.emplace_back(ref);
    }

private:
    JsRuntime& jsRuntime_;
    std::shared_ptr<NativeReference> jsFunction_;
    std::vector<std::shared_ptr<NativeReference>> jsArgs_;
    std::string name_;
    int64_t interval_ = 0;
#ifdef SUPPORT_GRAPHICS
    int32_t containerScopeId_ = 0;
#endif
#ifdef ENABLE_HITRACE
    OHOS::HiviewDFX::HiTraceId* traceId_ = nullptr;
#endif
};

void JsRuntime::PostTask(const TimerTask& task, const std::string& name, int64_t delayTime)
{
    eventHandler_->PostTask(task, name, delayTime);
}

void JsRuntime::RemoveTask(const std::string& name)
{
    eventHandler_->RemoveTask(name);
}

NativeValue* JsRuntime::SetCallbackTimer(NativeEngine& engine, NativeCallbackInfo& info, bool isInterval)
{
    // parameter check, must have at least 2 params
    if (info.argc < 2 || info.argv[0]->TypeOf() != NATIVE_FUNCTION || info.argv[1]->TypeOf() != NATIVE_NUMBER) {
        HILOG_ERROR("Set callback timer failed with invalid parameter.");
        return engine.CreateUndefined();
    }

    // parse parameter
    std::shared_ptr<NativeReference> jsFunction(engine.CreateReference(info.argv[0], 1));
    int64_t delayTime = *ConvertNativeValueTo<NativeNumber>(info.argv[1]);
    uint32_t callbackId = callbackId_++;
    std::string name = "JsRuntimeTimer_";
    name.append(std::to_string(callbackId));

    // create timer task
    TimerTask task(*this, jsFunction, name, isInterval ? delayTime : 0);
    for (size_t index = 2; index < info.argc; ++index) {
        task.PushArgs(std::shared_ptr<NativeReference>(engine.CreateReference(info.argv[index], 1)));
    }

    JsRuntime::PostTask(task, name, delayTime);
    return engine.CreateNumber(callbackId);
}

NativeValue* JsRuntime::ClearCallbackTimer(NativeEngine& engine, NativeCallbackInfo& info)
{
    // parameter check, must have at least 1 param
    if (info.argc < 1 || info.argv[0]->TypeOf() != NATIVE_NUMBER) {
        HILOG_ERROR("Clear callback timer failed with invalid parameter.");
        return engine.CreateUndefined();
    }

    uint32_t callbackId = *ConvertNativeValueTo<NativeNumber>(info.argv[0]);
    std::string name = "JsRuntimeTimer_";
    name.append(std::to_string(callbackId));

    // event should be cancelable before executed
    JsRuntime::RemoveTask(name);
    return engine.CreateUndefined();
}

void JsRuntime::DumpHeapSnapshot(bool isPrivate)
{
    nativeEngine_->DumpHeapSnapshot(true, DumpFormat::JSON, isPrivate);
}

std::string JsRuntime::BuildJsStackTrace()
{
    std::string straceStr = "";
    [[maybe_unused]]bool temp = nativeEngine_->BuildJsStackTrace(straceStr);
    return straceStr;
}

void JsRuntime::NotifyApplicationState(bool isBackground)
{
    if (nativeEngine_ == nullptr) {
        HILOG_INFO("NotifyApplicationState, nativeEngine_ is nullptr");
        return;
    }
    nativeEngine_->NotifyApplicationState(isBackground);
    HILOG_INFO("NotifyApplicationState, isBackground %{public}d.", isBackground);
}
}  // namespace AbilityRuntime
}  // namespace OHOS
