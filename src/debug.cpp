#include "debug.h"


int Debug::failedFrames = 0;
int Debug::frames = 0;
void Debug::addFailedFrame() {
    failedFrames++;
}
void Debug::addFrame() {
    frames++;
}

JsonBuilder& Debug::getJsonReport(JsonBuilder& jb) {
    jb.beginObject("report")
        .add("failedFrames", failedFrames)
        .add("successFrames", frames)
        .add("totalFrames", failedFrames + frames);
    jb.endObject();
    return jb;
}
