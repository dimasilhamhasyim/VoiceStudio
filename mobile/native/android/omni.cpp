#include "control.h"
#include "omnivoice.h"
#include <android/log.h>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
// VSDIAG (fork build): capture library log lines so the app can show where a
// clone run stops. Lines are kept process-wide and mirrored to logcat.
namespace {
std::mutex vsdiag_mutex;
std::deque<std::string> vsdiag_lines;
void vsdiag_append(const char * message) {
    std::lock_guard<std::mutex> lock(vsdiag_mutex);
    if (vsdiag_lines.size() >= 120) vsdiag_lines.pop_front();
    vsdiag_lines.emplace_back(message ? message : "");
}
void vsdiag_install() {
    ov_log_set([](ov_log_level, const char * message, void *) {
        vsdiag_append(message);
        __android_log_print(ANDROID_LOG_INFO, "VSDiag", "%s", message ? message : "");
    }, nullptr);
}
}
extern "C" JNIEXPORT jint JNICALL Java_com_voicestudio_mobile_local_NativeBridge_omniAbi(JNIEnv*,jobject) {return OV_ABI_VERSION;}
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_voicestudio_mobile_local_NativeBridge_cloneAudio(JNIEnv* e,jobject,jstring model,jstring codec,jstring text,jstring refText,jfloatArray reference,jobject control) {
    Utf8 m(e,model),c(e,codec),t(e,text),r(e,refText); Control ctl(e,control);
    vsdiag_install();
    ov_init_params init{}; ov_init_default_params(&init); init.model_path=m.chars;init.codec_path=c.chars;init.use_fa=false;
    if(Control::cancelled(&ctl))return nullptr;
    std::unique_ptr<ov_context,decltype(&ov_free)> context(ov_init(&init),ov_free);
    if(!context){fail(e);return nullptr;} ctl.progress(0.2f);
    std::vector<float> ref(e->GetArrayLength(reference));e->GetFloatArrayRegion(reference,0,ref.size(),ref.data());
    ov_tts_params params{};ov_tts_default_params(&params);params.text=t.chars;params.ref_text=r.chars;
    params.ref_audio_24k=ref.data();params.ref_n_samples=ref.size();params.cancel=Control::cancelled;params.cancel_user_data=&ctl;
    ov_audio audio{};auto status=ov_synthesize(context.get(),&params,&audio);
    jfloatArray result=nullptr;
    if(status==OV_STATUS_OK&&!Control::cancelled(&ctl)){result=e->NewFloatArray(audio.n_samples);e->SetFloatArrayRegion(result,0,audio.n_samples,audio.samples);ctl.progress(1.f);}
    else if(status!=OV_STATUS_CANCELLED&&!Control::cancelled(&ctl))fail(e);
    ov_audio_free(&audio);if(ctl.failed)fail(e);return result;
}

extern "C" JNIEXPORT jfloatArray JNICALL Java_com_voicestudio_mobile_local_NativeBridge_designAudio(JNIEnv* e,jobject,jstring model,jstring codec,jstring text,jstring instruction,jlong seed,jobject control) {
    Utf8 m(e,model),c(e,codec),t(e,text),i(e,instruction); Control ctl(e,control);
    vsdiag_install();
    ov_init_params init{}; ov_init_default_params(&init);init.model_path=m.chars;init.codec_path=c.chars;init.use_fa=false;
    if(Control::cancelled(&ctl)) return nullptr;
    std::unique_ptr<ov_context,decltype(&ov_free)> context(ov_init(&init),ov_free);
    if(!context){fail(e);return nullptr;} ctl.progress(.2f);
    ov_tts_params params{};ov_tts_default_params(&params);params.text=t.chars;params.instruct=i.chars;params.mg_seed=static_cast<uint64_t>(seed);
    params.cancel=Control::cancelled;params.cancel_user_data=&ctl;
    ov_audio audio{};auto status=ov_synthesize(context.get(),&params,&audio);
    jfloatArray result=nullptr;
    if(status==OV_STATUS_OK&&!Control::cancelled(&ctl)){result=e->NewFloatArray(audio.n_samples);e->SetFloatArrayRegion(result,0,audio.n_samples,audio.samples);ctl.progress(1.f);}
    else if(status!=OV_STATUS_CANCELLED&&!Control::cancelled(&ctl))fail(e);
    ov_audio_free(&audio);if(ctl.failed)fail(e);return result;
}

// VSDIAG (fork build): JNI accessors for the captured native log.
extern "C" JNIEXPORT jstring JNICALL Java_com_voicestudio_mobile_local_NativeBridge_diagLog(JNIEnv* e,jobject) {
    std::string joined;
    {std::lock_guard<std::mutex> lock(vsdiag_mutex);
     for (const auto & line : vsdiag_lines) {joined += line; joined += '\n';}}
    return e->NewStringUTF(joined.c_str());
}
extern "C" JNIEXPORT void JNICALL Java_com_voicestudio_mobile_local_NativeBridge_diagClear(JNIEnv*,jobject) {
    std::lock_guard<std::mutex> lock(vsdiag_mutex);
    vsdiag_lines.clear();
}
