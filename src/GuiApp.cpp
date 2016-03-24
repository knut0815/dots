#include "GuiApp.h"

void GuiApp::setup(){

    ofSoundStream ss;
    soundDevices = ss.getDeviceList();
    nSoundDevices = soundDevices.size();

    audioInputParameters.setName("Audio Input");
    audioInputParameters.add(audioMode.set("audioMode", AUDIO_MODE_MIC, 0, N_AUDIO_MODES-1));
    audioInputParameters.add(soundStreamDevice.set("soundStreamDevice", 0, 0, nSoundDevices-1));
    inputGui.setup(audioInputParameters);

    // Auto select loopback if available
    for (int i = 0; i < soundDevices.size(); ++i) {
        ofSoundDevice &sd = soundDevices[i];
        if (sd.name.find("Soundflower") != string::npos && sd.name.find("2ch") != string::npos) {
            soundStreamDevice.set(i);
        }
    }

    // Debug
    debugParameters.setName("Debug");
    debugParameters.add(drawMode.set("drawMode", 0, 0, 1));
    debugParameters.add(nFlameSequences.set("nFlameSequences", 5, 1, 10));
    debugParameters.add(maxPixels.set("maxPixels", 5000000, 10000, 100000000));
    debugParameters.add(pctToAllowRandom.set("pctToAllowRandom", 0.02, 0, 0.1));
    debugGui.setup(debugParameters);

    // Meta
    metaParams.setName("Meta");
    metaParams.add(wandering.set("wandering", false));
    displayParameters.add(metaParams);

    // Audio analysis
    audioAnalysisParameters.setName("Audio Analysis");
    audioAnalysisParameters.add(fftDecayRate.set("fftDecayRate", 0.9, 0, 1));
    audioAnalysisParameters.add(centroidMaxBucket.set("centroidMax (mpx)", 0.35, 0, 1));
    audioAnalysisParameters.add(rmsMultiple.set("rmsMult (mpy)", 5, 0, 15));
    audioAnalysisParameters.add(mpxSmoothingFactor.set("mpxSmoothingFactor", 0.4, 0, 1));
    audioAnalysisParameters.add(mpySmoothingFactor.set("mpySmoothingFactor", 0.1, 0, 1));
    displayParameters.add(audioAnalysisParameters);

    // Drawing
    drawingParams.setName("Drawing");
    drawingParams.add(clearSpeed.set("clearSpeed", 50, 0, 255));
    drawingParams.add(particleAlpha.set("particleAlpha", 50, 0, 255));
    drawingParams.add(overallScale.set("overallScale", 1, 0.1, 3.0));
    displayParameters.add(drawingParams);

    // Rotation / Interpolation speed
    speedParams.setName("Speed");
    speedParams.add(baseSpeed.set("baseSpeed", 0, 0, 10));
    speedParams.add(rmsSpeedMult.set("rmsSpeedMult", 30, 0, 100));
    displayParameters.add(speedParams);

    // Dot size
    dotParams.setName("Dots");
    dotParams.add(pointRadiusUsesAudio.set("dotSizeUsesAudio", true));
    dotParams.add(pointRadiusAudioScale.set("dotAudioScale", 10, 0, 50));
    dotParams.add(basePointRadius.set("baseDotRadius", 10, 0, 50));
    displayParameters.add(dotParams);

    // Line size
    lineParams.setName("Lines");
    lineParams.add(maxLineLength.set("maxLineLength", 100, 0, 3000));
    displayParameters.add(lineParams);

    // Audio effects
    audioEffectParams.setName("Audio Effect Sizes");
    audioEffectParams.add(audioEffectSize1.set("audioEffectSize1", 1, 0, 1));
    audioEffectParams.add(audioEffectSize2.set("audioEffectSize2", 1, 0, 1));
    audioEffectParams.add(audioEffectSize3.set("audioEffectSize3", 1, 0, 1));
    audioEffectParams.add(audioEffectSize4.set("audioEffectSize4", 1, 0, 1));
    displayParameters.add(audioEffectParams);

    displayParameters.setName("Display");
    displayGui.setup(displayParameters);

    inputGui.setPosition(10, 450);
    analysisGui.setPosition(inputGui.getPosition().x, inputGui.getPosition().y + inputGui.getHeight() + 10);
    debugGui.setPosition(inputGui.getPosition().x + inputGui.getWidth() + 10, inputGui.getPosition().y);
    
    displayGui.setPosition(580, 15);
    ofSetWindowShape(displayGui.getPosition().x + displayGui.getWidth() + 10,
                     displayGui.getPosition().y + displayGui.getHeight() + 10);
    
    visuals = NULL;
    audioBuckets = NULL;
    nAudioBuckets = 0;
    frameRate = 0;
    pctParticles = 0;
    genomeIdx = 0;
    mpx = 0;
    mpy = 0;
    
    ofBackground(0);
    ofSetVerticalSync(false);
}

void GuiApp::update(){
    if (ofGetFrameNum() % 30 == 0) {
        // Every 30 frames, refresh some data...
        ofSoundStream ss;
        soundDevices = ss.getDeviceList();
        if (soundDevices.size() != nSoundDevices) {
            if (soundStreamDevice.get() >= soundDevices.size()) {
                soundStreamDevice.set(0);
            }
            nSoundDevices = soundDevices.size();
            soundStreamDevice.setMax(nSoundDevices);
        }
    }
}

void GuiApp::draw() {
    char s[512];
    float margin = 10, padding = 10;
    
    ofPushMatrix();
    
    if (frameRate) {
        sprintf(s, "%.2f fps", frameRate);
        ofDrawBitmapString(s, 10, 15);
    }

    if (pctParticles) {
        sprintf(s, "%d%% particles", (int)(pctParticles*100));
        ofDrawBitmapString(s, 150, 15);
    }

    sprintf(s, "genome: %d", genomeIdx);
    ofDrawBitmapString(s, 300, 15);

    ofTranslate(0, 17);

    if (visuals) {
        ofTranslate(0, margin);
        // TODO fit visuals to a bounding box
        
        float scale = min(480.0 / visuals->getWidth(), 270.0 / visuals->getHeight());
        float width = visuals->getWidth() * scale;
        float height = visuals->getHeight() * scale;

        ofFill();
        ofSetColor(255, 255);
        ofDrawRectangle(margin, 0, width + padding * 2, height + padding * 2);
        ofSetColor(0, 255);
        ofDrawRectangle(margin + padding, padding, width, height);
        ofSetColor(255, 255);
        visuals->draw(margin + padding, padding, width, height);

        ofTranslate(0, 270 + padding * 2);
    }
    
    if (audioBuckets) {
        ofTranslate(0, 100 + margin);

        ofPushStyle();

        ofSetColor(255);
        ofNoFill();
        ofSetLineWidth(1);
        
        float fftWidth = 480.0, fftHeight = 100;
        float fftBucketWidth = fftWidth / nAudioBuckets;
        
        ofPushMatrix();
        
        ofTranslate(margin, 0);
        ofDrawRectangle(0, 0, fftWidth, -fftHeight);
        ofBeginShape();
        for (int i = 0; i < nAudioBuckets; i++) {
            ofVertex(i * fftBucketWidth, -audioBuckets[i] * fftHeight);
        }
        ofEndShape();
        
        string audioModeDesc;
        if (audioMode == AUDIO_MODE_MIC) {
            audioModeDesc = "Microphone Input";
        } else if (audioMode == AUDIO_MODE_MP3) {
            audioModeDesc = "MP3 Playing";
        } else if (audioMode == AUDIO_MODE_NOISE) {
            audioModeDesc = "White Noise";
        } else {
            audioModeDesc = "No Audio";
        }
        ofDrawBitmapString(audioModeDesc, 5, 15-fftHeight);

        if (soundStreamDevice < soundDevices.size()) {
            ofDrawBitmapString(soundDevices[soundStreamDevice].name, 5, 30-fftHeight);
        }
        
        // Draw audio centroid
        float mappedCentroid = fftWidth * audioCentroid;
        ofSetColor(200, 0, 0);
        ofDrawLine(mappedCentroid, 0, mappedCentroid, -fftHeight);
        ofDrawBitmapString("centroid", mappedCentroid + 4, -30);
        float mappedCentroidMax = fftWidth * centroidMaxBucket;
        ofSetColor(125, 0, 0);
        ofDrawLine(mappedCentroidMax, 0, mappedCentroidMax, -fftHeight);
        ofDrawBitmapString("centroidMax", mappedCentroidMax + 4, -30);
        
        // Draw MPX, MPY
        float mpxHeight = 100 * mpx;
        float mpyHeight = 100 * mpy;
        ofFill();
        ofSetColor(255);
        ofDrawBitmapString("mpx", fftWidth + 10, -100);
        ofDrawRectangle(fftWidth + 10, 0, 25, -mpxHeight);
        ofDrawBitmapString("mpy", fftWidth + 40, -100);
        ofDrawRectangle(fftWidth + 40, 0, 25, -mpyHeight);
        
        ofPopMatrix();
        ofPopStyle();
    }
    
    ofPopMatrix();

    inputGui.draw();
    debugGui.draw();
    analysisGui.draw();
    displayGui.draw();
}

void GuiApp::handleKey(int key) {
    // None for now
}

void GuiApp::keyPressed(int key) {
    keyPresses.push(key);
}